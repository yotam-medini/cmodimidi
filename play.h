// -*- c++ -*-
#pragma once

#include <cstdint>
#include "midi.h"

class PlayParams {
 public:
  uint32_t begin_ms_{0};
  uint32_t end_ms_{0};
  float tempo_factor_{1.0};
  uint32_t initial_delay_ms_{0};
  uint32_t batch_duration_ms_{0};
  bool progress_{false};
  uint32_t debug_{0};
};

class SynthSequencer;
extern int play(
  const midi::Midi &parsed_midi,
  SynthSequencer &synth_sequencer,
  const PlayParams &play_params);
