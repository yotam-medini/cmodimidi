// -*- c++ -*-
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace midi {

class ParseState {
 public:
  size_t offset_{0};
  uint8_t last_status_{0};
  uint8_t last_channel{0};
};

class Event {
 public:
  Event() {}
  virtual ~Event() {}
  virtual std::string str() const { return ""; }
};

class MetaEvent : public Event {
 public:
  MetaEvent() {}
  virtual ~MetaEvent() {}
  virtual std::string str() const { return ""; }
};

class MidiEvent : public Event {
 public:
  MidiEvent() {}
  virtual ~MidiEvent() {}
  virtual std::string str() const { return ""; }
};

class TextBaseEvent : public MetaEvent {
 public:
  TextBaseEvent(const std::string& s="") : s_{s} {}
  virtual ~TextBaseEvent() {}
  virtual std::string str() const;
  virtual std::string event_type_name() const = 0;
  std::string s_;
};

class TextEvent : public TextBaseEvent {
 public:
  TextEvent(const std::string& s="") : TextBaseEvent{s} {}
  virtual ~TextEvent() {}
  std::string event_type_name() const { return "Text"; }
  std::string s_;
};

class Track {
 public:
  std::vector<std::unique_ptr<Event>> events_;
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
  void ReadOneTrack() { ReadTrack(); }
  void ReadTracks();
  void ReadTrack();
  std::unique_ptr<Event> GetTrackEvent();
  std::unique_ptr<MetaEvent> GetMetaEvent();
  std::unique_ptr<MidiEvent> GetMidiEvent();
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

  std::vector<Track> tracks_;

  const uint32_t debug_;
};

}
