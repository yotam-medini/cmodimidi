#include <iostream>
#include <fstream>
#include <regex>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <fmt/core.h>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

static const std::regex tpq_seg_regex(
  "^.*ticksPer\\(1/4\\)=(\\d+).*");
static const std::regex time_sig_regex(
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
    denom_ = 1;
    for ( ; dd > 0; --dd, denom_ *= 2) {
    }
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
  uint32_t Ticks(uint32_t ticks_per_quarter) const {
    uint32_t ret = (4 * uint32_t{nn_} * ticks_per_quarter + denom_/2) / denom_;
    return ret;
  }
  std::string ly_str() const {
    return fmt::format("{}/{}", unsigned{nn_}, unsigned{denom_});
  }
  uint32_t abs_time_{0};
  uint8_t nn_{0};
  uint8_t dd_{0};
  uint8_t cc_{0};
  uint8_t bb_{0};
  uint8_t denom_{4};
  uint32_t duration_ticks_{0};
};

static const TimeSignature time_signature_initial{0, 4, 2, 24, 8}; // 24=0x18

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
  bool SameTime(const Note &other) const {
    return (abs_time_ == other.abs_time_) && (end_time_ == other.end_time_);
  }
  std::string str() const {
    return fmt::format("Note([{}, {}], c={}, key={}, v={})",
      abs_time_, end_time_, channel_, key_, value_);
  }
  uint8_t value_{0};
  uint32_t end_time_;
};
using vnotes_t = std::vector<Note>;

