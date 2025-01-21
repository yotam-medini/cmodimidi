// -*- c++ -*-
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace midi {

enum MetaVarByte : uint8_t {
  SEQNUM_x00    = 0x00,
  TEXT_x01      = 0x01,
  COPYRIGHT_x02 = 0x02,
  TRACKNAME_x03 = 0x03,
  INSTRNAME_x04 = 0x04,
  LYRICS_x05    = 0x05,
  MARK_x06      = 0x06,
  DEVICE_x09    = 0x09,
  CHANPFX_x20   = 0x20,
  PORT_x21      = 0x21,
  ENDTRACK_x2f  = 0x2f,
  TEMPO_x51     = 0x51,
  SMPTE_x54     = 0x54,
  TIMESIGN_x58  = 0x58,
  KEYSIGN_x59   = 0x59,
  SEQUEMCER_x7f = 0x7f,
};

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
  virtual MetaVarByte VarByte() const = 0;
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
  virtual MetaVarByte VarByte() const { return MetaVarByte::SEQNUM_x00; }
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
  virtual MetaVarByte VarByte() const { return MetaVarByte::TEXT_x01; }
  std::string event_type_name() const { return "Text"; }
};

class CopyrightEvent : public TextBaseEvent { // 0xff 0x02
 public:
  CopyrightEvent(uint32_t dt, const std::string& s="") : TextBaseEvent{dt, s} {}
  virtual ~CopyrightEvent() {}
  virtual MetaVarByte VarByte() const { return MetaVarByte::COPYRIGHT_x02; }
  std::string event_type_name() const { return "Copyright"; }
};

class SequenceTrackNameEvent : public TextBaseEvent { // 0xff 0x03
 public:
  SequenceTrackNameEvent(uint32_t dt, const std::string& s="") :
    TextBaseEvent{dt, s} {}
  virtual ~SequenceTrackNameEvent() {}
  virtual MetaVarByte VarByte() const { return MetaVarByte::TRACKNAME_x03; }
  std::string event_type_name() const { return "SequenceTrackName"; }
};

class InstrumentNameEvent : public TextBaseEvent { // 0xff 0x04
 public:
  InstrumentNameEvent(uint32_t dt, const std::string& s="") :
    TextBaseEvent{dt, s} {}
  virtual ~InstrumentNameEvent() {}
  virtual MetaVarByte VarByte() const { return MetaVarByte::INSTRNAME_x04; }
  std::string event_type_name() const { return "InstrumentName"; }
};

class LyricEvent : public TextBaseEvent { // 0xff 0x05
 public:
  LyricEvent(uint32_t dt, const std::string& s="") : TextBaseEvent{dt, s} {}
  virtual ~LyricEvent() {}
  virtual MetaVarByte VarByte() const { return MetaVarByte::LYRICS_x05; }
  std::string event_type_name() const { return "Lyric"; }
};

class MarkerEvent : public TextBaseEvent { // 0xff 0x06
 public:
  MarkerEvent(unsigned dt, const std::string& s="") : TextBaseEvent{dt, s} {}
  virtual ~MarkerEvent() {}
  virtual MetaVarByte VarByte() const { return MetaVarByte::MARK_x06; }
  std::string event_type_name() const { return "Marker"; }
};

class DeviceEvent : public TextBaseEvent { // 0xff 0x09
 public:
  DeviceEvent(uint32_t dt, const std::string& s="") : TextBaseEvent{dt, s} {}
  virtual ~DeviceEvent() {}
  virtual MetaVarByte VarByte() const { return MetaVarByte::DEVICE_x09; }
  std::string event_type_name() const { return "Device"; }
};

class ChannelPrefixEvent : public MetaEvent { // 0xff 0x20
 public:
  ChannelPrefixEvent(uint32_t dt, uint8_t channel) :
    MetaEvent{dt}, channel_{channel} {}
  virtual ~ChannelPrefixEvent() {}
  virtual MetaVarByte VarByte() const { return MetaVarByte::CHANPFX_x20; }
  uint8_t channel_;
};

class PortEvent : public MetaEvent { // 0xff 0x21
 public:
  PortEvent(uint32_t dt, uint8_t port) : MetaEvent{dt}, port_{port} {}
  virtual ~PortEvent() {}
  virtual MetaVarByte VarByte() const { return MetaVarByte::PORT_x21; }
  uint8_t port_;
};

class EndOfTrackEvent : public MetaEvent { // 0xff 0x2f
 public:
  EndOfTrackEvent(uint32_t dt) : MetaEvent{dt}  {}
  virtual ~EndOfTrackEvent() {}
  virtual MetaVarByte VarByte() const { return MetaVarByte::ENDTRACK_x2f; }
};

class TempoEvent : public MetaEvent { // 0xff 0x51
 public:
  TempoEvent(uint32_t dt, uint32_t tttttt) : MetaEvent{dt}, tttttt_{tttttt}  {}
  virtual ~TempoEvent() {}
  virtual MetaVarByte VarByte() const { return MetaVarByte::TEMPO_x51; }
  uint32_t tttttt_;
};

class SmpteOffsetEvent : public MetaEvent { // 0xff 0x54
 public:
  SmpteOffsetEvent(
    uint32_t dt, 
    uint8_t hr,
    uint8_t mn,
    uint8_t se,
    uint8_t fr,
    uint8_t ff) :
     MetaEvent{dt}, hr_{hr}, mn_{mn}, se_{se}, fr_{fr}, ff_{ff} {}
  virtual ~SmpteOffsetEvent() {}
  virtual MetaVarByte VarByte() const { return MetaVarByte::SMPTE_x54; }
  uint8_t hr_;
  uint8_t mn_;
  uint8_t se_;
  uint8_t fr_;
  uint8_t ff_;
};

class TimeSignatureEvent : public MetaEvent { // 0xff 0x58
 public:
  TimeSignatureEvent(
    uint32_t dt,
    uint8_t nn,
    uint8_t dd,
    uint8_t cc,
    uint8_t bb) : 
    MetaEvent{dt}, nn_{nn}, dd_{dd}, cc_{cc}, bb_{bb} {}
  virtual ~TimeSignatureEvent() {}
  virtual MetaVarByte VarByte() const { return MetaVarByte::TIMESIGN_x58; }
  uint8_t nn_;
  uint8_t dd_;
  uint8_t cc_;
  uint8_t bb_;
};

class KeySignatureEvent : public MetaEvent { // 0xff 0x59
 public:
  KeySignatureEvent(uint32_t dt, uint16_t sf, bool mi) : 
    MetaEvent{dt}, sf_{sf}, mi_{mi} {}
  virtual ~KeySignatureEvent() {}
  virtual MetaVarByte VarByte() const { return MetaVarByte::KEYSIGN_x59; }
  uint16_t sf_;
  bool mi_;
};

class SequencerEvent : public MetaEvent { // 0xff 0x7f
 public:
  SequencerEvent(uint32_t dt, const std::vector<uint8_t> &data) : 
    MetaEvent{dt}, data_{data} {}
  virtual ~SequencerEvent() {}
  virtual MetaVarByte VarByte() const { return MetaVarByte::SEQUEMCER_x7f; }
  std::vector<uint8_t> data_;
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
