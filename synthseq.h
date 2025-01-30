// -*- c++ -*-
#pragma once

#include <cstdint>
#include <string>
#include <fluidsynth/types.h>

class SynthSequencer {
 public:
  SynthSequencer(const std::string &sound_font_path, uint32_t debug);
  ~SynthSequencer();
  bool ok() const { return error_.empty(); }
  void DeleteFluidObjects();
  const std::string &error() const { return error_; }
  fluid_settings_t *settings_{nullptr};
  fluid_synth_t *synth_{nullptr};
  fluid_audio_driver_t *audio_driver_{nullptr};
  fluid_sequencer_t *sequencer_{nullptr};
  int16_t synth_seq_id_{-1};  
  int16_t sfont_id_{-1};
 private:
  std::string error_;
  const uint32_t debug_{0};
};
