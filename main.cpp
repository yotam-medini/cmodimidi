#include <iostream>
#include <fmt/core.h>
#include <fluidsynth.h>
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
    std::cout << fmt::format("b={}, e={}\n",
      options.begin_millisec(), options.end_millisec());
    std::cout << fmt::format("mf={}\n", options.midifile_path());
    SynthSequencer synth_sequencer(options.soundfonts_path());
  }
  return rc;
}
