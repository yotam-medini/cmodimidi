#include <iostream>
#include <fstream>
#include <regex>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <fmt/core.h>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

static const std::regex time_seg_regex(
  "^.*AT=(\\d++),.* "
  "TimeSignature\\(nn=(\\d+), dd=(\\d+), cc=(\\d+), bb=(\\d+)\\).*");
static const std::regex note_on_off_regex(
  "^.*AT=(\\d++),.* "
  "Note([Ofn]+)\\(channel=(\\d+), key=(\\d+), velocity=(\\d+)\\).*");
static const std::regex track_name_regex(
  "^.*AT=.* SequenceTrackName\\((\\w+)\\).*");

class TimeSignature {
 public:
  TimeSignature(
    uint32_t at=0,
    uint8_t nn=0,
    uint8_t dd=0,
    uint8_t cc=0,
    uint8_t bb=0) : abs_time_{at}, nn_{nn}, dd_{dd}, cc_{cc}, bb_{bb} {
  }
  TimeSignature(const std::smatch &base_match) :
    TimeSignature(
     std::stoi(base_match[1].str()),
     std::stoi(base_match[2].str()),
     std::stoi(base_match[3].str()),
     std::stoi(base_match[4].str()),
     std::stoi(base_match[5].str())) {
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
  NoteOn(const std::smatch &base_match) :
    NoteOn(
      std::stoi(base_match[1].str()),
      std::stoi(base_match[3].str()),
      std::stoi(base_match[4].str()),
      std::stoi(base_match[5].str())) {
  }
  uint8_t value_{0};
};

class NoteOff : public NoteBase {
 public:
  NoteOff(uint32_t at=0, uint8_t channel=0, uint8_t key=0) :
    NoteBase{at, channel, key} {
  }
  NoteOff(const std::smatch &base_match) :
    NoteOff(
      std::stoi(base_match[1].str()),
      std::stoi(base_match[3].str()),
      std::stoi(base_match[4].str())) {
  }
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
  uint32_t Duration() const { return end_time_ - abs_time_; }
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
    if (Debug() & 0x2) {
      std::cout << fmt::format("#(TimeSignature)={}\n", time_sigs_.size());
      std::cout << fmt::format("#(tracks)={}: [\n", tracks_.size());
      for (size_t i = 0; i < tracks_.size(); ++i) {
        const Track &track = tracks_[i];
        std::cout << fmt::format("  [{}] name={}, #(notes)={}\n",
          i, track.name_, track.notes_.size());
      }
      std::cout << "]\n";
    }
    ifs.close();
  }
  if (Debug() & 0x1) { std::cerr << "} end of Parse\n"; }
  return RC();
}

bool ModiDump2Ly::GetTrack(std::istream &ifs) {
  using key_note_ons_t = std::unordered_map<uint8_t, std::vector<NoteOn>>;
  key_note_ons_t note_ons;
  bool got = false;
  for (bool skip = true; skip && not ifs.eof(); ) {
    std::string line;
    std::getline(ifs, line);
    // skip = not line.starts_with("Track");
    skip = (line.find("Track") != 0);
  }
  if (!ifs.eof()) {
    got = true;
    Track track;
    std::string line;
    while (!((line.find("}") == 0) || ifs.eof())) {
      std::getline(ifs, line);
      std::smatch base_match;
      if (std::regex_match(line, base_match, track_name_regex)) {
        if (base_match.size() == 2) {
          track.name_ = base_match[1].str();
        }
      } else if (std::regex_match(line, base_match, time_seg_regex)) {
        if (base_match.size() == 6) {
          TimeSignature ts(base_match);
          time_sigs_.push_back(std::move(ts));
        }
      } else if (std::regex_match(line, base_match, note_on_off_regex)) {
        if (base_match.size() == 6) {
          uint32_t abs_time = std::stoi(base_match[1].str());
          std::string on_off = base_match[2].str();
          uint8_t key = std::stoi(base_match[4].str());
          uint8_t velocity = std::stoi(base_match[5].str());
          if ((on_off == std::string{"On"}) && (velocity > 0)) {
            NoteOn note_on{base_match};
            auto iter = note_ons.find(key);
            if (iter == note_ons.end()) {
              iter = note_ons.insert(iter, {key, std::vector<NoteOn>()});
            }
            iter->second.push_back(note_on);
          } else {
            NoteOff note_off{base_match};
            auto iter = note_ons.find(key);
            if ((iter == note_ons.end()) || iter->second.empty()) {
              std::cerr << fmt::format("Unmatched NoteOff in {}\n", line);
            } else {
              const NoteOn &note_on = iter->second.back();
              Note note{note_on.abs_time_, note_on.channel_, note_on.key_,
                note_on.value_, note_off.abs_time_};
              track.notes_.push_back(note);
              iter->second.pop_back();
            }
          }
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
