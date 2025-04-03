#include <iostream>
#include <fmt/core.h>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

class ModiDump2Ly {
 public:
  ModiDump2Ly();
  void SetOptions();
  void SetArgs(int argc, char **argv);
 private:
  uint32_t debug() const { return debug_; }
  po::options_description desc_; 
  po::variables_map vm_;
  uint32_t debug_{0};
};

ModiDump2Ly::ModiDump2Ly() {
  SetOptions();
}

void ModiDump2Ly::SetOptions() {
  desc_.add_options()
    ("help,h", "produce help message")
    ("input,i",
      po::value<std::string>(),
      "Dump produced by modimidi")
    ("output,o",
      po::value<std::string>(),
      "Output in Lilypond format")
    ("flat", po::bool_switch()->default_value(false),
      "Prefer flats♭ to default sharps♯")
    ("debug", po::value<std::string>()->default_value("0"),
      "Debug flags (hex ok)")
  ;
}

void ModiDump2Ly::SetArgs(int argc, char **argv) {
  po::store(po::command_line_parser(argc, argv)
      .options(desc_)
      .run(),
    vm_);
  po::notify(vm_);
  auto raw = vm_["debug"].as<std::string>();
  debug_ = std::stoi(raw, nullptr, 0);
}

int main(int argc, char **argv) {
  std::cout << fmt::format("Hello {}\n", argv[0]);
  ModiDump2Ly modi_dump_2ly;
  modi_dump_2ly.SetArgs(argc, argv);
  return 0;
}
