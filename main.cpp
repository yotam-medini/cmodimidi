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
    midi::Midi parsed_midi = midi::Midi(options.midifile_path(), debug);
    std::cout << fmt::format("b={}, e={}\n",
      options.begin_millisec(), options.end_millisec());
    std::cout << fmt::format("mf={}\n", options.midifile_path());
    SynthSequencer synth_sequencer(options.soundfonts_path());
  }
  return rc;
}
