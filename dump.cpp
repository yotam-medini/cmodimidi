#include "midi.h"
#include <iostream>
#include <fstream>
#include <memory>
#include <fmt/core.h>

void midi_dump(const midi::Midi &parsed_midi, std::string &path) {
  std::unique_ptr<std::ofstream> pofs;
  std::ostream *pout = &std::cout;
  if (path != std::string("-")) {
    pofs = std::make_unique<std::ofstream>(path);
    pout = pofs.get();
  }
  std::ostream &out = *pout;
  const midi::Midi &pm = parsed_midi; // abbreviation
  out << "Midi dump\n";
  const size_t ntrks = pm.GetNumTracks();
  out << fmt::format("format={}, #(tracks)={} division={} ticksPer(1/4)={}\n",
    pm.GetFormat(), ntrks, pm.GetDivision(), pm.GetTicksPerQuarterNote());
  for (size_t itrack = 0; itrack < ntrks; ++itrack) {
    const midi::Track &track = pm.GetTracks()[itrack];
    const std::vector<std::unique_ptr<midi::Event>> &events = track.events_;
    const size_t ne = events.size();
    out << fmt::format("Track[{:2d}] #(events)={}", itrack, ne) << " {\n";
    for (size_t ie = 0; ie < ne; ++ie) {
      out << fmt::format(" [{:4d}] {}\n", ie, events[ie]->str());
    } 
    out << "}\n";
  }
}

