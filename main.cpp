#include <iostream>
#include <fmt/core.h>
#include <boost/program_options.hpp>
#include <fluidsynth.h>

namespace po = boost::program_options;

void add_options(po::options_description &desc) {
  desc.add_options()
    ("help", "produce help message")
    ("debug", po::value<std::string>()->default_value("0"), "Debug flags")
  ;
}

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
  po::variables_map vm;
  po::options_description desc("modimidi options");
  add_options(desc);
  InitSynth();
  return rc;
}
