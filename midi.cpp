#include "midi.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <fmt/core.h>

namespace fs = std::filesystem;

namespace midi {

std::string TextBaseEvent::str() const {
  return fmt::format("{}({})", event_type_name(), s_);
}

Midi::Midi(const std::string &midifile_path, uint32_t debug) :
  debug_{debug} {
  GetData(midifile_path);
  if (Valid()) {
    Parse();
  }
}

void Midi::GetData(const std::string &midifile_path) {
  if (fs::exists(midifile_path)) {
    auto file_size = fs::file_size(midifile_path);
    if (debug_ & 0x1) {
      std::cout << fmt::format("size({})={}\n", midifile_path, file_size);
    }
    if (file_size < 0x20) {
      error_ = fmt::format("Midi file size={} too short", file_size);
    }
    if (Valid()) {
      data_.clear();
      data_.insert(data_.end(), file_size, 0);
      std::ifstream f(midifile_path, std::ios::binary);
      f.read(reinterpret_cast<char*>(data_.data()), file_size);
      if (f.fail()) {
        error_ = fmt::format("Failed to read {} bytes from {}",
          file_size, midifile_path);
      }
    }
  } else {
    error_ = fmt::format("Does not exist: {}", midifile_path);
  }
}

void Midi::Parse() {
  ParseHeader();
  if (Valid()) {
    switch (format_) {
     case 0:
       ReadOneTrack();
       break;
     case 1:
       ReadTracks();
       break;
     default:
       error_ = fmt::format("Unsupported format={}", format_);
    }
  }
}

void Midi::ParseHeader() {
  static const std::string MThd{"MThd"};
  const std::string header = GetChunkType();
  if (header != MThd) {
    error_ = fmt::format("header: {} != {}", header, MThd);
  }
  if (Valid()) {
     size_t length = GetNextSize();
     if (length != 6) {
       std::cerr << fmt::format("Unexpected length: {} != 6\n", length);
     }
     format_ = GetU16from(8);
     ntrks_ = GetU16from(10);
     division_ = GetU16from(12);
     if (debug_ & 0x1) {
       std::cout << fmt::format("length={}, format={}, ntrks={}, "
         "division={:018b}\n",
         length, format_, ntrks_, division_);
     }
     uint16_t bit15 = division_ >> 15;
     if (bit15 == 0) {
       ticks_per_quarter_note_ = division_;
     } else {
       negative_smpte_format_ = data_[12] & 0x7f;
       ticks_per_frame_ = data_[13];
       // hack
       ticks_per_quarter_note_ =
         (0x100 - uint16_t{data_[12]}) * uint16_t{data_[13]};
     }
     parse_state_.offset_ += length;
     if (debug_ & 0x1) {
       std::cout << fmt::format(
         "ticks_per_quarter_note={} ticks_per_frame={}\n",
         ticks_per_quarter_note_, ticks_per_frame_);
     }
  }
}

void Midi::ReadTracks() {
  for (uint16_t itrack = 0; Valid() && (itrack < ntrks_); ++itrack) {
    ReadTrack();
  }
}

void Midi::ReadTrack() {
  static const std::string MTrk{"MTrk"};
  const std::string chunk_type = GetChunkType();
  if (chunk_type != MTrk) {
    error_ = fmt::format("chunk_type={} != {} @ offset={}",
      chunk_type, MTrk, parse_state_.offset_ - 4);
  }
  const size_t length = GetNextSize();
  const size_t offset_eot = parse_state_.offset_ + length;
  tracks_.push_back(Track());
  auto &events = tracks_.back().events_;
  bool got_eot = false;
  while ((!got_eot) && (parse_state_.offset_ < offset_eot)) {
    auto event = GetTrackEvent();
    events.push_back(std::move(event));
  }
}

std::unique_ptr<Event> Midi::GetTrackEvent() {
  uint32_t delta_time = GetVariableLengthQuantity();
  uint8_t event_first_byte = data_[parse_state_.offset_++];
  std::unique_ptr<Event> e;
  switch (event_first_byte) {
   case 0xff:
    e = GetMetaEvent(delta_time);
    break;
   case 0xf0:
   case 0xf7:
     std::cerr << "Sysex Event ignored\n";
    break;
   default:
    e = GetMidiEvent(delta_time);
  }
  return e;
}

std::unique_ptr<MetaEvent> Midi::GetMetaEvent(uint32_t delta_time) {
  std::unique_ptr<MetaEvent> e;
  uint32_t length;
  std::string text;
  uint8_t meta_first_byte = data_[parse_state_.offset_++];
  switch (meta_first_byte) {
   case MetaVarByte::SEQNUM_x00:
    length = data_[parse_state_.offset_++];
    if (length != 2) {
      std::cerr << fmt::format("Unexpected length={}!=2 in SequenceNumber",
        length);
    }
    {
      uint16_t number = GetU16from(parse_state_.offset_);
      e = std::make_unique<SequenceNumberEvent>(delta_time, number);
    }
    parse_state_.offset_ += length;
   break;
   case MetaVarByte::TEXT_x01:
   case MetaVarByte::COPYRIGHT_x02:
   case MetaVarByte::TRACKNAME_x03:
   case MetaVarByte::INSTRNAME_x04:
   case MetaVarByte::LYRICS_x05:
   case MetaVarByte::MARK_x06:
   case MetaVarByte::DEVICE_x09:
    e = GetTextBaseEvent(delta_time, meta_first_byte);
    break;
   default:
    error_ = fmt::format("Meta event unsupported byte={:02x}", meta_first_byte);
  }
  return e;
}

std::unique_ptr<MidiEvent> Midi::GetMidiEvent(uint32_t delta_time) {
  std::unique_ptr<MidiEvent> e;
  return e;
}

std::unique_ptr<TextBaseEvent> Midi::GetTextBaseEvent(
    uint32_t delta_time,
    uint8_t meta_first_byte) {
  std::unique_ptr<TextBaseEvent> e;
  uint32_t length = GetVariableLengthQuantity();
  auto text = GetString(length);
  switch (meta_first_byte) {
   case MetaVarByte::TEXT_x01:
    e = std::make_unique<TextEvent>(delta_time, text);
    break;
   case MetaVarByte::COPYRIGHT_x02:
    e = std::make_unique<CopyrightEvent>(delta_time, text);
    break;
   case MetaVarByte::TRACKNAME_x03:
    e = std::make_unique<SequenceTrackNameEvent>(delta_time, text);
    break;
   case MetaVarByte::INSTRNAME_x04:
    e = std::make_unique<InstrumentNameEvent>(delta_time, text);
    break;
   case MetaVarByte::LYRICS_x05:
    e = std::make_unique<LyricEvent>(delta_time, text);
    break;
   case MetaVarByte::MARK_x06:
    e = std::make_unique<MarkerEvent>(delta_time, text);
    break;
   case MetaVarByte::DEVICE_x09:
    e = std::make_unique<DeviceEvent>(delta_time, text);
    break;
   default:
    error_ = fmt::format("BUG meta_first_byte={:0x2}", meta_first_byte);
  }
  return e;
}

size_t Midi::GetNextSize() {
  const size_t offs = parse_state_.offset_;
  size_t sz = 0;
  for (size_t i = 0; i < 4; ++i) {
    size_t b{data_[offs + i]};
    sz = (sz << 8) | b;
  }
  parse_state_.offset_ += 4;
  return sz;
}

size_t Midi::GetVariableLengthQuantity() {
  size_t quantity = 0;
  size_t offs = parse_state_.offset_;
  const size_t ofss_limit = offs + 4;
  bool done = false;
  while ((offs < ofss_limit) && !done) {
    size_t b = data_[offs++];
    quantity = (quantity << 7) + (b & 0x7f);
    done = (b & 0x80) == 0;
  }
  parse_state_.offset_ = offs;
  return quantity;
}

uint16_t Midi::GetU16from(size_t from) const {
  uint16_t ret = (uint16_t{data_[from]} << 8) | uint16_t{data_[from + 1]};
  return ret;
}

std::string Midi::GetString(size_t length) {
  const size_t offs = parse_state_.offset_;
  std::string s{&data_[offs], &data_[offs + length]};
  parse_state_.offset_ += length;
  return s;
}

}
