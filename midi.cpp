#include "midi.h"
#include <filesystem>
#include <iostream>
#include <fmt/core.h>

namespace fs = std::filesystem;

namespace midi {

Midi::Midi(const std::string &midifile_path, uint32_t debug) :
  debug_{debug} {
  GetData(midifile_path);
}

void Midi::GetData(const std::string &midifile_path) {
  if (fs::exists(midifile_path)) {
    auto size = fs::file_size(midifile_path);
    if (debug_ & 0x1) {
      std::cout << fmt::format("size({})={}\n", midifile_path, size);
    }
  } else {
    error_ = fmt::format("Does not exist: {}", midifile_path);
  }
}
  
}
