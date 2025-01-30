#include "synthseq.h"
#include <iostream>
#include <fmt/core.h>
#include <fluidsynth.h>
// #include <fluidsynth/settings.h>

SynthSequencer::SynthSequencer(
    const std::string &sound_font_path,
    uint32_t debug) : 
    debug_{debug} {
  settings_ = new_fluid_settings();
  int fs_rc;
  if (ok()) {
    fs_rc = fluid_settings_setint(settings_, "synth.reverb.active", 0);
    if (fs_rc != FLUID_OK) {
      error_ = fmt::format("setting reverb: failed rc={}", fs_rc);
    }
  }
  if (ok()) {
    fs_rc = fluid_settings_setint(settings_, "synth.chorus.active", 0);
    if (fs_rc != FLUID_OK) {
      error_ = fmt::format("setting chorus: failed rc={}", fs_rc);
    }
  }
  if (ok()) {
    synth_ = new_fluid_synth(settings_);
    sfont_id_ = fluid_synth_sfload(synth_, sound_font_path.c_str(), 1);
    if (sfont_id_ == FLUID_FAILED) {
      error_ = fmt::format("failed: sfload({})", sound_font_path);
    }
  }
  if (ok()) {
    audio_driver_ = new_fluid_audio_driver(settings_, synth_);
  }
  if (ok()) {
    sequencer_ = new_fluid_sequencer2(0);
    synth_seq_id_ = fluid_sequencer_register_fluidsynth(sequencer_, synth_);
  }
}

SynthSequencer::~SynthSequencer() {
  DeleteFluidObjects();
}

void SynthSequencer::DeleteFluidObjects() {
  if (synth_seq_id_ != -1) {
    if (debug_ & 0x1) { std::cerr<<"call fluid_sequencer_unregister_client\n"; }
    fluid_sequencer_unregister_client(sequencer_, synth_seq_id_);
    synth_seq_id_ = -1;
  }
  if (sequencer_) {
    if (debug_ & 0x1) { std::cerr << "call delete_fluid_sequencer\n"; }
    delete_fluid_sequencer(sequencer_);
    sequencer_ = nullptr;
  }
  if (audio_driver_) {
    if (debug_ & 0x1) { std::cerr << "call delete_fluid_audio_driver\n"; }
    delete_fluid_audio_driver(audio_driver_);
    audio_driver_ = nullptr;
  }
  if (sfont_id_ != -1) {
    if (debug_ & 0x1) { std::cerr << "call fluid_synth_sfunload\n"; }
    fluid_synth_sfunload(synth_, sfont_id_, 0);
    sfont_id_ = -1;
  }
  if (synth_) {
    if (debug_ & 0x1) { std::cerr << "call delete_fluid_synth\n"; }
    delete_fluid_synth(synth_);
    synth_ = nullptr;
  }
  if (settings_) {
    if (debug_ & 0x1) { std::cerr << "call delete_fluid_settings\n"; }
    delete_fluid_settings(settings_);
    settings_ = nullptr;
  }
}
