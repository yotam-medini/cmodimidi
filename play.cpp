#include "play.h"
#include <algorithm>
#include <array>
#include <condition_variable>
#include <iostream>
#include <iostream>
#include <mutex>
#include <numeric>
#include <tuple>
#include <vector>
#include <fmt/core.h>
#include <fluidsynth.h>
#include "synthseq.h"
#include "util.h"

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

class Player; // forward

class AbsEvent {
 public:
  AbsEvent(uint32_t time_ms=0, uint32_t time_ms_original=0) :
    time_ms_{time_ms}, time_ms_original_{time_ms_original} {}
  virtual ~AbsEvent() {}
  virtual void SetSendFluidEvent(
    fluid_event_t *event, const Player *player, uint32_t date_ms) = 0;
  virtual uint32_t end_time_ms() const { return time_ms_; }
  virtual std::string str() const = 0;
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
  void SetSendFluidEvent(
    fluid_event_t *event, const Player *player, uint32_t date_ms);
  uint32_t end_time_ms() const { return time_ms_ + duration_ms_; }
  std::string str() const {
    return fmt::format(
      "Note(t={}, channel={}, key={}, velocity={}, duration={})",
      time_ms_, channel_, key_, velocity_, duration_ms_);
  }
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
  void SetSendFluidEvent(
    fluid_event_t *event, const Player *player, uint32_t date_ms);
  std::string str() const {
    return fmt::format("ProgramChange(t={}, channel={}, program={})",
      time_ms_, channel_, program_);
  }
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
  void SetSendFluidEvent(
    fluid_event_t *event, const Player *player, uint32_t date_ms);
  std::string str() const {
    return fmt::format("PitchWheel(t={}, channel={}, bend={})",
      time_ms_, channel_, bend_);
  }
  int channel_;
  int bend_;
};

class FinalEvent : public AbsEvent {
 public:
  FinalEvent(uint32_t time_ms=0, uint32_t time_ms_original=0) :
    AbsEvent{time_ms, time_ms_original} {}
  virtual ~FinalEvent() {}
  void SetSendFluidEvent(
    fluid_event_t *event, const Player *player, uint32_t date_ms);
  std::string str() const { return fmt::format("Final(t={})", time_ms_); }
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
    uint64_t number = uint64_t{ticks} * microseconds_per_quarter_;
    uint32_t ms = RoundDiv(number, k_ticks_per_quarter_);
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

class CallBackData {
 public:
  enum CallBack { Periodic, Final, Progress };
  CallBackData(CallBack ecb, Player *player) : ecb_{ecb}, player_{player} {}
  CallBack ecb_;
  Player *player_;
};

////////////////////////////////////////////////////////////////////////

class Player {
 public:
  enum SeqId : size_t { 
    SeqIdSynth, SeqIdPeriodic, SeqIdFinal, SeqIdProgress, SeqId_N };
  Player(const midi::Midi &pm, SynthSequencer &ss, const PlayParams &pp) :
    pm_{pm}, ss_{ss}, pp_{pp} {
    std::fill(seq_ids_.begin(), seq_ids_.end(), -1);
    seq_ids_[SeqIdSynth] = ss.synth_seq_id_;
  }
  const SynthSequencer &GetSynthSequencer() const { return ss_; }
  int GetSeqId(SeqId esi) const { return seq_ids_[esi]; }
  int run();

 private:
  void SetIndexEvents();
  uint32_t GetFirstNoteTime();
  void SetAbsEvents();
  void play();
  void HandleMeta(const midi::MetaEvent*, DynamicTiming&, uint32_t ts);
  void HandleMidi(
    const midi::MidiEvent*,
    DynamicTiming& dyn_timing,
    size_t index_event_index,
    uint32_t date_ms);
  uint32_t GetNoteDuration(size_t iei, const midi::NoteOnEvent &note_on) const;
  static uint32_t FactorU32(double f, uint32_t u);
  static void MaxBy(uint32_t &v, uint32_t x) { if (v < x) { v = x; } }

