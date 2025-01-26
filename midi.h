// -*- c++ -*-
#pragma once

#include <array>
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

enum MidiVarByte : uint8_t {
  NOTE_OFF_x0         = 0x0,
  NOTE_ON_x1          = 0x1,
  KEY_PRESSURE_x2     = 0x2,
  CONTROL_CHANGE_x3   = 0x3,
  PROGRAM_CHANGE_x4   = 0x4,
  CHANNEL_PRESSURE_x5 = 0x5,
  PITCH_WHEEL_x6      = 0x6,
};

class ParseState {
 public:
  size_t offset_{0};
  uint8_t last_status_{0};
  uint8_t last_channel_{0};
};

class Event {
 public:
  Event(uint32_t delta_time) : delta_time_{delta_time} {}
  virtual ~Event() {}
  std::string dt_str() const;
  virtual std::string str() const = 0;
  uint32_t delta_time_;
};

class MetaEvent : public Event {
 public:
  MetaEvent(uint32_t dt) : Event{dt} {}
  virtual ~MetaEvent() {}
  virtual MetaVarByte VarByte() const = 0;
};

class MidiEvent : public Event {
 public:
  MidiEvent(uint32_t dt) : Event{dt} {}
  virtual ~MidiEvent() {}
  virtual MidiVarByte VarByte() const = 0;
};

////////////////////////////////////////////////////////////////////////
// Meta Events

class SequenceNumberEvent : public MetaEvent { // 0xff 0x01
 public:
  SequenceNumberEvent(uint32_t dt, uint16_t number) :
    MetaEvent{dt},
    number_{number_} {}
  virtual MetaVarByte VarByte() const { return MetaVarByte::SEQNUM_x00; }
  std::string str() const;
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
  std::string str() const;
  uint8_t channel_;
};

class PortEvent : public MetaEvent { // 0xff 0x21
 public:
  PortEvent(uint32_t dt, uint8_t port) : MetaEvent{dt}, port_{port} {}
  virtual ~PortEvent() {}
  virtual MetaVarByte VarByte() const { return MetaVarByte::PORT_x21; }
  std::string str() const;
  uint8_t port_;
};

class EndOfTrackEvent : public MetaEvent { // 0xff 0x2f
 public:
  EndOfTrackEvent(uint32_t dt) : MetaEvent{dt}  {}
  virtual ~EndOfTrackEvent() {}
  virtual MetaVarByte VarByte() const { return MetaVarByte::ENDTRACK_x2f; }
  std::string str() const;
};

class TempoEvent : public MetaEvent { // 0xff 0x51
 public:
  TempoEvent(uint32_t dt, uint32_t tttttt) : MetaEvent{dt}, tttttt_{tttttt}  {}
  virtual ~TempoEvent() {}
  virtual MetaVarByte VarByte() const { return MetaVarByte::TEMPO_x51; }
  std::string str() const;
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
  std::string str() const;
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
  std::string str() const;
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
  std::string str() const;
  uint16_t sf_;
  bool mi_;
};

class SequencerEvent : public MetaEvent { // 0xff 0x7f
 public:
  SequencerEvent(uint32_t dt, const std::vector<uint8_t> &data) :
    MetaEvent{dt}, data_{data} {}
  virtual ~SequencerEvent() {}
  virtual MetaVarByte VarByte() const { return MetaVarByte::SEQUEMCER_x7f; }
  std::string str() const;
  std::vector<uint8_t> data_;
};

// End of Meta Events
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// Midi Events

class NoteOffEvent : public MidiEvent {
 public:
  NoteOffEvent(
    unsigned dt,
    uint8_t channel,
    uint8_t key,
    uint8_t velocity) :
      MidiEvent(dt), channel_{channel}, key_{key}, velocity_{velocity} {}
  virtual ~NoteOffEvent() {}
  virtual MidiVarByte VarByte() const { return MidiVarByte::NOTE_OFF_x0; }
  virtual std::string str() const;
  uint8_t channel_{0};
  uint8_t key_{0};
  uint8_t velocity_{0}; // Should be zero
};

class NoteOnEvent : public MidiEvent {
 public:
  NoteOnEvent(
    unsigned dt,
    uint8_t channel,
    uint8_t key,
    uint8_t velocity) :
      MidiEvent(dt), channel_{channel}, key_{key}, velocity_{velocity} {}
  virtual ~NoteOnEvent() {}
  virtual MidiVarByte VarByte() const { return MidiVarByte::NOTE_ON_x1; }
  virtual std::string str() const;
  uint8_t channel_{0};
  uint8_t key_{0};
  uint8_t velocity_{0};
};

