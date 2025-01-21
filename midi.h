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
  Event(uint32_t delta_time) : delta_time_{delta_time_} {}
  virtual ~Event() {}
  virtual std::string str() const { return ""; }
  uint32_t delta_time_;
};

class MetaEvent : public Event {
 public:
  MetaEvent(uint32_t dt) : Event{dt} {}
  virtual ~MetaEvent() {}
  virtual std::string str() const { return ""; }
};

class MidiEvent : public Event {
 public:
  MidiEvent(uint32_t dt)  : Event{dt} {}
  virtual ~MidiEvent() {}
  virtual std::string str() const { return ""; }
};

////////////////////////////////////////////////////////////////////////
// Meta Events 

class SequenceNumberEvent : public MetaEvent { // 0xff 0x01
 public:
  SequenceNumberEvent(uint32_t dt, uint16_t number) :
    MetaEvent{dt},
    number_{number_} {}
  uint16_t number_;
};

class TextBaseEvent : public MetaEvent {
 public:
  TextBaseEvent(uint32_t dt, const std::string& s="") : MetaEvent{dt}, s_{s} {}
  virtual ~TextBaseEvent() {}
  virtual std::string str() const;
  virtual std::string event_type_name() const = 0;
  std::string s_;
};

class TextEvent : public TextBaseEvent { // 0xff 0x01
 public:
  TextEvent(uint32_t dt, const std::string& s="") : TextBaseEvent{dt, s} {}
  virtual ~TextEvent() {}
  std::string event_type_name() const { return "Text"; }
};

class CopyrightEvent : public TextBaseEvent { // 0xff 0x02
 public:
  CopyrightEvent(uint32_t dt, const std::string& s="") : TextBaseEvent{dt, s} {}
  virtual ~CopyrightEvent() {}
  std::string event_type_name() const { return "Copyright"; }
};

class SequenceTrackNameEvent : public TextBaseEvent { // 0xff 0x03
 public:
  SequenceTrackNameEvent(uint32_t dt, const std::string& s="") :
    TextBaseEvent{dt, s} {}
  virtual ~SequenceTrackNameEvent() {}
  std::string event_type_name() const { return "SequenceTrackName"; }
};

class InstrumentNameEvent : public TextBaseEvent { // 0xff 0x04
 public:
  InstrumentNameEvent(uint32_t dt, const std::string& s="") :
    TextBaseEvent{dt, s} {}
  virtual ~InstrumentNameEvent() {}
  std::string event_type_name() const { return "InstrumentName"; }
};

class LyricEvent : public TextBaseEvent { // 0xff 0x05
 public:
  LyricEvent(uint32_t dt, const std::string& s="") : TextBaseEvent{dt, s} {}
  virtual ~LyricEvent() {}
  std::string event_type_name() const { return "Lyric"; }
};

class MarkerEvent : public TextBaseEvent { // 0xff 0x06
 public:
  MarkerEvent(unsigned dt, const std::string& s="") : TextBaseEvent{dt, s} {}
  virtual ~MarkerEvent() {}
  std::string event_type_name() const { return "Marker"; }
};

class DeviceEvent : public TextBaseEvent { // 0xff 0x09
 public:
  DeviceEvent(uint32_t dt, const std::string& s="") : TextBaseEvent{dt, s} {}
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
  std::unique_ptr<MetaEvent> GetMetaEvent(uint32_t delta_time);
  std::unique_ptr<MidiEvent> GetMidiEvent(uint32_t delta_time);
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
