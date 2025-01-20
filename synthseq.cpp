#include "synthseq.h"
#include <fmt/core.h>
#include <fluidsynth.h>
// #include <fluidsynth/settings.h>

SynthSequencer::SynthSequencer(const std::string &sound_font_path) {
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
  if (synth_seq_id_ != -1) {
    fluid_sequencer_unregister_client(sequencer_, synth_seq_id_);
  }
  if (sequencer_) {
    delete_fluid_sequencer(sequencer_);
  }
  if (audio_driver_) {
    delete_fluid_audio_driver(audio_driver_);
  }
  if (sfont_id_ != -1) {
    fluid_synth_sfunload(synth_, sfont_id_, 0);
  }
  if (synth_) {
    delete_fluid_synth(synth_);
  }
  if (settings_) {
    delete_fluid_settings(settings_);
  }
}