  static void callback(
    unsigned int time,
    fluid_event_t *event,
    fluid_sequencer_t *seq,
    void *data);
  void periodic_callback(
    unsigned int time,
    fluid_event_t *event,
    fluid_sequencer_t *seq);
  void final_callback(
    unsigned int time,
    fluid_event_t *event,
    fluid_sequencer_t *seq);
  void progress_callback(
    unsigned int time,
    fluid_event_t *event,
    fluid_sequencer_t *seq);
  void SchedulePeriodicAt(uint32_t at) {
    ScheduleCallback(seq_ids_[SeqIdPeriodic], at);
  }
  void ScheduleProgressAt(uint32_t at) {
    ScheduleCallback(seq_ids_[SeqIdProgress], at);
  }
  void ScheduleCallback(int seq_id, uint32_t at);

  int rc_{0};

  const midi::Midi &pm_; // parsed_midi
  SynthSequencer &ss_;
  const PlayParams &pp_;

  std::vector<IndexEvent> index_events_;
  std::vector<std::unique_ptr<AbsEvent>> abs_events_;
  uint32_t final_ms_{0};

  std::array<int, SeqId_N>  seq_ids_;
  size_t next_send_index_{0};
  uint32_t date_add_ms_{0};
  bool final_handled_{false};
  std::mutex sending_mtx_;
  std::mutex play_mtx_;
  std::condition_variable cv_;
};

int Player::run() {
  if (pp_.debug_ & 0x1) { std::cerr << "Player::run() begin\n"; }
  SetIndexEvents();
  if (pp_.debug_ & 0x1) { std::cerr << "Player::run() end\n"; }
  SetAbsEvents();
  play();
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
  if (pp_.debug_ & 0x100) { std::cout << "Raw events:\n"; }
  for (size_t ti = 0; ti < tracks.size(); ++ti) {
    if (pp_.debug_ & 0x100) {
      std::cout << fmt::format("Track[{}]", ti) << "{\n";
    }
    const midi::Track &track = tracks[ti];
    const std::vector<std::unique_ptr<midi::Event>> &events = track.events_;
    const size_t nte = events.size();
    uint32_t time = 0;
    for (size_t tei = 0; tei < nte; ++tei) {
      if (pp_.debug_ & 0x100) {
        std::cout << fmt::format("  [{:4d}] {}\n", tei, events[tei]->dt_str());
      }
      uint32_t dt = events[tei]->delta_time_;
      time += dt;
      index_events_.push_back(IndexEvent(time, ti, tei));
    }
    if (pp_.debug_ & 0x100) { std::cout << "}\n"; }
  }
  std::sort(index_events_.begin(), index_events_.end());
}

void Player::SetAbsEvents() {
  const std::vector<midi::Track> &tracks = pm_.GetTracks();
  DynamicTiming dyn_timing{
    500000,
    1000ull * uint64_t{pm_.GetTicksPerQuarterNote()},
    0, 0};
  final_ms_ = 0;
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
  abs_events_.push_back(std::make_unique<FinalEvent>(
    final_ms_ + 1,
    abs_events_.empty() ? 0 : abs_events_.back()->time_ms_original_ + 1));
  if (pp_.debug_ & 0x4) {
    const size_t nae = abs_events_.size();
    std::cout << fmt::format("abs_events[{}]", nae) << "{\n";
    for (size_t i = 0; i < nae; ++i) {
      std::cout << fmt::format("  [{:4d}] {}\n", i, abs_events_[i]->str());
    }
    std::cout << fmt::format("abs_events[{}]", nae) << "{\n";
  }
}

void Player::play() {
  std::unique_lock lock(play_mtx_);
  if (pp_.debug_ & 0x2) { std::cout << "play: mutex locked\n"; }
  CallBackData cbd_periodic{CallBackData::CallBack::Periodic, this};
  seq_ids_[SeqIdPeriodic] = fluid_sequencer_register_client(
    ss_.sequencer_, "periodic", callback, &cbd_periodic);
  CallBackData cbd_final{CallBackData::CallBack::Final, this};
  seq_ids_[SeqIdFinal] = fluid_sequencer_register_client(
    ss_.sequencer_, "final", callback, &cbd_final);
  CallBackData cbd_progress{CallBackData::CallBack::Progress, this};
  if (pp_.progress_) {
    seq_ids_[SeqIdProgress] = fluid_sequencer_register_client(
      ss_.sequencer_, "progress", callback, &cbd_progress);
  }
  SchedulePeriodicAt(0);
  if (pp_.progress_) {
    ScheduleProgressAt(100);
  }
  if (pp_.debug_ & 0x2) { std::cout << "wait on lock\n"; }
  cv_.wait(lock, [this]{ return final_handled_; });
  if (pp_.progress_) { std::cout << '\n'; }
  if (pp_.debug_ & 0x2) { std::cout << "unlocked\n"; }
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
    ? FactorU32(pp_.tempo_div_factor_, date_ms - pp_.begin_ms_)
    : 0;
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
        uint32_t duration_modified = FactorU32(pp_.tempo_div_factor_, duration_ms);
        MaxBy(final_ms_, date_ms_modified + duration_modified);
        abs_events_.push_back(std::make_unique<NoteEvent>(
          date_ms_modified, date_ms,
          note_on->channel_, note_on->key_, note_on->velocity_,
          duration_modified, duration_ms));
      }
    }
    break;
   case midi::MidiVarByte::PROGRAM_CHANGE_x4: {
      const midi::ProgramChangeEvent* pc =
        dynamic_cast<const midi::ProgramChangeEvent*>(me);
      MaxBy(final_ms_, date_ms_modified);
      abs_events_.push_back(std::make_unique<ProgramChange>(
        date_ms_modified, date_ms, pc->channel_, pc->number_));
    }
    break;
   case midi::MidiVarByte::PITCH_WHEEL_x6: {
      const midi::PitchWheelEvent* pw =
        dynamic_cast<const midi::PitchWheelEvent*>(me);
      MaxBy(final_ms_, date_ms_modified);
      abs_events_.push_back(std::make_unique<PitchWheel>(
        date_ms_modified, date_ms, pw->channel_, pw->bend_));
    }
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

