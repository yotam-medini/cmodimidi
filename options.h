// -*- c++ -*-
#pragma once

#include <cstdint>
#include <string>

class _OptionsImpl;

class Options {
 public:
  Options(int argc, char **argv);
  ~Options();
  std::string description() const;
  bool valid() const;
  bool help() const;
  bool info() const;
  std::string dump_path() const;
  bool play() const;
  bool progress() const;
  uint32_t begin_millisec() const;
  uint32_t end_millisec() const;
  uint32_t delay_millisec() const;
  uint32_t batch_duration_millisec() const;
  float tempo() const;
  unsigned tuning() const;
  uint32_t debug() const; // flags
  std::string midifile_path() const;
  std::string soundfonts_path() const;
 private:
  Options() = delete;
  class _OptionsImpl *p_{nullptr};
};
