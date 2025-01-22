#include "play.h"
#include <algorithm>
#include <iostream>
#include <numeric>
#include <tuple>
#include <vector>
#include <fmt/core.h>
#include "synthseq.h"

class IndexEvent {
 public:
  IndexEvent(uint32_t time=0, size_t track=0, size_t tei=0) :
    time_{time}, track_{track}, tei_{tei} {}
  uint32_t time_{0}; // sum of delta_time
  size_t track_{0};
  size_t tei_{0}; // track event index;
};
bool operator<(const IndexEvent& ie0, const IndexEvent& ie1) {
  return
    std::tie(ie0.time_, ie0.track_, ie0.tei_) <
    std::tie(ie1.time_, ie1.track_, ie1.tei_);
}

class AbsEvent {
 public:
  AbsEvent(uint32_t time_ms=0, uint32_t time_ms_original=0) :
    time_ms_{time_ms}, time_ms_original_{time_ms_original} {}
  virtual ~AbsEvent() {}
  uint32_t time_ms_;
  uint32_t time_ms_original_;
};

class NoteEvent : public AbsEvent {
 public:
  NoteEvent(
    uint32_t time_ms=0,
    uint32_t time_ms_original=0,
    int channel=0,
    int16_t key=0,
    int16_t velocity=0,
    uint32_t duration_ms=0,
    uint32_t duration_ms_original=0) :
      AbsEvent(time_ms, time_ms_original),
      channel_{channel},
      key_{key},
      velocity_{velocity},
      duration_ms_{duration_ms},
      duration_ms_original_{duration_ms_original} {}
  virtual ~NoteEvent() {}
  int channel_;
  int16_t key_;
  int16_t velocity_;
  uint32_t duration_ms_;
  uint32_t duration_ms_original_;
};

class ProgramChange : public AbsEvent {
 public:
  ProgramChange(
    uint32_t time_ms=0,
    uint32_t time_ms_original=0,
    int channel=0,
    int program=0) :
      AbsEvent{time_ms, time_ms_original},
      channel_{channel},
      program_{program} {
  }
  virtual ~ProgramChange() {}
  int channel_;
  int program_;
};

class PitchWheel : public AbsEvent {
 public:
  PitchWheel(
    uint32_t time_ms=0,
    uint32_t time_ms_original=0,
    int channel=0,
    int bend=0) :
      AbsEvent{time_ms, time_ms_original},
      channel_{channel},
      bend_{bend} {
  }
  virtual ~PitchWheel() {}
  int channel_;
  int bend_;
};

class FinalEvent : public AbsEvent {
 public:
  FinalEvent(uint32_t time_ms=0, uint32_t time_ms_original=0) :
    AbsEvent{time_ms, time_ms_original} {}
  virtual ~FinalEvent() {}
};

class DynamicTiming {
 public:
  DynamicTiming(
    uint64_t microseconds_per_quarter=0,
    uint64_t k_ticks_per_quarter=0,
    uint32_t ticks_ref=0,
    uint32_t ms_ref=0) : 
      microseconds_per_quarter_{microseconds_per_quarter},
      k_ticks_per_quarter_{k_ticks_per_quarter},
      ticks_ref_{ticks_ref},
      ms_ref_{ms_ref} {
  }
  void SetMicrosecondsPerQuarter(uint32_t curr_ticks, uint64_t ms_per_quarter) {
    ms_ref_ = AbsTicksToMs(curr_ticks);
    ticks_ref_ = curr_ticks;
    microseconds_per_quarter_ = ms_per_quarter;
  }
  uint32_t TicksToMs(uint32_t ticks) const {
    uint64_t numer = uint64_t{ticks} * microseconds_per_quarter_;
    uint32_t ms = RoundDiv(numer, k_ticks_per_quarter_);
    return ms;
  }
  uint32_t AbsTicksToMs(uint32_t abs_ticks) {
    uint64_t numer = uint64_t{abs_ticks - ticks_ref_} *
      microseconds_per_quarter_;
    uint32_t add = RoundDiv(numer, k_ticks_per_quarter_);
    uint32_t ms = ms_ref_ + add;
    return ms;
  }
 private:
  static uint32_t RoundDiv(uint64_t n, uint64_t d) {
    static const uint64_t u64max32 = std::numeric_limits<uint32_t>::max();
    uint64_t q = (n + d/2) / d;
    if (q > u64max32) {
      std::cerr << fmt::format("overflow @ RoundDiv({}, {})", n, d);
    }
    uint32_t ret = static_cast<uint32_t>(q);
    return ret;
  }
  uint64_t microseconds_per_quarter_{0};
  uint64_t k_ticks_per_quarter_{0};
  uint32_t ticks_ref_{0};
  uint32_t ms_ref_{0};
};