void Player::ScheduleCallback(int seq_id, uint32_t at) {
  fluid_event_t *e = new_fluid_event();
  fluid_event_set_source(e, -1);
  fluid_event_set_dest(e, seq_id);
  fluid_event_timer(e, nullptr);
  int send_rc = fluid_sequencer_send_at(ss_.sequencer_, e, at, 1);
  if (send_rc != FLUID_OK) {
    std::cerr << fmt::format("fluid_sequencer_send_at rc={}\n", send_rc);
  }
  delete_fluid_event(e);
}

void Player::callback(
    unsigned int time,
    fluid_event_t *event,
    fluid_sequencer_t *seq,
    void *data) {
  CallBackData *cbd = static_cast<CallBackData*>(data);
  switch (cbd->ecb_) {
   case CallBackData::CallBack::Periodic:
    cbd->player_->periodic_callback(time, event, seq);
    break;
   case CallBackData::CallBack::Final:
    cbd->player_->final_callback(time, event, seq);
    break;
   case CallBackData::CallBack::Progress:
    cbd->player_->progress_callback(time, event, seq);
    break;
   default:
    std::cerr << "BUG: callback ecb=" << static_cast<int>(cbd->ecb_) << '\n';
  }  
}

void Player::periodic_callback(
    unsigned int time,
    fluid_event_t *event,
    fluid_sequencer_t *seq) {
  const std::lock_guard<std::mutex> lock(sending_mtx_);
  const size_t nae = abs_events_.size();
  bool batch_done = false;
  uint32_t now = fluid_sequencer_get_tick(ss_.sequencer_);
  uint32_t time_limit = (next_send_index_ < nae)
    ? abs_events_[next_send_index_]->time_ms_ + pp_.batch_duration_ms_ : 0;
  for (; (next_send_index_ < nae) && !batch_done; ++next_send_index_) {
    if (next_send_index_ == 0) {
      date_add_ms_ = now + pp_.initial_delay_ms_;
      if (pp_.debug_ & 0x1) {
        std::cerr << fmt::format("date_add_ms_={}\n", date_add_ms_);
      }
    }
    AbsEvent *e = abs_events_[next_send_index_].get();
    uint32_t date_ms = e->time_ms_ + date_add_ms_;
    fluid_event_t *event = new_fluid_event();
    e->SetSendFluidEvent(event, this, date_ms);
    delete_fluid_event(event);
    batch_done = (e->time_ms_ >= time_limit);
  }
  if (next_send_index_ < nae) {
    SchedulePeriodicAt(now + pp_.batch_duration_ms_ / 2);
  }
}

