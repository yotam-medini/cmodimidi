// -*- c++ -*-
#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>

class _OptionsImpl;

class Options {
 public:
  using range_t = std::array<uint8_t, 2>;
  using k2range_t = std::unordered_map<uint8_t, range_t>;
  Options(int argc, char **argv);
  ~Options();
  std::string Description() const;
  bool Valid() const;
  bool Help() const;
  bool Version() const;
  bool Info() const;
  std::string DumpPath() const;
  bool Play() const;
  bool Progress() const;
  uint32_t BeginMillisec() const;
  uint32_t EndMillisec() const;
  uint32_t DelayMillisec() const;
  uint32_t BatchDurationMillisec() const;
  float Tempo() const;
  int8_t KeyShift() const;
  unsigned Tuning() const;
  k2range_t GetTracksVelocityMap() const;
  k2range_t GetChannelsVelocityMap() const;
  uint32_t Debug() const; // flags
  std::string MidifilePath() const;
  std::string SoundfontsPath() const;
 private:
  Options() = delete;
  class _OptionsImpl *p_{nullptr};
};
