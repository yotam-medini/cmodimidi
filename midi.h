// -*- c++ -*-
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace midi {

class Midi {
 public:
   Midi(const std::string &path, uint32_t debug=0);
 private:
   Midi() = delete;
   Midi(const Midi&) = delete;
   const uint32_t debug_;
};

}
