#include <iostream>
#include <fstream>
#include <regex>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <fmt/core.h>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

class TimeSignature {
 public:
  TimeSignature(
    uint32_t at=0,
    uint8_t nn=0,
    uint8_t dd=0,
    uint8_t cc=0,
    uint8_t bb=0) : abs_time_{at}, nn_{nn}, dd_{dd}, cc_{cc}, bb_{bb} {
  }
  float quarters() const {
    float wholes = float(nn_) / float(uint32_t(1) << dd_);
    float qs = 4 * wholes;
    return qs;
  }
  uint32_t abs_time_{0};
  uint8_t nn_{0};
  uint8_t dd_{0};
  uint8_t cc_{0};
  uint8_t bb_{0};
  uint32_t duration_ticks_{0};
};

static constexpr uint8_t REST_KEY = 0xff;

class NoteBase {
 public:
  NoteBase(uint32_t at=0, uint8_t channel=0, uint8_t key=0) :
    abs_time_{at}, channel_{channel}, key_{key} {
  }
  uint32_t abs_time_{0};
  uint8_t channel_{0};
  uint8_t key_{0};
};

class NoteOn : public NoteBase {
 public:
  NoteOn(uint32_t at=0, uint8_t channel=0, uint8_t key=0, uint8_t value=0) :
    NoteBase{at, channel, key}, value_{value} {
  }
  uint8_t value_{0};
};

class NoteOff : public NoteBase {
 public:
  NoteOff(uint32_t at=0, uint8_t channel=0, uint8_t key=0) :
    NoteBase{at, channel, key} {
  }
  uint8_t value_{0};
};

class Note : public NoteBase {
 public:
  Note(
    uint32_t at=0,
    uint8_t channel=0,
    uint8_t key=0, // 
    uint8_t value_=0,
    uint32_t et=0) :
    NoteBase{at, channel, key}, end_time_{et} {
  }
  std::string str() const {
    return fmt::format("Note([{}, {}], c={}, key={}, v={})",
      abs_time_, end_time_, channel_, key_, value_);
  }
  uint8_t value_{0};
  uint32_t end_time_;
};

class Track {
 public:
   std::string name_{""};
   std::vector<Note> notes_;
};

class ModiDump2Ly {
 public:
  ModiDump2Ly();
  void SetOptions();
  void SetArgs(int argc, char **argv);
  int Run();
  int RC() const { return rc_; }
 private:
  uint32_t Debug() const { return debug_; }
  void Help(std::ostream &os) const {
    os << desc_;
  }
  int Parse();
  bool GetTrack(std::istream &ifs);
  int rc_{0};
  po::options_description desc_; 
  po::variables_map vm_;
  std::string dump_filename_;
  std::string ly_filename_;
  std::string debug_raw_;
  uint32_t debug_{0};
  std::vector<TimeSignature> time_sigs_;
  std::vector<Track> tracks_;
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

int ModiDump2Ly::Run() {
  if (Debug() & 0x1) { std::cerr << "{Run\n"; }
  Parse();
  if (Debug() & 0x1) { std::cerr << "} end of Run\n"; }
  return RC();
}

int ModiDump2Ly::Parse() {
  if (Debug() & 0x1) { std::cerr << "{Parse\n"; }
  std::ifstream ifs(dump_filename_);
  if (ifs.fail()) {
    rc_ = 1;
    std::cerr << fmt::format("Failed to open {}\n", dump_filename_);
  } else {
    bool getting_tracks = true;
    while (getting_tracks && (RC() == 0)) {
      getting_tracks = GetTrack(ifs);
    }
  }
  if (Debug() & 0x1) { std::cerr << "} end of Parse\n"; }
  return RC();
}

bool ModiDump2Ly::GetTrack(std::istream &ifs) {
  bool got = false;
  for (bool skip = true; skip && not ifs.eof(); ) {
    std::string line;
    std::getline(ifs, line);
    // skip = not line.starts_with("Track");
    skip = (line.find("Track") != 0);
  }
  if (!ifs.eof()) {
    got = true;
    const std::regex base_regex(
      "^.*AT=(\\d++),.* "
      "TimeSignature\\(nn=(\\d+), dd=(\\d+), cc=(\\d+), bb=(\\d+)\\).*");
    Track track;
    std::string line;
    // while (!(line.starts_with("}") || ifs.eof())) {
    while (!((line.find("}") == 0) || ifs.eof())) {
      std::getline(ifs, line);
      std::smatch base_match;
      if (std::regex_match(line, base_match, base_regex)) {
        std::cout << "match_size=" << base_match.size() << '\n';
        if (base_match.size() == 6) {
          uint32_t abs_time = std::stoi(base_match[1].str());
          uint8_t nn = std::stoi(base_match[2].str());
          uint8_t dd = std::stoi(base_match[3].str());
          uint8_t cc = std::stoi(base_match[4].str());
          uint8_t bb = std::stoi(base_match[5].str());
          TimeSignature ts(abs_time, nn, dd, cc, bb);
          time_sigs_.push_back(std::move(ts));
        }
      }
    }
    tracks_.push_back(std::move(track));
  }         
  return got;
}

int main(int argc, char **argv) {
  std::cout << fmt::format("Hello {}\n", argv[0]);
  ModiDump2Ly modi_dump_2ly;
  modi_dump_2ly.SetArgs(argc, argv);
  int rc = modi_dump_2ly.RC();
  if (rc == 0) {
    rc = modi_dump_2ly.Run();
  }
  return rc;
}
