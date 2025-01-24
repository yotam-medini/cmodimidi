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
  if (options.help()) {
    std::cout << options.description();
  } else if (!options.valid()) {
    std::cerr << options.description();
    rc = 1;
  } else {
    uint32_t debug = options.debug();
    if (debug) { 
      std::cout << fmt::format("debug=0x{:x}, b={}, e={}\n",
        debug, options.begin_millisec(), options.end_millisec());
      std::cout << fmt::format("mf={}\n", options.midifile_path());
    }
    midi::Midi parsed_midi = midi::Midi(options.midifile_path(), debug);
    if (!parsed_midi.Valid()) {
      std::cerr << fmt::format("Midi error: {}\n", parsed_midi.GetError());
      rc = 1;
    }
    if (rc == 0) {
      auto dump_path = options.dump_path();
      if (!dump_path.empty()) {
        midi_dump(parsed_midi, dump_path);
      }
    }
    if ((rc == 0) && options.play()) {
      SynthSequencer synth_sequencer(options.soundfonts_path());
      if (synth_sequencer.ok()) {
        PlayParams pp;
        pp.begin_ms_ = options.begin_millisec();
        pp.end_ms_ = options.end_millisec();
        pp.tempo_div_factor_ = 1./options.tempo();
        pp.initial_delay_ms_ = options.delay_millisec();
        pp.batch_duration_ms_ = options.batch_duration_millisec();
        pp.progress_ = options.progress();
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
