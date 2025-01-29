#include <iostream>
#include <fmt/core.h>
#include <fluidsynth.h>
#include "dump.h"
#include "midi.h"
#include "options.h"
#include "play.h"
#include "synthseq.h"

int main(int argc, char **argv) {
  int rc = 0;
  Options options(argc, argv);
  if (options.Help()) {
    std::cout << options.Description();
  } else if (!options.Valid()) {
    std::cerr << options.Description();
    rc = 1;
  } else {
    uint32_t debug = options.Debug();
    if (debug) { 
      std::cout << fmt::format("debug=0x{:x}, b={}, e={}\n",
        debug, options.BeginMillisec(), options.EndMillisec());
      std::cout << fmt::format("mf={}\n", options.MidifilePath());
    }
    midi::Midi parsed_midi = midi::Midi(options.MidifilePath(), debug);
    if (!parsed_midi.Valid()) {
      std::cerr << fmt::format("Midi error: {}\n", parsed_midi.GetError());
      rc = 1;
    }
    if (rc == 0) {
      auto dump_path = options.DumpPath();
      if (!dump_path.empty()) {
        midi_dump(parsed_midi, dump_path);
      }
    }
    if (rc == 0) {
      if (options.Info()) {
        std::cout << parsed_midi.info();
      }
    }
    if ((rc == 0) && options.Play()) {
      SynthSequencer synth_sequencer(options.SoundfontsPath());
      if (synth_sequencer.ok()) {
        PlayParams pp;
        pp.begin_ms_ = options.BeginMillisec();
        pp.end_ms_ = options.EndMillisec();
        pp.tempo_div_factor_ = 1./options.Tempo();
        pp.tuning_ = options.Tuning();
        pp.initial_delay_ms_ = options.DelayMillisec();
        pp.batch_duration_ms_ = options.BatchDurationMillisec();
        pp.progress_ = options.Progress();
        pp.debug_ = debug;
        play(parsed_midi, synth_sequencer, pp);
      } else {
        std::cerr << fmt::format("Synth/Sequencer error: {}\n",
          synth_sequencer.error());
      }
    }
  }
  return rc;
}
