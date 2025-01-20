#include "options.h"
#include <sstream>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

class _OptionsImpl {
 public:
  _OptionsImpl(int argc, char **argv) {
    AddOptions();
    po::store(po::parse_command_line(argc, argv, desc_), vm_);
    po::notify(vm_);
  }
  bool help() const { return vm_.count("help"); }
  std::string description() const {
    std::ostringstream oss;
    oss << desc_;
    return oss.str();
  }
 private:
  void AddOptions();
  po::options_description desc_;
  po::variables_map vm_;
};

void _OptionsImpl::AddOptions() {
  desc_.add_options()
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


Options::Options(int argc, char **argv) :
  p_{new _OptionsImpl(argc, argv)} {
}

Options::~Options() {
  delete p_;
}

bool Options::help() const {
  return p_->help();
}

std::string Options::description() const {
  return p_->description();
}
