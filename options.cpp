#include "options.h"
#include <charconv>
#include <iostream>
#include <limits>
#include <sstream>
#include <fmt/core.h>
#include <boost/program_options.hpp>

static uint32_t MINUTE_MILLIES = 60000;
static uint32_t INFINITE_MINUTES_MILLIES = MINUTE_MILLIES *
  (std::numeric_limits<uint32_t>::max() / MINUTE_MILLIES);

namespace po = boost::program_options;

template <typename T>
static int StrToInt(const std::string &s, T defval) {
  T n;
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
    // last argument - the midi file
    pos_desc_.add("midifile", 1);
    po::store(po::command_line_parser(argc, argv)
        .options(desc_)
        .positional(pos_desc_)
        .run(),
      vm_);
    po::notify(vm_);
  }
  bool help() const { return vm_.count("help"); }
  std::string description() const {
    std::ostringstream oss;
    oss << desc_;
    return oss.str();
  }
  bool valid() const {
    bool v = vm_.count("midifile") > 0;
    if (!v) { std::cerr << "Missing midifile\n"; }
    for (const char *key: {"begin", "end", "delay", "batch-duration"}) {
      if (v) {
        v = vm_[key].as<OptionMilliSec>().valid_;
        if (!v) {
          std::cerr << fmt::format("Bad value for {}\n", key);
        }
      }
    }
    return v;
  }
  bool info() const { return vm_.count("info"); }
  std::string dump_path() const { return vm_["dump"].as<std::string>(); }
  bool play() const { return !(vm_["noplay"].as<bool>()); }
  bool progress() const { return vm_.count("progress"); }
  uint32_t begin_millisec() const { return GetMilli("begin"); }
  uint32_t end_millisec() const { return GetMilli("end"); }
  uint32_t delay_millisec() const { return GetMilli("delay"); }
  uint32_t batch_duration_millisec() const { return GetMilli("batch-duration"); }
  float tempo() const {
    static float tempo_min = 1./8.;
    static float tempo_max = 8;
    float v = vm_["tempo"].as<float>();
    if (v < tempo_min) {
      std::cerr << fmt::format("tempo increased from {} to {}\n", v, tempo_min);
    } else if (tempo_max < v) {
      std::cerr << fmt::format("tempo decreased from {} to {}\n", v, tempo_max);
    }
    return v;
  }
  uint32_t debug() const {
    auto raw = vm_["debug"].as<std::string>();
    uint32_t flags = std::stoi(raw, nullptr, 0);
    return flags;
  }
  std::string soundfonts_path() const {
    return vm_["soundfont"].as<std::string>();
  }
  std::string midifile_path() const {
    return vm_["midifile"].as<std::string>();
  }
 private:
  void AddOptions();
  uint32_t GetMilli(const char *key) const {
    return vm_[key].as<OptionMilliSec>().ms_;
  }
  po::options_description desc_;
  po::positional_options_description pos_desc_;
  po::variables_map vm_;
};

void _OptionsImpl::AddOptions() {
  desc_.add_options()
    ("help,h", "produce help message")
    ("midifile", po::value<std::string>(),
       "Positional argument. Path the midi file to be played")
    ("begin,b", 
      po::value<OptionMilliSec>()->default_value(OptionMilliSec{true, 0}),
      "start time [minutes]:seconds[.millisecs]")
    ("end,e",
       po::value<OptionMilliSec>()->default_value(
         OptionMilliSec{true, INFINITE_MINUTES_MILLIES}),
       "end time [minutes]:seconds[.millisecs]")
    ("delay", 
      po::value<OptionMilliSec>()->default_value(OptionMilliSec{true, 200}),
      "Initial extra playing delay in [minutes]:seconds[.millisecs]")
    ("batch-duration", 
      po::value<OptionMilliSec>()->default_value(OptionMilliSec{true, 10000}),
      "sequencer batch duration in [minutes]:seconds[.millisecs]")
    ("tempo,T",
       po::value<float>()->default_value(1.),
       "Speed Multiplier factor")
    ("soundfont,s",
       po::value<std::string>()->default_value(
         "/usr/share/sounds/sf2/FluidR3_GM.sf2"),
       "Path to sound fonts file")
    ("info", po::bool_switch()->default_value(false),
       "print general information of the midi file")
    ("dump", po::value<std::string>()->default_value(""),
       "Dump midi contents to file, '-' for stdout")
    ("noplay", po::bool_switch()->default_value(false), "Suppress playing")
    ("progress", po::bool_switch()->default_value(false), "show progress")
    ("debug", po::value<std::string>()->default_value("0"), "Debug flags")
  ;
}

// ========================================================================

Options::Options(int argc, char **argv) :
  p_{new _OptionsImpl(argc, argv)} {
}

Options::~Options() {
  delete p_;
}

std::string Options::description() const {
  return p_->description();
}

bool Options::valid() const {
  return p_->valid();
}

bool Options::help() const {
  return p_->help();
}

bool Options::info() const {
  return p_->info();
}

std::string Options::dump_path() const {
  return p_->dump_path();
}

bool Options::play() const {
  return p_->play();
}

bool Options::progress() const {
  return p_->progress();
}

uint32_t Options::begin_millisec() const {
  return p_->begin_millisec();
}

uint32_t Options::end_millisec() const {
  return p_->end_millisec();
}

uint32_t Options::delay_millisec() const {
  return p_->delay_millisec();
}

uint32_t Options::batch_duration_millisec() const {
  return p_->batch_duration_millisec();
}

float Options::tempo() const {
  return p_->tempo();
}

uint32_t Options::debug() const {
  return p_->debug();
}

std::string Options::midifile_path() const {
  return p_->midifile_path();
}

std::string Options::soundfonts_path() const {
  return p_->soundfonts_path();
}

