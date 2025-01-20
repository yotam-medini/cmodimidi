// -*- c++ -*-
#pragma once

#include <cstdint>
#include <fluidsynth/types.h>

class SynthSequencer {
 public:
  SynthSequencer();
  ~SynthSequencer();
  fluid_synth_t *settings_{nullptr};
  fluid_synth_t *synth_{nullptr};
  fluid_audio_driver_t *audio_driver_{nullptr};
  int16_t synth_seq_id_{-1};  
  int16_t sfont_id{-1};
};
