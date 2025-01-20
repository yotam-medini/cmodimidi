#include <iostream>
#include <fmt/core.h>
#include <fluidsynth.h>
#include "options.h"

void InitSynth() {
  fluid_settings_t *settings = new_fluid_settings();
  fluid_synth_t *synth = new_fluid_synth(settings);
  // Load SoundFont
  int sf_id = fluid_synth_sfload(synth, "/usr/share/sounds/sf2/FluidR3_GM.sf2", 1);
  if (sf_id == FLUID_FAILED) {
      std::cerr << fmt::format("Failed to load SoundFont\n");
  }
  std::cout << fmt::format("sf_id={}\n", sf_id);  
}

int main(int argc, char **argv) {
  int rc = 0;
  std::cout << fmt::format("Hello argc={}\n", argc);
  Options options(argc, argv);
  if (options.help()) {
    std::cout << options.description();
  } else if (!options.valid()) {
    std::cerr << options.description();
  } else {
    std::cout << fmt::format("b={}, e={}\n",
      options.begin_millisec(), options.end_millisec());
    std::cout << fmt::format("mf={}\n", options.midifile_path());
    InitSynth();
  }
  return rc;
}