void Player::final_callback(
    unsigned int time,
    fluid_event_t *event,
    fluid_sequencer_t *seq) {
  if (pp_.debug_ & 0x2) { std::cout << "final_callback\n"; } 
  final_handled_ = true;
  for (size_t seqii = 0; seqii < SeqId_N; ++seqii) {
    int seq_id = seq_ids_[seqii];
    fluid_sequencer_remove_events(ss_.sequencer_, -1, seq_id, -1);
    fluid_sequencer_unregister_client(ss_.sequencer_, seq_id);
  }
  if (pp_.debug_ & 0x2) { std::cout << "final_callback notify\n"; } 
  cv_.notify_one();
}

void Player::progress_callback(
    unsigned int time,
    fluid_event_t *event,
    fluid_sequencer_t *seq) {
  if ((next_send_index_ > 0) && time >= date_add_ms_) {
    const uint32_t last_ms = abs_events_.back()->time_ms_original_;
    uint32_t dt = time - date_add_ms_;
    float dt_div_f = dt / pp_.tempo_div_factor_; // save div in PlayParams ?
    uint32_t dt_div = static_cast<uint32_t>(dt_div_f);
    uint32_t btime = dt_div + pp_.begin_ms_;
    if ((date_add_ms_ <= btime) && (btime <= last_ms)) {
      uint32_t tdone = btime - date_add_ms_;
      auto mmss_done = milliseconds_to_string(tdone);
      auto mmss_final = milliseconds_to_string(last_ms);
      std::cout << fmt::format("\rProgress: {} / {}", mmss_done, mmss_final);
      std::cout.flush();
    }
  }
  // about event 1/10 second
  uint32_t tmod100 = time % 100;
  uint32_t time_next = time + (tmod100 > 50 ? 200 : 100) - tmod100;
  ScheduleProgressAt(time_next);
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

// Now that Player is defined, we can define Handle virtual methods
void NoteEvent::SetSendFluidEvent(
    fluid_event_t *event, const Player *player, uint32_t date_ms) {
  fluid_event_set_source(event, -1);
  fluid_event_set_dest(event, player->GetSeqId(Player::SeqIdSynth));
  fluid_event_note(event, channel_, key_, velocity_, duration_ms_);
  fluid_sequencer_send_at(
    player->GetSynthSequencer().sequencer_, event, date_ms, 1);
}

void ProgramChange::SetSendFluidEvent(
    fluid_event_t *event, const Player *player, uint32_t date_ms) {
  fluid_event_set_source(event, -1);
  fluid_event_set_dest(event, player->GetSeqId(Player::SeqIdSynth));
  fluid_event_program_change(event, channel_, program_);
  fluid_sequencer_send_at(
    player->GetSynthSequencer().sequencer_, event, date_ms, 1);
}

void PitchWheel::SetSendFluidEvent(
    fluid_event_t *event, const Player *player, uint32_t date_ms) {
  fluid_event_set_source(event, -1);
  fluid_event_set_dest(event, player->GetSeqId(Player::SeqIdSynth));
  fluid_event_pitch_bend(event, channel_, bend_);
  fluid_sequencer_send_at(
    player->GetSynthSequencer().sequencer_, event, date_ms, 1);
}

void FinalEvent::SetSendFluidEvent(
    fluid_event_t *event, const Player *player, uint32_t date_ms) {
  fluid_event_set_source(event, -1);
  fluid_event_set_dest(event, player->GetSeqId(Player::SeqIdFinal));
  fluid_sequencer_send_at(
    player->GetSynthSequencer().sequencer_, event, date_ms, 1);
}

////////////////////////////////////////////////////////////////////////

int play(
    const midi::Midi &parsed_midi,
    SynthSequencer &synth_sequencer,
    const PlayParams &play_params) {
  int rc = Player(parsed_midi, synth_sequencer, play_params).run();
  return rc;
}
