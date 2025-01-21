#include "midi.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <fmt/core.h>

namespace fs = std::filesystem;

namespace midi {

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
  static const std::string MThd{"MThd"};
  const std::string header = GetChunkType();
  if (header != MThd) {
    error_ = fmt::format("header: {} != {}", header, MThd);
  }
  if (Valid()) {
     size_t length = GetNextSize();
     format_ = (uint16_t{data_[8]} << 8) | uint16_t{data_[9]};
     ntrks_ = (uint16_t{data_[10]} << 8) | uint16_t{data_[11]};
     if (debug_ & 0x1) {
       std::cout << fmt::format("length={}, format={}, ntrks={}\n",
         length, format_, ntrks_);
     }
  }
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

std::string Midi::GetString(size_t length) {
  const size_t offs = parse_state_.offset_;
  std::string s{&data_[offs], &data_[offs + length]};
  parse_state_.offset_ += length;
  return s;
}

}