class KeyPressureEvent : public MidiEvent {
 public:
  KeyPressureEvent(
    unsigned dt,
    uint8_t channel,
    uint8_t number,
    uint8_t value) :
      MidiEvent(dt), channel_{channel}, number_{number}, value_{value} {}
  virtual ~KeyPressureEvent() {}
  virtual MidiVarByte VarByte() const { return MidiVarByte::KEY_PRESSURE_x2; }
  virtual std::string str() const;
  uint8_t channel_{0};
  uint8_t number_{0};
  uint8_t value_{0};
};

class ControlChangeEvent : public MidiEvent {
 public:
  ControlChangeEvent(
    unsigned dt,
    uint8_t channel,
    uint8_t number,
    uint8_t value) :
      MidiEvent(dt), channel_{channel}, number_{number}, value_{value} {}
  virtual ~ControlChangeEvent() {}
  virtual MidiVarByte VarByte() const { return MidiVarByte::CONTROL_CHANGE_x3; }
  virtual std::string str() const;
  uint8_t channel_{0};
  uint8_t number_{0};
  uint8_t value_{0};
};

class ProgramChangeEvent : public MidiEvent {
 public:
  ProgramChangeEvent(
    unsigned dt,
    uint8_t channel,
    uint8_t number) :
      MidiEvent(dt), channel_{channel}, number_{number} {}
  virtual ~ProgramChangeEvent() {}
  virtual MidiVarByte VarByte() const { return MidiVarByte::PROGRAM_CHANGE_x4; }
  virtual std::string str() const;
  uint8_t channel_{0};
  uint8_t number_{0};
};

class ChannelPressureEvent : public MidiEvent {
 public:
  ChannelPressureEvent(
    unsigned dt,
    uint8_t channel,
    uint8_t value) :
      MidiEvent(dt), channel_{channel}, value_{value} {}
  virtual ~ChannelPressureEvent() {}
  virtual MidiVarByte VarByte() const {
    return MidiVarByte::CHANNEL_PRESSURE_x5;
  }
  virtual std::string str() const;
  uint8_t channel_{0};
  uint8_t value_{0};
};

class PitchWheelEvent : public MidiEvent {
 public:
  PitchWheelEvent(
    unsigned dt,
    uint8_t channel,
    uint16_t bend) :
      MidiEvent(dt), channel_{channel}, bend_{bend} {}
  virtual ~PitchWheelEvent() {}
  virtual MidiVarByte VarByte() const { return MidiVarByte::PITCH_WHEEL_x6; }
  virtual std::string str() const;
  uint8_t channel_{0};
  uint16_t bend_{0};
};

// End of Midi Events
////////////////////////////////////////////////////////////////////////

class Track {
 public:
  std::vector<std::unique_ptr<Event>> events_;
  std::vector<uint8_t> GetChannels() const;
  std::vector<uint8_t> GetPrograms() const;
  // empty range if range[0] > range[1]
  std::array<uint8_t, 2> GetKeyRange() const;
  std::array<uint8_t, 2> GetVelocityRange() const;
  std::string info(const std::string &indent="") const;
};

class Midi {
 public:
  Midi(const std::string &path, uint32_t debug=0);
  std::string GetError() const { return error_; }
  bool Valid() const { return error_.empty(); }
  uint16_t GetFormat() const { return format_; }
  size_t GetNumTracks() const { return ntrks_; }
  uint16_t GetDivision() const { return division_; }
  uint32_t GetTicksPerQuarterNote() const { return ticks_per_quarter_note_; }
  uint8_t GetNegativeSmpteFormat() const { return negative_smpte_format_; }
  uint16_t GetTicksPerFrame() const { return ticks_per_frame_; }
  const std::vector<Track> &GetTracks() const { return tracks_; }
  std::vector<uint8_t> GetChannels() const;
  std::vector<uint8_t> GetPrograms() const;
  std::string info(const std::string& indent="") const;
  
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
  std::unique_ptr<MidiEvent> GetMidiEvent(
    uint32_t delta_time,
    uint8_t event_first_byte);
  std::unique_ptr<TextBaseEvent> GetTextBaseEvent(
    uint32_t delta_time,
    uint8_t meta_first_byte);
  size_t GetNextSize();
  size_t GetSizedQuantity();
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