////////////////////////////////////////////////////////////////////////

class Player {
 public:
  Player(const midi::Midi &pm, SynthSequencer &ss, const PlayParams &pp) :
    pm_{pm}, ss_{ss}, pp_{pp} {}
  int run();

 private:
  void SetIndexEvents();
  uint32_t GetFirstNoteTime();
  void SetAbsEvents();
  void HandleMeta(const midi::MetaEvent*, DynamicTiming&, uint32_t ts);
  void HandleMidi(
    const midi::MidiEvent*,
    DynamicTiming& dyn_timing,
    size_t index_event_index,
    uint32_t date_ms);
  uint32_t GetNoteDuration(size_t iei, const midi::NoteOnEvent &note_on) const;
  static uint32_t FactorU32(double f, uint32_t u);
  static void MaxBy(uint32_t &v, uint32_t x) { if (v < x) { v = x; } }

  int rc_{0};

  const midi::Midi &pm_; // parsed_midi
  SynthSequencer &ss_;
  const PlayParams &pp_;

  std::vector<IndexEvent> index_events_;
  std::vector<std::unique_ptr<AbsEvent>> abs_events_;
};

int Player::run() {
  if (pp_.debug_ & 0x1) { std::cerr << "Player::run() begin\n"; }
  SetIndexEvents();
  if (pp_.debug_ & 0x1) { std::cerr << "Player::run() end\n"; }
  SetAbsEvents();
  return rc_;
}

void Player::SetIndexEvents() {
  const std::vector<midi::Track> &tracks = pm_.GetTracks();
  const size_t ne = std::accumulate(tracks.begin(), tracks.end(), size_t{0},
    [](size_t r, const midi::Track &track) {
      return r + track.events_.size();
    });
  if (pp_.debug_ & 0x1) { std::cerr << fmt::format("Total events: {}\n", ne); }
  index_events_.reserve(ne);
  for (size_t ti = 0; ti < tracks.size(); ++ti) {
    const midi::Track &track = tracks[ti];
    const std::vector<std::unique_ptr<midi::Event>> &events = track.events_;
    const size_t nte = events.size();
    uint32_t time = 0;
    for (size_t tei = 0; tei < nte; ++tei) {
      uint32_t dt = events[tei]->delta_time_;
      time += dt;
      index_events_.push_back(IndexEvent(time, ti, tei));
    }
  }
  std::sort(index_events_.begin(), index_events_.end());
}

void Player::SetAbsEvents() {
  const std::vector<midi::Track> &tracks = pm_.GetTracks();
  DynamicTiming dyn_timing{
    500000,
    1000ull * uint64_t{pm_.GetTicksPerQuarterNote()},
    0, 0};
  uint32_t first_note_time = GetFirstNoteTime();
  bool done = false;
  size_t ie_size = index_events_.size();
  auto safe_subtract = [](uint32_t l, uint32_t r) { return l < r ? 0 : l - r; };
  for (size_t i = 0; (i < ie_size) && !done; ++i) {
    const IndexEvent &ie = index_events_[i];
    uint32_t time_shifted = safe_subtract(ie.time_, first_note_time);
    uint32_t date_ms = dyn_timing.AbsTicksToMs(time_shifted);
    done = date_ms > pp_.end_ms_;
    if (!done) {
      const midi::Event *e = tracks[ie.track_].events_[ie.tei_].get();
      if (pp_.debug_ & 0x80) {
        std::cout << fmt::format("[{:4}] time={} shifted={}, track_event={}\n",
          i, ie.time_, time_shifted, e->str());
      }
      const midi::MetaEvent *meta_event =
        dynamic_cast<const midi::MetaEvent*>(e);
      const midi::MidiEvent *midi_event =
        dynamic_cast<const midi::MidiEvent*>(e);
      if (meta_event) {
        HandleMeta(meta_event, dyn_timing, time_shifted);
      } else if (midi_event) {
        HandleMidi(midi_event, dyn_timing, i, date_ms);
      }
    }
  }
}

