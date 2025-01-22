#include "play.h"
#include <iostream>
#include "synthseq.h"

class Player {
 public:
  Player(const midi::Midi &pm, SynthSequencer &ss, const PlayParams &pp) :
    pm_{pm}, ss_{ss}, pp_{pp} {}
  int run();
  int rc_{0};
  const midi::Midi &pm_; // parsed_midi
  SynthSequencer &ss_;
  const PlayParams &pp_;
};

int Player::run() {
  if (pp_.debug_ & 0x1) { std::cerr << "Player::run() begin\n"; }
  if (pp_.debug_ & 0x1) { std::cerr << "Player::run() end\n"; }
  return rc_;
}

int play(
    const midi::Midi &parsed_midi,
    SynthSequencer &synth_sequencer,
    const PlayParams &play_params) {
  int rc = Player(parsed_midi, synth_sequencer, play_params).run();
  return rc;
}
