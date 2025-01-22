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
  
class Player {
 public:
  Player(const midi::Midi &pm, SynthSequencer &ss, const PlayParams &pp) :
    pm_{pm}, ss_{ss}, pp_{pp} {}
  int run();

 private:
  void SetIndexEvents();
  uint32_t GetFirstNoteTime();
  void SetAbsEvents();
  int rc_{0};

  const midi::Midi &pm_; // parsed_midi
  SynthSequencer &ss_;
  const PlayParams &pp_;

  std::vector<IndexEvent> index_events_;
};

int Player::run() {
  if (pp_.debug_ & 0x1) { std::cerr << "Player::run() begin\n"; }
  SetIndexEvents();
  if (pp_.debug_ & 0x1) { std::cerr << "Player::run() end\n"; }
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

int play(
    const midi::Midi &parsed_midi,
    SynthSequencer &synth_sequencer,
    const PlayParams &play_params) {
  int rc = Player(parsed_midi, synth_sequencer, play_params).run();
  return rc;
}
