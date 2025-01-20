#include <iostream>
#include <fmt/core.h>
#include <boost/program_options.hpp>
#include <fluidsynth.h>

namespace po = boost::program_options;

void add_options(po::options_description &desc) {
  desc.add_options()
    ("help,h", "produce help message")
    ("begin,b", 
      po::value<std::string>()->default_value("0:00"),
      "start time [minutes]:seconds[.millisecs]")
    ("end,e",
       po::value<std::string>()->default_value(""),
       "end time [minutes]:seconds[.millisecs]")
    ("tempo,T",
       po::value<float>()->default_value(1.),
       "Speed Multiplier factor")
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
  po::options_description desc("modimidi options");
  add_options(desc);
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);
  if (vm.count("help")) {
    std::cout << desc;
  } else {
    InitSynth();
  }
  return rc;
}
