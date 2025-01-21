#include <iostream>
#include <fmt/core.h>
#include <fluidsynth.h>
#include "midi.h"
#include "options.h"
#include "synthseq.h"

int main(int argc, char **argv) {
  int rc = 0;
  std::cout << fmt::format("Hello argc={}\n", argc);
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
      SynthSequencer synth_sequencer(options.soundfonts_path());
    }
  }
  return rc;
}
