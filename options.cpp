#include "options.h"
#include <charconv>
#include <limits>
#include <sstream>
#include <boost/program_options.hpp>

static uint32_t MINUTE_MILLIES = 60000;
static uint32_t INFINITE_MINUTES_MILLIES = MINUTE_MILLIES *
  (std::numeric_limits<uint32_t>::max() / MINUTE_MILLIES);

namespace po = boost::program_options;

static int StrToInt(const std::string &s, int defval) {
  int n;
  auto [_, ec] = std::from_chars(s.data(), s.data() + s.size(), n);
  if (ec != std::errc()) {
    n = defval;
  }
  return n;
}

struct OptionMilliSec {
  OptionMilliSec(bool valid=false, uint32_t ms=0) : valid_{valid}, ms_{ms} {}
  bool valid_{false};
  uint32_t ms_{0};
};

std::istream& operator>>(std::istream& is, OptionMilliSec& opt) {
  opt.valid_ = false;
  std::string s;
  is >> s;
  if (s.empty()) {
    opt.ms_ = INFINITE_MINUTES_MILLIES;
  } else {
    int minutes = 0;
    int seconds = 0;
    int millis = 0;
    auto colon = s.find(':');
    if (colon == std::string::npos) {
      seconds = StrToInt(s, -1);
    } else {
      minutes = StrToInt(s.substr(0, colon), -1);
      auto tail = s.substr(colon + 1);
      auto dot = tail.find('.');
      if (dot == std::string::npos) {
        seconds = StrToInt(tail, -1);
      } else {
        seconds = StrToInt(tail.substr(0, dot), -1);
        millis = StrToInt(tail.substr(dot + 1), -1);
      }
    }
    if ((minutes != -1) && (seconds != -1) && (millis != -1)) {
      opt.valid_ = true;
      opt.ms_ = 1000*(60*minutes + seconds) + millis;
    }
  }
  return is;
}

std::ostream& operator<<(std::ostream& os, const OptionMilliSec& opt) {
  if (opt.valid_) {
    uint32_t millis = opt.ms_ % 1000;
    uint32_t seconds = opt.ms_ / 1000;
    uint32_t minutes = seconds / 60;
    seconds %= 60;
    if (minutes > 0) {
      os << minutes << ':';
    }
    if ((minutes > 0) && (seconds < 10)) {
      os << '0';
    } 
    os << seconds;
    if (millis > 0) {
      auto smillis = std::to_string(millis);
      size_t nz = 3 - smillis.size();
      os << '.' << std::string(nz, '0') << smillis;
    }
  } else {
    os << "invalid OptionMilliSec";
  }
  return os;
}

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
      po::value<OptionMilliSec>()->default_value(OptionMilliSec{true, 0}),
      "start time [minutes]:seconds[.millisecs]")
    ("end,e",
       po::value<OptionMilliSec>()->default_value(
         OptionMilliSec{true, INFINITE_MINUTES_MILLIES}),
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
