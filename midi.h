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

////////////////////////////////////////////////////////////////////////
// Meta Events 

class SequenceNumberEvent : public MetaEvent { // 0xff 0x01
 public:
  SequenceNumberEvent(uint16_t number) : number_{number_} {}
  uint16_t number_;
};

class TextBaseEvent : public MetaEvent {
 public:
  TextBaseEvent(const std::string& s="") : s_{s} {}
  virtual ~TextBaseEvent() {}
  virtual std::string str() const;
  virtual std::string event_type_name() const = 0;
  std::string s_;
};

class TextEvent : public TextBaseEvent { // 0xff 0x01
 public:
  TextEvent(const std::string& s="") : TextBaseEvent{s} {}
  virtual ~TextEvent() {}
  std::string event_type_name() const { return "Text"; }
};

class CopyrightEvent : public TextBaseEvent { // 0xff 0x02
 public:
  CopyrightEvent(const std::string& s="") : TextBaseEvent{s} {}
  virtual ~CopyrightEvent() {}
  std::string event_type_name() const { return "Copyright"; }
};

class SequenceTrackNameEvent : public TextBaseEvent { // 0xff 0x03
 public:
  SequenceTrackNameEvent(const std::string& s="") : TextBaseEvent{s} {}
  virtual ~SequenceTrackNameEvent() {}
  std::string event_type_name() const { return "SequenceTrackName"; }
};

class InstrumentNameEvent : public TextBaseEvent { // 0xff 0x04
 public:
  InstrumentNameEvent(const std::string& s="") : TextBaseEvent{s} {}
  virtual ~InstrumentNameEvent() {}
  std::string event_type_name() const { return "InstrumentName"; }
};

class LyricEvent : public TextBaseEvent { // 0xff 0x05
 public:
  LyricEvent(const std::string& s="") : TextBaseEvent{s} {}
  virtual ~LyricEvent() {}
  std::string event_type_name() const { return "Lyric"; }
};

class MarkerEvent : public TextBaseEvent { // 0xff 0x06
 public:
  MarkerEvent(const std::string& s="") : TextBaseEvent{s} {}
  virtual ~MarkerEvent() {}
  std::string event_type_name() const { return "Marker"; }
};

class DeviceEvent : public TextBaseEvent { // 0xff 0x09
 public:
  DeviceEvent(const std::string& s="") : TextBaseEvent{s} {}
  virtual ~DeviceEvent() {}
  std::string event_type_name() const { return "Device"; }
};

// End of Meta Events 
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// Midi Events 

// End of Midi Events 
////////////////////////////////////////////////////////////////////////


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
  uint16_t GetU16from(size_t from) const;
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
