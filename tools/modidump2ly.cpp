#include <iostream>
#include <fmt/core.h>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

class ModiDump2Ly {
 public:
  ModiDump2Ly();
  void SetOptions();
  void SetArgs(int argc, char **argv);
  int RC() const { return rc_; }
 private:
  uint32_t Debug() const { return debug_; }
  void Help(std::ostream &os) const {
    os << desc_;
  }
  int rc_{0};
  po::options_description desc_; 
  po::variables_map vm_;
  std::string dump_filename_;
  std::string ly_filename_;
  std::string debug_raw_;
  uint32_t debug_{0};
};

ModiDump2Ly::ModiDump2Ly() {
  SetOptions();
}

void ModiDump2Ly::SetOptions() {
  desc_.add_options()
    ("help,h", "produce help message")
    ("input,i",
      po::value<std::string>(&dump_filename_),
      "Dump produced by modimidi (required)")
    ("output,o",
      po::value<std::string>(&ly_filename_),
      "Output in Lilypond format (required)")
    ("flat", po::bool_switch()->default_value(false),
      "Prefer flats♭ to default sharps♯")
    ("debug", po::value<std::string>(&debug_raw_)->default_value("0"),
      "Debug flags (hex ok)")
  ;
}

void ModiDump2Ly::SetArgs(int argc, char **argv) {
  po::store(po::command_line_parser(argc, argv)
      .options(desc_)
      .run(),
    vm_);
  po::notify(vm_);
  if (dump_filename_.empty() || ly_filename_.empty()) {
    rc_ = 1;
    std::cerr << "Missing arguments (-i, -o)\n";
    Help(std::cerr);
  }
  debug_ = std::stoi(debug_raw_, nullptr, 0);
  if (vm_.count("help")) {
    Help(std::cout);
  }
}

int main(int argc, char **argv) {
  std::cout << fmt::format("Hello {}\n", argv[0]);
  ModiDump2Ly modi_dump_2ly;
  modi_dump_2ly.SetArgs(argc, argv);
  return 0;
}
