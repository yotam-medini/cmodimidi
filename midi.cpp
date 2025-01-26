#include "midi.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <set>
#include <fmt/core.h>

namespace fs = std::filesystem;

namespace midi {

std::string Event::dt_str() const {
  return fmt::format("DT={} {}", delta_time_, str());
}

// Meta Event

std::string SequenceNumberEvent::str() const {
  return fmt::format("SequenceNumber({})", number_);
}

std::string TextBaseEvent::str() const {
  return fmt::format("{}({})", event_type_name(), s_);
}

std::string ChannelPrefixEvent::str() const {
  return fmt::format("ChannelPrefix({})", channel_);
}

std::string PortEvent::str() const {
  return fmt::format("PortEvent({})", port_);
}

std::string EndOfTrackEvent::str() const {
  return fmt::format("EndOfTrack");
}

std::string TempoEvent::str() const {
  return fmt::format("Port({})", tttttt_);
}

std::string SmpteOffsetEvent::str() const {
  return fmt::format("SmpteOffset(hr={}, mn={}, se={}, fr={}, ff={})",
    hr_, mn_, se_, fr_, ff_);
}

std::string TimeSignatureEvent::str() const {
  return fmt::format("TimeSignature(nn={}, dd={}, cc={}, bb={})",
    nn_, dd_, cc_, bb_);
}

std::string KeySignatureEvent::str() const {
  return fmt::format("KeySignatureEvent(sf={}, mi={})",
    sf_, mi_);
}

std::string SequencerEvent::str() const {
  return fmt::format("Sequencer(#(data)={})", data_.size());
}

// Midi Event
std::string NoteOffEvent::str() const { 
  return fmt::format("NoteOff(channel={}, key={}, velocity={})",
    channel_, key_, velocity_);
}
std::string NoteOnEvent::str() const {
  return fmt::format("NoteOn(channel={}, key={}, velocity={})",
    channel_, key_, velocity_);
}
std::string KeyPressureEvent::str() const {
  return fmt::format("KeyPressure(channel={}, number={}, value={})",
    channel_, number_, value_);
}
std::string ControlChangeEvent::str() const {
  return fmt::format("ControlChange(channel={}, number={}, value={})",
    channel_, number_, value_);
}
std::string ProgramChangeEvent::str() const {
  return fmt::format("ProgramChange(channel={}, number={})", channel_, number_);
}
std::string ChannelPressureEvent::str() const {
  return fmt::format("ChannelPressure(channel={}, value={})", channel_, value_);
}
std::string PitchWheelEvent::str() const {
  return fmt::format("ChannelPressure(channel={}, bend={})", channel_, bend_);
}

////////////////////////////////////////////////////////////////////////

std::vector<uint8_t> Track::GetChannels() const {
  std::set<uint8_t> channels;
  for (const auto &e: events_) {
    const NoteOnEvent *note_on = dynamic_cast<const NoteOnEvent*>(e.get());
    if (note_on) {
      channels.insert(channels.end(), note_on->channel_);
    }
  }
  return std::vector(channels.begin(), channels.end());
}

std::vector<uint8_t> Track::GetPrograms() const {
  std::set<uint8_t> programs;
  for (const auto &e: events_) {
    const ProgramChangeEvent *prog_change =
      dynamic_cast<const ProgramChangeEvent*>(e.get());
    if (prog_change) {
      programs.insert(programs.end(), prog_change->number_);
    }
  }
  return std::vector(programs.begin(), programs.end());
}

std::array<uint8_t, 2> Track::GetKeyRange() const {
  std::array<uint8_t, 2> range{0xff, 0};
  for (const auto &e: events_) {
    const NoteOnEvent *note_on = dynamic_cast<const NoteOnEvent*>(e.get());
    if (note_on) {
      uint8_t v = note_on->key_;
      if (range[0] > v) {
        range[0] = v;
      }
      if (range[1] < v) {
        range[1] = v;
      }
    }
  }
  return range;
}

std::array<uint8_t, 2> Track::GetVelocityRange() const {
  std::array<uint8_t, 2> range{0xff, 0};
  for (const auto &e: events_) {
    const NoteOnEvent *note_on = dynamic_cast<const NoteOnEvent*>(e.get());
    uint8_t v;
    if (note_on && (v = note_on->velocity_) > 0) {
      if (range[0] > v) {
        range[0] = v;
      }
      if (range[1] < v) {
        range[1] = v;
      }
    }
  }
  return range;
}

std::string Track::info(const std::string& indent) const {
  std::string s;
  size_t n_notes = 0;
  for (const auto &e: events_) {
    const MetaEvent *meta_event = dynamic_cast<const MetaEvent*>(e.get());
    const MidiEvent *midi_event = dynamic_cast<const MidiEvent*>(e.get());
    if (meta_event) {
      if ((!dynamic_cast<const LyricEvent*>(meta_event)) &&
          (!dynamic_cast<const EndOfTrackEvent*>(meta_event))) {
        s = fmt::format("{}{}{}\n", s, indent, meta_event->str());
      }
    } else if (midi_event) {
      const NoteOnEvent *note_on = dynamic_cast<const NoteOnEvent*>(midi_event);
      if (note_on) {
        if (note_on->velocity_ > 0) {
          ++n_notes;
        }
      } else if (!dynamic_cast<const NoteOffEvent*>(midi_event)) {
        s = fmt::format("{}{}{}\n", s, indent, midi_event->str());
      }
    }
  }
  if (n_notes == 0) {
    s = fmt::format("{}{}No notes\n", s, indent);
  } else {
    std::vector<uint8_t> channels = GetChannels();
    std::array<uint8_t, 2> key_range = GetKeyRange();
    std::array<uint8_t, 2> vel_range = GetVelocityRange();
    s = fmt::format("{}{}Channels:", s, indent);
    for (uint8_t c: channels) { s = fmt::format("{} {}", s, int(c)); }
    s = fmt::format("{}\n{}{} notes, keys: [{}, {}], velocity: [{}, {}]\n",
      s, indent, n_notes,
      key_range[0], key_range[1], vel_range[0], vel_range[1]);
  }
  return s;
}


////////////////////////////////////////////////////////////////////////

Midi::Midi(const std::string &midifile_path, uint32_t debug) :
  debug_{debug} {
  GetData(midifile_path);
  if (Valid()) {
    Parse();
  }
}

std::vector<uint8_t> Midi::GetChannels() const {
  std::set<uint8_t> channels;
  for (const Track& track: tracks_) {
    std::vector<uint8_t> t_channels = track.GetChannels();
    for (uint8_t c: t_channels) {
      channels.insert(channels.end(), c);
    }
  }
  return std::vector<uint8_t>(channels.begin(), channels.end()); 
}

std::vector<uint8_t> Midi::GetPrograms() const {
  std::set<uint8_t> programs;
  for (const Track& track: tracks_) {
    std::vector<uint8_t> t_programs = track.GetPrograms();
    for (uint8_t c: t_programs) {
      programs.insert(programs.end(), c);
    }
  }
  return std::vector<uint8_t>(programs.begin(), programs.end()); 
}

std::string Midi::info(const std::string& indent) const {
  const std::string sub_indent{indent + std::string("  ")};
  std::string s = fmt::format("{}Format={} ntrks={}, Ticks Per (1/4)={}\n",
    indent, format_, ntrks_, ticks_per_quarter_note_);
  for (size_t ti = 0; ti < tracks_.size(); ++ti) {
    s = fmt::format("{}{}Track[{}] {}", s, indent, ti, "{\n");
    s = fmt::format("{}{}", s, tracks_[ti].info(sub_indent));
    s = fmt::format("{}{}{}", s, indent, "}\n");
  }
  return s;
}

void Midi::GetData(const std::string &midifile_path) {
  if (fs::exists(midifile_path)) {
    auto file_size = fs::file_size(midifile_path);
    if (debug_ & 0x1) {
      std::cout << fmt::format("size({})={}\n", midifile_path, file_size);
    }
    if (file_size < 0x20) {
      error_ = fmt::format("Midi file size={} too short", file_size);
    }
    if (Valid()) {
      data_.clear();
      data_.insert(data_.end(), file_size, 0);
      std::ifstream f(midifile_path, std::ios::binary);
      f.read(reinterpret_cast<char*>(data_.data()), file_size);
      if (f.fail()) {
        error_ = fmt::format("Failed to read {} bytes from {}",
          file_size, midifile_path);
      }
    }
  } else {
    error_ = fmt::format("Does not exist: {}", midifile_path);
  }
}

void Midi::Parse() {
  ParseHeader();
  if (Valid()) {
    switch (format_) {
     case 0:
       ReadOneTrack();
       break;
     case 1:
       ReadTracks();
       break;
     default:
       error_ = fmt::format("Unsupported format={}", format_);
    }
  }
}

void Midi::ParseHeader() {
  static const std::string MThd{"MThd"};
  const std::string header = GetChunkType();
  if (header != MThd) {
    error_ = fmt::format("header: {} != {}", header, MThd);
  }
  if (Valid()) {
     size_t length = GetNextSize();
     if (length != 6) {
       std::cerr << fmt::format("Unexpected length: {} != 6\n", length);
     }
     format_ = GetU16from(8);
     ntrks_ = GetU16from(10);
     division_ = GetU16from(12);
     if (debug_ & 0x1) {
       std::cout << fmt::format("length={}, format={}, ntrks={}, "
         "division={:018b}\n",
         length, format_, ntrks_, division_);
     }
     uint16_t bit15 = division_ >> 15;
     if (bit15 == 0) {
       ticks_per_quarter_note_ = division_;
     } else {
       negative_smpte_format_ = data_[12] & 0x7f;
       ticks_per_frame_ = data_[13];
       // hack
       ticks_per_quarter_note_ =
         (0x100 - uint16_t{data_[12]}) * uint16_t{data_[13]};
     }
     parse_state_.offset_ += length;
     if (debug_ & 0x1) {
       std::cout << fmt::format(
         "ticks_per_quarter_note={} ticks_per_frame={}\n",
         ticks_per_quarter_note_, ticks_per_frame_);
     }
  }
}

void Midi::ReadTracks() {
  for (uint16_t itrack = 0; Valid() && (itrack < ntrks_); ++itrack) {
    ReadTrack();
  }
}

void Midi::ReadTrack() {
  static const std::string MTrk{"MTrk"};
  const std::string chunk_type = GetChunkType();
  if (chunk_type != MTrk) {
    error_ = fmt::format("chunk_type={} != {} @ offset={}",
      chunk_type, MTrk, parse_state_.offset_ - 4);
  }
  const size_t length = GetNextSize();
  const size_t offset_eot = parse_state_.offset_ + length;
  tracks_.push_back(Track());
  auto &events = tracks_.back().events_;
  bool got_eot = false;
  while ((!got_eot) && (parse_state_.offset_ < offset_eot)) {
    auto event = GetTrackEvent();
    got_eot = (dynamic_cast<EndOfTrackEvent*>(event.get()) != nullptr);
    events.push_back(std::move(event));
  }
  if ((!got_eot) || (parse_state_.offset_ != offset_eot)) {
    std::cerr << fmt::format(
      "Track not cleanly ended got_eot={}, offset={} != offset_eot={}\n",
      got_eot, parse_state_.offset_, offset_eot);
  }
}

std::unique_ptr<Event> Midi::GetTrackEvent() {
  uint32_t delta_time = GetVariableLengthQuantity();
  uint8_t event_first_byte = data_[parse_state_.offset_++];
  std::unique_ptr<Event> e;
  switch (event_first_byte) {
   case 0xff:
    e = GetMetaEvent(delta_time);
    break;
   case 0xf0:
   case 0xf7:
     std::cerr << "Sysex Event ignored\n";
    break;
   default:
    e = GetMidiEvent(delta_time, event_first_byte);
  }
  return e;
}

std::unique_ptr<MetaEvent> Midi::GetMetaEvent(uint32_t delta_time) {
  std::unique_ptr<MetaEvent> e;
  uint32_t length;
  std::string text;
  uint8_t meta_first_byte = data_[parse_state_.offset_++];
  switch (meta_first_byte) {
   case MetaVarByte::SEQNUM_x00:
    length = data_[parse_state_.offset_++];
    if (length != 2) {
      std::cerr << fmt::format("Unexpected length={}!=2 in SequenceNumber",
        length);
    }
    {
      uint16_t number = GetU16from(parse_state_.offset_);
      e = std::make_unique<SequenceNumberEvent>(delta_time, number);
    }
    parse_state_.offset_ += length;
   break;
   case MetaVarByte::TEXT_x01:
   case MetaVarByte::COPYRIGHT_x02:
   case MetaVarByte::TRACKNAME_x03:
   case MetaVarByte::INSTRNAME_x04:
   case MetaVarByte::LYRICS_x05:
   case MetaVarByte::MARK_x06:
   case MetaVarByte::DEVICE_x09:
    e = GetTextBaseEvent(delta_time, meta_first_byte);
    break;
   case MetaVarByte::CHANPFX_x20:
    length = GetVariableLengthQuantity();
    if (length != 1) {
      std::cerr << fmt::format("Unexpected length={}!=1 in ChannelPrefix",
        length);
    }
    e = std::make_unique<ChannelPrefixEvent>(
      delta_time, data_[parse_state_.offset_]);
    parse_state_.offset_ += length;
    break;
   case MetaVarByte::PORT_x21:
    length = GetVariableLengthQuantity();
    if (length != 1) {
      std::cerr << fmt::format("Unexpected length={}!=1 in ChannelPrefix",
        length);
    }
    e = std::make_unique<PortEvent>(delta_time, data_[parse_state_.offset_]);
    parse_state_.offset_ += length;
    break;
   case MetaVarByte::ENDTRACK_x2f:
    length = data_[parse_state_.offset_++];
    if (length != 0) {
      std::cerr << fmt::format("Unexpected length={}!=0 in EndOfTrack",
        length);
    }
    e = std::make_unique<EndOfTrackEvent>(delta_time);
    parse_state_.offset_ += length;
    break;
   case MetaVarByte::TEMPO_x51: {
      auto tttttt = GetSizedQuantity();
      e = std::make_unique<TempoEvent>(delta_time, tttttt);
    }
    break;
   case MetaVarByte::SMPTE_x54:
    length = data_[parse_state_.offset_++];
    if (length != 5) {
      std::cerr << fmt::format("Unexpected length={}!=5 in Tempo",
        length);
    }
    {
      size_t offs = parse_state_.offset_;
      e = std::make_unique<SmpteOffsetEvent>(
        delta_time, 
        data_[offs + 0],
        data_[offs + 1],
        data_[offs + 2],
        data_[offs + 3],
        data_[offs + 4]);
    }
    parse_state_.offset_ += length;
    break;
   case MetaVarByte::TIMESIGN_x58:
    length = data_[parse_state_.offset_++];
    if (length != 4) {
      std::cerr << fmt::format("Unexpected length={}!=4 in TimeSignature",
        length);
    }
    {
      size_t offs = parse_state_.offset_;
      e = std::make_unique<TimeSignatureEvent>(
        delta_time, 
        data_[offs + 0],
        data_[offs + 1],
        data_[offs + 2],
        data_[offs + 3]);
    }
    parse_state_.offset_ += length;
    break;
   case MetaVarByte::KEYSIGN_x59:
    length = data_[parse_state_.offset_++];
    if (length != 2) {
      std::cerr << fmt::format("Unexpected length={}!=2 in KeySignature",
        length);
    }
    {
      size_t offs = parse_state_.offset_;
      e = std::make_unique<KeySignatureEvent>(
        delta_time, data_[offs + 0], data_[offs + 1] != 0);
    }
    parse_state_.offset_ += length;
    break;
   case MetaVarByte::SEQUEMCER_x7f:
    length = GetVariableLengthQuantity();
    {
      std::vector<uint8_t>::const_iterator 
        b = data_.begin() + parse_state_.offset_;
      std::vector<uint8_t> data{b, b + length};
      e = std::make_unique<SequencerEvent>(delta_time, data);
    }
    parse_state_.offset_ += length;
    break;
   default:
    error_ = fmt::format("Meta event unsupported byte={:02x} @ {}",
      meta_first_byte, parse_state_.offset_ - 1);
    length = GetVariableLengthQuantity();
    parse_state_.offset_ += length;
  }
  return e;
}

std::unique_ptr<MidiEvent> Midi::GetMidiEvent(
    uint32_t delta_time,
    uint8_t event_first_byte) {
  std::unique_ptr<MidiEvent> e;
  uint8_t upper4 = (event_first_byte >> 4) & 0xf;
  if ((upper4 & 0x8) != 0) {
    parse_state_.last_status_ = upper4 & 0x7;
    parse_state_.last_channel_ = event_first_byte & 0xf;
  } else {
    --parse_state_.offset_; 
  }
  const size_t offs = parse_state_.offset_;
  switch (parse_state_.last_status_) {
   case MidiVarByte::NOTE_OFF_x0:
    e = std::make_unique<NoteOffEvent>(
      delta_time, parse_state_.last_channel_, data_[offs], data_[offs + 1]);
    parse_state_.offset_ += 2;
    break;
   case MidiVarByte::NOTE_ON_x1:
    e = std::make_unique<NoteOnEvent>(
      delta_time, parse_state_.last_channel_, data_[offs], data_[offs + 1]);
    parse_state_.offset_ += 2;
    break;
   case MidiVarByte::KEY_PRESSURE_x2:
    e = std::make_unique<KeyPressureEvent>(
      delta_time, parse_state_.last_channel_, data_[offs], data_[offs + 1]);
    parse_state_.offset_ += 2;
    break;
   case MidiVarByte::CONTROL_CHANGE_x3:
    e = std::make_unique<ControlChangeEvent>(
      delta_time, parse_state_.last_channel_, data_[offs], data_[offs + 1]);
    parse_state_.offset_ += 2;
    break;
   case MidiVarByte::PROGRAM_CHANGE_x4:
    e = std::make_unique<ProgramChangeEvent>(
      delta_time, parse_state_.last_channel_, data_[offs]);
    parse_state_.offset_ += 1;
    break;
   case MidiVarByte::CHANNEL_PRESSURE_x5:
    e = std::make_unique<ChannelPressureEvent>(
      delta_time, parse_state_.last_channel_, data_[offs]);
    parse_state_.offset_ += 1;
    break;
   case MidiVarByte::PITCH_WHEEL_x6: {
      uint16_t lllllll = data_[offs] & 0x7f;
      uint16_t mmmmmmm = data_[offs + 1] & 0x7f;
      uint16_t bend = (mmmmmmm << 7) | lllllll;
      e = std::make_unique<PitchWheelEvent>(
        delta_time, parse_state_.last_channel_, bend);
    }
    break;
  }
  return e;
}

std::unique_ptr<TextBaseEvent> Midi::GetTextBaseEvent(
    uint32_t delta_time,
    uint8_t meta_first_byte) {
  std::unique_ptr<TextBaseEvent> e;
  uint32_t length = GetVariableLengthQuantity();
  auto text = GetString(length);
  switch (meta_first_byte) {
   case MetaVarByte::TEXT_x01:
    e = std::make_unique<TextEvent>(delta_time, text);
    break;
   case MetaVarByte::COPYRIGHT_x02:
    e = std::make_unique<CopyrightEvent>(delta_time, text);
    break;
   case MetaVarByte::TRACKNAME_x03:
    e = std::make_unique<SequenceTrackNameEvent>(delta_time, text);
    break;
   case MetaVarByte::INSTRNAME_x04:
    e = std::make_unique<InstrumentNameEvent>(delta_time, text);
    break;
   case MetaVarByte::LYRICS_x05:
    e = std::make_unique<LyricEvent>(delta_time, text);
    break;
   case MetaVarByte::MARK_x06:
    e = std::make_unique<MarkerEvent>(delta_time, text);
    break;
   case MetaVarByte::DEVICE_x09:
    e = std::make_unique<DeviceEvent>(delta_time, text);
    break;
   default:
    error_ = fmt::format("BUG meta_first_byte={:0x2}", meta_first_byte);
  }
  return e;
}

size_t Midi::GetNextSize() {
  const size_t offs = parse_state_.offset_;
  size_t sz = 0;
  for (size_t i = 0; i < 4; ++i) {
    size_t b{data_[offs + i]};
    sz = (sz << 8) | b;
  }
  parse_state_.offset_ += 4;
  return sz;
}

size_t Midi::GetSizedQuantity() {
  const size_t n_bytes = data_[parse_state_.offset_++];
  size_t quantity = 0;
  for (size_t i = 0; i < n_bytes; ++i) {
    size_t b{data_[parse_state_.offset_++]};
    quantity = (quantity << 8) | b;
  }
  return quantity;
}

size_t Midi::GetVariableLengthQuantity() {
  size_t quantity = 0;
  size_t offs = parse_state_.offset_;
  const size_t ofss_limit = offs + 4;
  bool done = false;
  while ((offs < ofss_limit) && !done) {
    size_t b = data_[offs++];
    quantity = (quantity << 7) + (b & 0x7f);
    done = (b & 0x80) == 0;
  }
  parse_state_.offset_ = offs;
  return quantity;
}

uint16_t Midi::GetU16from(size_t from) const {
  uint16_t ret = (uint16_t{data_[from]} << 8) | uint16_t{data_[from + 1]};
  return ret;
}

std::string Midi::GetString(size_t length) {
  const size_t offs = parse_state_.offset_;
  std::string s{&data_[offs], &data_[offs + length]};
  parse_state_.offset_ += length;
  return s;
}

}
