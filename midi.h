// -*- c++ -*-
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace midi {

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
  std::string error_;
  vu8_t data_;
  const uint32_t debug_;
};

}
