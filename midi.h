// -*- c++ -*-
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace midi {

class ParseState {
 public:
  size_t offset_{0};
  uint8_t last_status_{0};
  uint8_t last_channel{0};
};

class Midi {
 public:
  Midi(const std::string &path, uint32_t debug=0);
  std::string GetError() const { return error_; }
  bool Valid() const { return error_.empty(); }
 private:
  using vu8_t = std::vector<uint8_t>;
  Midi() = delete;
  Midi(const Midi&) = delete;
  void GetData(const std::string &path);
  void Parse();
  void ParseHeader();
  size_t GetNextSize();
  size_t GetVariableLengthQuantity();
  std::string GetString(size_t length);
  std::string GetChunkType() { return GetString(4); }
  std::string error_;
  vu8_t data_;
  ParseState parse_state_;

  uint16_t format_{0};
  uint16_t ntrks_{0};
  uint16_t division_{0};

  uint32_t ticks_per_quarter_note_{0};
  uint8_t negative_smpte_format_{0};
  uint16_t ticks_per_frame_{0};

  const uint32_t debug_;
};

}
