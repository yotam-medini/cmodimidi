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

struct U8ToRange {
  U8ToRange(uint8_t key=0, uint8_t low=0xff, uint8_t high=0) :
    key_{key_}, range_{low, high} {}
  bool Valid() const { return range_[0] <= range_[1]; }
  uint8_t key_;
  std::array<uint8_t, 2> range_{0xff, 0};
};

std::istream& operator>>(std::istream& is, U8ToRange& u2r) {
  std::string s;
  is >> s;
  is >> s;
  u2r.key_ = 0;
  u2r.range_ = {0xff, 0};
  size_t colon = s.find(':');
  if (colon == std::string::npos) {
    std::cerr << fmt::format("U8ToRange missing colon in {}", s);
  } else {
    uint8_t u8;
    auto pec = std::from_chars(s.data(), s.data() + colon, u8);
    if (pec.ec != std::errc()) {
      std::cerr << fmt::format("U8ToRange: Bad key in {}\n", s);
    } else {
      u2r.key_ = u8;
      std::string tail = s.substr(colon + 1);
      size_t comma = tail.find(',');
      if (comma == std::string::npos) {
        pec = std::from_chars(tail.data(), tail.data() + tail.size(), u8);
        if (pec.ec != std::errc()) {
          std::cerr << fmt::format("U8ToRange: Bad range in {}\n", s);
        } else {
          u2r.range_ = {u8, u8};
        }
      } else {
        pec = std::from_chars(tail.data(), tail.data() + comma, u8);
        if (pec.ec != std::errc()) {
          std::cerr << fmt::format("U8ToRange: Bad low in {}\n", s);
        } else {
          u2r.range_[0] = u8;
          pec = std::from_chars(
            tail.data() + comma + 1, tail.data() + tail.size(), u8);
          if (pec.ec != std::errc()) {
            std::cerr << fmt::format("U8ToRange: Bad high in {}\n", s);
            u2r.range_[0] = 0xff;
          } else {
            u2r.range_[1] = u8;
          }
        }
      }
    }
  }
  return is;
}

std::ostream& operator<<(std::ostream& os, const U8ToRange& u2r) {
  os << fmt::format("{}{}:{},{}",
    (u2r.Valid() ? "" : "(Invalid)"), u2r.key_, u2r.range_[0], u2r.range_[1]);
  return os;
}

class _OptionsImpl {
 public:
  using k2range_t = Options::k2range_t;
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
  bool Help() const { return vm_.count("help"); }
  std::string Description() const {
    std::ostringstream oss;
    oss << desc_;
    return oss.str();
  }
  bool Valid() const {
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
  bool Info() const { return vm_["info"].as<bool>(); }
  std::string DumpPath() const { return vm_["dump"].as<std::string>(); }
  bool Play() const { return !(vm_["noplay"].as<bool>()); }
  bool Progress() const { return vm_["progress"].as<bool>(); }
  uint32_t BeginMillisec() const { return GetMilli("begin"); }
  uint32_t EndMillisec() const { return GetMilli("end"); }
  uint32_t DelayMillisec() const { return GetMilli("delay"); }
  uint32_t BatchDurationMillisec() const { return GetMilli("batch-duration"); }
  float Tempo() const {
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
  unsigned Tuning() const {
    static unsigned tuning_min = 300.;
    static unsigned tuning_max = 480;
    unsigned v = vm_["tuning"].as<unsigned>();
    if (v < tuning_min) {
      std::cerr << fmt::format("tuning increased from {} to {}\n",
        v, tuning_min);
    } else if (tuning_max < v) {
      std::cerr << fmt::format("tuning decreased from {} to {}\n",
        v, tuning_max);
    }
    return v;
  }
  k2range_t GetTracksVelocityMap() const {
    k2range_t k2vel;
    return k2vel;
  }
  k2range_t GetChannelsVelocityMap() const {
    k2range_t k2vel;
    return k2vel;
  }
  uint32_t Debug() const {
    auto raw = vm_["debug"].as<std::string>();
    uint32_t flags = std::stoi(raw, nullptr, 0);
    return flags;
  }
  std::string SoundfontsPath() const {
    return vm_["soundfont"].as<std::string>();
  }
  std::string MidifilePath() const {
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
    ("tuning",
       po::value<unsigned>()->default_value(440),
       "Tuning - frequency of A4 (central La)")
    ("tmap",
       po::value<std::vector<U8ToRange>>()->multitoken(),
       "Track velocity mappings <track>:<low>[,<high>]")
    ("cmap",
       po::value<std::vector<U8ToRange>>()->multitoken(),
       "Channel velocity mappings <track>:<low>[,<high>]")
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

std::string Options::Description() const {
  return p_->Description();
}

bool Options::Valid() const {
  return p_->Valid();
}

bool Options::Help() const {
  return p_->Help();
}

bool Options::Info() const {
  return p_->Info();
}

std::string Options::DumpPath() const {
  return p_->DumpPath();
}

bool Options::Play() const {
  return p_->Play();
}

bool Options::Progress() const {
  return p_->Progress();
}

uint32_t Options::BeginMillisec() const {
  return p_->BeginMillisec();
}

uint32_t Options::EndMillisec() const {
  return p_->EndMillisec();
}

uint32_t Options::DelayMillisec() const {
  return p_->DelayMillisec();
}

uint32_t Options::BatchDurationMillisec() const {
  return p_->BatchDurationMillisec();
}

float Options::Tempo() const {
  return p_->Tempo();
}

unsigned Options::Tuning() const {
  return p_->Tuning();
}

Options::k2range_t Options::GetTracksVelocityMap() const {
  return p_->GetTracksVelocityMap();
}

Options::k2range_t Options::GetChannelsVelocityMap() const {
  return p_->GetChannelsVelocityMap();
}

uint32_t Options::Debug() const {
  return p_->Debug();
}

std::string Options::MidifilePath() const {
  return p_->MidifilePath();
}

std::string Options::SoundfontsPath() const {
  return p_->SoundfontsPath();
}