uint32_t Player::GetFirstNoteTime() {
  uint32_t t = 0;
  bool note_seen = false;
  const std::vector<midi::Track> &tracks = pm_.GetTracks();
  for (size_t i = 0; (i < index_events_.size()) && !note_seen; ++i) {
    const IndexEvent &ie = index_events_[i];
    midi::Event *e = tracks[ie.track_].events_[ie.tei_].get();
    midi::NoteOnEvent *note_on = dynamic_cast<midi::NoteOnEvent*>(e);
    if (note_on) {
      note_seen = true;
      t = ie.time_;
    }
  }
  return t;
}

void Player::HandleMeta(
    const midi::MetaEvent* me,
    DynamicTiming& dyn_timing,
    uint32_t time_shifted) {
  const midi::TempoEvent *tempo = dynamic_cast<const midi::TempoEvent*>(me);
  if (tempo) {
    dyn_timing.SetMicrosecondsPerQuarter(time_shifted, tempo->tttttt_);
  }
}

void Player::HandleMidi(
    const midi::MidiEvent* me,
    DynamicTiming &dyn_timing,
    size_t index_event_index,
    uint32_t date_ms) {
  const bool after_begin = pp_.begin_ms_ <= date_ms;
  uint32_t date_ms_modified = after_begin
    ? FactorU32(pp_.tempo_factor_, date_ms - pp_.begin_ms_)
    : pp_.begin_ms_;
  const midi::MidiVarByte vb = me->VarByte();
  switch (vb) {
   case midi::MidiVarByte::NOTE_OFF_x0: // handled by NOTE_ON
    break;
   case midi::MidiVarByte::NOTE_ON_x1: {
    const midi::NoteOnEvent *note_on =
      dynamic_cast<const midi::NoteOnEvent*>(me);
      if (after_begin && note_on->velocity_ != 0) {
        uint32_t duration_ticks = GetNoteDuration(index_event_index, *note_on);
        uint32_t duration_ms = dyn_timing.TicksToMs(duration_ticks);
      }
    }
    break;
   case midi::MidiVarByte::PROGRAM_CHANGE_x4:
    break;
   case midi::MidiVarByte::PITCH_WHEEL_x6:
    break;
   default: // ignored
    break;
  }
}

uint32_t Player::GetNoteDuration(
    size_t iei,
    const midi::NoteOnEvent& note_on) const {
  uint32_t curr_time = 0;
  const std::vector<midi::Track> &tracks = pm_.GetTracks();
  bool end_note_found = false;
  for (size_t i = iei + 1; (i < index_events_.size()) && !end_note_found; ++i) {
    const IndexEvent &ie = index_events_[i];
    curr_time = ie.time_;
    const midi::Event *e = tracks[ie.track_].events_[ie.tei_].get();
    const midi::NoteOffEvent *note_off =
      dynamic_cast<const midi::NoteOffEvent*>(e);
    const midi::NoteOnEvent *note_on1 =
      dynamic_cast<const midi::NoteOnEvent*>(e);
    if (note_off) {
      end_note_found = (note_off->channel_ == note_on.channel_) &&
        (note_off->key_ == note_on.key_);
    } else if (note_on1) {
      end_note_found = (note_on1->velocity_ == 0) &&
        (note_on1->channel_ == note_on.channel_) &&
        (note_on1->key_ == note_on.key_);
    }
  }
  uint32_t dur = curr_time - index_events_[iei].time_;
  return dur;
}

uint32_t Player::FactorU32(double f, uint32_t u) {
  static const double dmaxu32 = std::numeric_limits<uint32_t>::max();
  uint32_t ret = 0;
  double fu = (f * u) + (1./2.);
  if (fu > dmaxu32) {
    std::cerr << fmt::format("Overflow factor_u32(f={}, u={})", f, u);
    ret = u;
  } else {
    ret = static_cast<uint32_t>(fu);
  }
  return ret;
}

////////////////////////////////////////////////////////////////////////

int play(
    const midi::Midi &parsed_midi,
    SynthSequencer &synth_sequencer,
    const PlayParams &play_params) {
  int rc = Player(parsed_midi, synth_sequencer, play_params).run();
  return rc;
}