class Track {
 public:
   std::string name_{""};
   vnotes_t notes_;
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
  void WriteLyNotes();
  void WriteTrackNotes(std::ofstream &f_ly, size_t ti);
  void WriteKeyDuration(
    std::ofstream &f_ly,
    const std::string &sym,
    uint32_t tbegin,
    const uint32_t tend,
    const bool use_ts);
  std::string GetChordSym(
    const vnotes_t &notes,
    size_t &ni,
    uint8_t &key_last) const;
  std::string GetSymKey(uint8_t key, uint8_t key_last) const;
  bool IsNextTimeSig(uint32_t t) const {
    return ((time_sig_idx + 1 < time_sigs_.size()) && 
      (time_sigs_[time_sig_idx + 1].abs_time_ <= t));
  }
  uint32_t WholeTicks() const { return 4*ticks_per_quarter_; }
  uint32_t TsTicks(const TimeSignature &ts) const {
    return ts.Ticks(ticks_per_quarter_);
  }
  int rc_{0};
  po::options_description desc_; 
  po::variables_map vm_;
  std::string dump_filename_;
  std::string ly_filename_;
  std::string debug_raw_;
  uint32_t debug_{0};
  bool flat_{false};
  uint32_t ticks_per_quarter_{48};
  uint32_t small_time_{0};
  std::vector<TimeSignature> time_sigs_;
  std::vector<Track> tracks_;
  size_t time_sig_idx{0};
  size_t curr_ts_bar_begin{0};
  size_t curr_bar{0}; // 0-based
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
    ("flat", po::bool_switch(&flat_)->default_value(false),
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
  if (debug_ & 0x1) { std::cerr << "{ Run\n"; }
  Parse();
  if (rc_ == 0) {
    WriteLyNotes();
  }
  if (debug_ & 0x1) { std::cerr << "} end of Run\n"; }
  return rc_;
}

int ModiDump2Ly::Parse() {
  if (debug_ & 0x1) { std::cerr << "{ Parse\n"; }
  std::ifstream ifs(dump_filename_);
  if (ifs.fail()) {
    rc_ = 1;
    std::cerr << fmt::format("Failed to open {}\n", dump_filename_);
  } else {
    bool getting_tracks = true;
    while (getting_tracks && (RC() == 0)) {
      getting_tracks = GetTrack(ifs);
    }
    if (debug_ & 0x2) {
      std::cout << fmt::format("ticksPerQuarter={}\n", ticks_per_quarter_);
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
  if (debug_ & 0x1) { std::cerr << "} end of Parse\n"; }
  return RC();
}

bool ModiDump2Ly::GetTrack(std::istream &ifs) {
  using key_note_ons_t = std::unordered_map<uint8_t, std::vector<NoteOn>>;
  key_note_ons_t note_ons;
  bool got = false;
  for (bool skip = true; skip && not ifs.eof(); ) {
    std::string line;
    std::getline(ifs, line);
    std::smatch base_match;
    if (std::regex_match(line, base_match, tpq_seg_regex)) {
      if (base_match.size() == 2) {
        ticks_per_quarter_ = std::stoi(base_match[1].str());
        small_time_ = ticks_per_quarter_ / 64;
      }
    }
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
      } else if (std::regex_match(line, base_match, time_sig_regex)) {
        if (base_match.size() == 6) {
          TimeSignature ts(base_match);
          if (time_sigs_.empty() && ts.abs_time_ > 0) {
            time_sigs_.push_back(time_signature_initial);
          }
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

void ModiDump2Ly::WriteLyNotes() {
  if (debug_ & 0x1) { std::cerr << "{ WriteLyNotes\n"; }
  std::ofstream f_ly(ly_filename_);
  if (f_ly.fail()) {
    std::cerr << fmt::format("Failed to open {}\n", ly_filename_);
    rc_ = 1;
  } else {
    for (size_t ti = 0; (rc_ == 0) && (ti < tracks_.size()); ++ti) {
      if (!tracks_[ti].notes_.empty()) {
        WriteTrackNotes(f_ly, ti);
      }
    }
  }
  if (debug_ & 0x1) { std::cerr << "} end of WriteLyNotes\n"; }
}

void ModiDump2Ly::WriteTrackNotes(std::ofstream &f_ly, size_t ti) {
  const Track &track = tracks_[ti];
  f_ly << fmt::format("\ntrack{}{} = {}\n", ti, track.name_, "{");
  time_sig_idx = 0;
  curr_ts_bar_begin = 0;
  f_ly << fmt::format("  \\time {}\n ", time_sigs_[0].ly_str());
  curr_bar = 0;
  uint32_t prev_note_end_time = 0;
  const size_t n_notes = track.notes_.size();
  uint8_t key_last = 0;
  for (size_t ni = 0; ni < n_notes; ++ni) {
    const Note &note = track.notes_[ni];
    uint32_t rest_time = note.abs_time_ - prev_note_end_time;
    if (rest_time > small_time_) {
      WriteKeyDuration(f_ly, "r", prev_note_end_time, note.abs_time_, true);
    }
    if (IsNextTimeSig(note.abs_time_)) {
      const TimeSignature &ts = time_sigs_[time_sig_idx];
      uint32_t dt = time_sigs_[time_sig_idx + 1].abs_time_ - ts.abs_time_;
      uint32_t n_bars = dt / TsTicks(ts);
      curr_bar = curr_ts_bar_begin + n_bars;
      ++time_sig_idx;
      curr_ts_bar_begin = curr_bar;
      f_ly << fmt::format("  % bar {}\n  \\time {}\n ",
        curr_bar + 1, time_sigs_[time_sig_idx].ly_str());
    }
    bool polyphony = false;
    const uint8_t key_base = note.key_;
    const std::string chord_sym = GetChordSym(track.notes_, ni, key_last);
    for ( ; (ni + 1 < n_notes) &&
      (track.notes_[ni + 1].abs_time_ < note.end_time_); ++ni) {
      if (!polyphony) {
        f_ly << "\n  % polyphony: ";
        const std::string key_sym = GetSymKey(note.key_, note.key_);
        WriteKeyDuration(f_ly, key_sym, note.abs_time_, note.end_time_, false);
        polyphony = true;
      }
    }
    if (polyphony) {
      f_ly << "\n ";
    }
    // const std::string key_sym = GetSymKey(note.key_, key_last);
    WriteKeyDuration(f_ly, chord_sym, note.abs_time_, note.end_time_, true);
    key_last = key_base;
    prev_note_end_time = note.end_time_;
  }
  f_ly << "\n}\n";
}

void ModiDump2Ly::WriteKeyDuration(
  std::ofstream &f_ly,
  const std::string &sym,
  uint32_t tbegin,
  const uint32_t tend,
  const bool use_ts) {
  while (tbegin + small_time_ < tend) {
    const TimeSignature &ts = time_sigs_[time_sig_idx];
    uint32_t et = tend;
    if (use_ts) {
      uint32_t curr_ts_bars = curr_bar - curr_ts_bar_begin;
      uint32_t end_of_bar = ts.abs_time_ + (curr_ts_bars + 1)*TsTicks(ts);
      if (tbegin + small_time_ >= end_of_bar) {
         ++curr_bar;
         f_ly << fmt::format("\n  % bar {}\n ", curr_bar + 1);
         tbegin += small_time_;
         if (IsNextTimeSig(tbegin)) {
           ++time_sig_idx;
           curr_ts_bar_begin = curr_bar;
           f_ly << fmt::format(" \\time {}\n ",
             time_sigs_[time_sig_idx].ly_str());
         }
         const TimeSignature &ts1 = time_sigs_[time_sig_idx];
         curr_ts_bars = curr_bar - curr_ts_bar_begin;
         end_of_bar = ts1.abs_time_ + (curr_ts_bars + 1)*TsTicks(ts1);
      }
      if (et > end_of_bar) {
        et = end_of_bar;
      }
    }
    uint32_t duration = et - tbegin;
    while (duration >= 2*WholeTicks()) {
      f_ly << fmt::format(" {}1", sym);
      duration -= WholeTicks();
    }
    if (duration >= 6 * ticks_per_quarter_) {
      f_ly << fmt::format(" {}1.", sym);
      duration -= 6 * ticks_per_quarter_;
    }
    if (duration >= WholeTicks()) {
      f_ly << fmt::format(" {}1", sym);
      duration -= WholeTicks();
    }
    if (duration >= 3*ticks_per_quarter_) {
      f_ly << fmt::format(" {}2.", sym);
      duration -= 3*ticks_per_quarter_;
    }
    if (duration >= 2*ticks_per_quarter_) {
      f_ly << fmt::format(" {}2", sym);
      duration -= 2*ticks_per_quarter_;
    }
    if (duration >= (3*ticks_per_quarter_)/2) {
      f_ly << fmt::format(" {}4.", sym);
      duration -= (3*ticks_per_quarter_)/2;
    }
    if (duration >= ticks_per_quarter_) {
      f_ly << fmt::format(" {}4", sym);
      duration -= ticks_per_quarter_;
    }
    if (duration >= ((3*ticks_per_quarter_)/4)) {
      f_ly << fmt::format(" {}8.", sym);
      duration -= (3*ticks_per_quarter_)/4;
    }
    if (duration >= (ticks_per_quarter_/2)) {
      f_ly << fmt::format(" {}8", sym);
      duration -= ticks_per_quarter_/2;
    }
    if (duration >= (ticks_per_quarter_/2)) {
      f_ly << fmt::format(" {}8", sym);
      duration -= ticks_per_quarter_/2;
    }
    if (duration >= (ticks_per_quarter_/4)) {
      f_ly << fmt::format(" {}16", sym);
      duration -= ticks_per_quarter_/4;
    }
    if (duration >= (ticks_per_quarter_/8)) {
      f_ly << fmt::format(" {}32", sym);
      duration -= ticks_per_quarter_/8;
    }
    tbegin = et;
  }
}

std::string ModiDump2Ly::GetChordSym(
  const vnotes_t &notes,
  size_t &ni,
  uint8_t &key_last) const {
  vnotes_t chord;
  chord.push_back(notes[ni]);
  while ((ni + 1 < notes.size()) && chord.front().SameTime(notes[ni + 1])) {
    ++ni;
    chord.push_back(notes[ni]);
  }
  std::sort(chord.begin(), chord.end(), [](const Note& n0, const Note &n1) {
    return n0.key_ < n1.key_; });
  uint8_t chord_key = notes[0].key_;
  std::string sym = GetSymKey(chord_key, key_last);
  key_last = chord_key;
  if (chord.size() > 1) {
    sym = fmt::format("<{}", sym);
    for (size_t i = 1; i < chord.size(); ++i) {
      uint8_t key_next = chord[i].key_;
      std::string sym_next = GetSymKey(key_next, chord_key);
      chord_key = key_next;
      sym = fmt::format("{} {}", sym, sym_next);
    }
    sym.push_back('>');
  }
  return sym;
}

std::string ModiDump2Ly::GetSymKey(uint8_t key, uint8_t key_last) const {
  using vs_t = std::vector<std::string>;
  using vi_t = std::vector<int>;
  static const vs_t syms_flat{
    "c", "df", "d", "ef", "e", "f", "gf", "g", "af", "a", "bf", "b"};
  static const vs_t syms_sharp{
    "c", "cs", "d", "ds", "e", "f", "fs", "g", "gs", "a", "as", "b"};
                                  // C     D     E  F     G     A     B
  static const vi_t syms_index_flat {0, 1, 1, 2, 2, 3, 4, 4, 5, 5, 6, 6};
  static const vi_t syms_index_sharp{0, 0, 1, 1, 2, 3, 3, 4, 4, 5, 5, 6};
  const vs_t &syms = (flat_ ? syms_flat : syms_sharp);
  const vi_t &syms_index = (flat_ ? syms_index_flat : syms_index_sharp);
  uint8_t key_mod12 = key % 12;
  uint8_t key_last_mod12 = key_last % 12;
  std::string sym{syms[key_mod12]};
  int sym_idx = syms_index[key_mod12];
  int sym_idx_last = syms_index[key_last_mod12];
  if (key_last < key) {
    if (((sym_idx + 7 - sym_idx_last) % 7) > 3) {
      sym.push_back('\'');
    }
  } else if (key < key_last) {
    if (((sym_idx_last + 7 - sym_idx) % 7) > 3) {
      sym.push_back(',');
    }
  }
  return sym;
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
