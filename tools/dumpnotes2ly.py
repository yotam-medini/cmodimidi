#!/usr/bin/env python
import argparse
import pprint
import re
import sys

ow = sys.stdout.write
ew = sys.stderr.write

class TimeSignature:
    def __init__(self, at=0, nn=0, dd=1, cc=0, bb=0):
        self.abs_time = at
        self.nn = nn
        self.dd = dd
        self.cc = cc
        self.bb = bb
        self.quarters_ = None
        self.duration_ = None
        
    def quarters(self) -> int:
        if self.quarters_ is None:
            wholes = self.nn / (2**self.dd)
            qs = 4 * wholes
            self.quarters_ = qs
        return self.quarters_

    def duration(self) -> int: # WRONG!
        if self.duration_ is None:
            self.duration_ = self.quarters()
        return self.duration_

    def stime(self) -> str:
        return f"{self.nn}/{2**self.dd}"
    def __str__(self) -> str:
        return (
            f"TimeSignature(at={self.abs_time}, nn={self.nn}, dd={self.dd}"
            f", cc={self.cc}, bb={self.bb}, #(1/4)={self.quarters()}")

class NoteOn:
    def __init__(self, at=0, channel=0, key=0, value=0):
        self.abs_time = at
        self.channel = channel
        self.key = key
        self.value = value

    def __str__(self) -> str:
        return (f"NoteOn(at={self.abs_time}, c={self.channel}, "
                f"key={self.key}, v={self.value})")
        
class NoteOff:
    def __init__(self, at=0, channel=0, key=0):
        self.abs_time = at
        self.channel = channel
        self.key = key

    def __str__(self) -> str:
        return (f"NoteOff(at={self.abs_time}, channel={self.channel}, "
                f"key={self.key})")
        
class Note:
    def __init__(self, at=0, key=0, duration=0):
        self.abs_time = at
        self.key = key
        self.duration = duration

    def end_time(self):
        return self.abs_time + self.duration
    
    def __str__(self) -> str:
        return f"Note(at={self.abs_time}, key={self.key}, dur={self.duration})"
        
class Track:
    def __init__(self):
        self.name = ""
        self.notes = []

class Dump2Ly:
    def __init__(self, pa):
        self.rc = 0
        self.pa = pa

    def run(self) -> int:
        ifn = self.pa.input
        fin = sys.stdin if ifn == "-" else open(ifn)
        self.parse(fin)
        fin.close()
        ofn = self.pa.output
        fout = sys.stdout if ofn == "-" else open(ofn, "w")
        self.write_notes(fout)
        fout.close()
        return self.rc

    def parse(self, fin):
        self.ticks_per_quarter = 48
        self.time_signatures = []
        self.tracks = []
        i_track = -1
        track = None
        ln = 0
        re_tpq = r".*ticksPer.1/4.=(\d+).*"
        re_ts = (r".*AT=([0-9]*).*TimeSignature."
                 "nn=([0-9]*), dd=([0-9]*), cc=([0-9]*), bb=([0-9]*).*")
        re_noteon = (r".*AT=(\d+).*NoteOn."
                     "channel=(\d+), key=(\d+), velocity=(\d+)")
        re_noteoff = r".*AT=(\d+).*NoteOff.channel=(\d+), key=(\d+).*"
        re_trackname = r".*SequenceTrackName.([A-Za-z0-9]*).*"
        note_ons = {}
        for line in fin.readlines():
            ln += 1
            note_off = None
            if "ticksPer" in line:
                m = re.match(re_tpq, line)
                if m is None:
                    ew(f"Failed to parse [{ln}] {line}\n")
                else:
                    self.ticks_per_quarter = int(m.groups()[0])
            elif line.startswith("Track["):
                i_track += 1
                track = Track()
                self.tracks.append(track)
                note_ons = {}
            elif "SequenceTrackName" in line:
                m = re.match(re_trackname, line)
                if m is None:
                    ew(f"Failed to parse [{ln}] {line}\n")
                else:
                    track.name = m.groups()[0]
            elif "TimeSignature" in line:
                m = re.match(re_ts, line)
                if m is None:
                    ew(f"Failed to parse [{ln}] {line}\n")
                else:
                    nums = list(map(int, m.groups()))
                    ts = TimeSignature(*nums)
                    ew(f"ts={ts}\n")
                    self.time_signatures.append(ts)
            elif "NoteOn(" in line:
                m = re.match(re_noteon, line)
                if m is None:
                    ew(f"Failed to parse [{ln}] {line}\n")
                    sys.exit(13)
                else:
                    nums = list(map(int, m.groups()))
                    velocity = nums[3]
                    if velocity > 0:
                        note_on = NoteOn(nums[0], nums[1], nums[2], velocity)
                        note_ons.setdefault(note_on.key, []).append(note_on)
                    else:
                        note_off = Note_off(nums[0], nums[1], nums[2])
            elif "NoteOff(" in line:
                m = re.match(re_noteoff, line)
                if m is None:
                    ew(f"Failed to parse [{ln}] {line}\n")
                    sys.exit(13)
                else:
                    nums = list(map(int, m.groups()))
                    note_off = NoteOff(nums[0], nums[1], nums[2])
            if note_off is not None:
                notes_on_l = note_ons.setdefault(note_off.key, [])
                if len(notes_on_l) == 0:
                    ew(f"Unmatched {note_off} @ ln={ln}\n")
                else:
                    note_on = notes_on_l[-1]
                    duration = note_off.abs_time - note_on.abs_time
                    note = Note(note_on.abs_time, note_on.key, duration)
                    track.notes.append(note)
                    notes_on_l.pop()
        ew(f"#(time_signatures)={len(self.time_signatures)}\n")
        
    def write_notes(self, fout):
        for ti, track in enumerate(self.tracks):
            ew(f"Track[{ti}] {track.name} #(notes)={len(track.notes)}\n")

        syms = (
            ["c", "df", "d", "ef", "e", "f", "gf", "g", "af", "a", "bf", "b"]
            if self.pa.flat else
            ["c", "cs", "d", "ds", "e", "f", "fs", "g", "gs", "a", "as", "b"]
        )

        for ti, track in enumerate(self.tracks):
            curr_ts = TimeSignature(nn=4, dd=2, cc=24, bb=8)
            curr_ts_idx = -1
            pre_key = 60
            fout.write(f"\ntrack{ti}{track.name} = ")
            fout.write("{\n ")
            curr_ts_bar = 0
            curr_bar = 0
            last_bar = 0
            curr_at = 0
            last_dur = dur = ""
            tss = self.time_signatures # abbreviate
            gnotes = self.fill_rests(track.notes)
            for ni, note in enumerate(gnotes):
                while (curr_ts_idx + 1 < len(tss) and
                       (tss[curr_ts_idx + 1].abs_time < note.abs_time)):
                    pre_ts = curr_ts
                    curr_ts_idx += 1
                    curr_ts = tss[curr_ts_idx];
                    fout.write(f"\n  \\time {curr_ts.stime()}\n ")
                    dt = curr_ts.abs_time - curr_at
                    n_quarters = pre_ts.quarters()
                    bars = dt / (n_quarters * self.ticks_per_quarter)
                    curr_bar += int(bars + 1./2.)
                    curr_at = curr_ts.abs_time
                    curr_ts_bar = curr_bar
                if ni > 0 and note.abs_time <= gnotes[ni - 1].abs_time:
                    fout.write("\n ") # multi voice
                dt = note.abs_time - curr_ts.abs_time
                n_quarters = curr_ts.quarters()
                bars = dt/(n_quarters * self.ticks_per_quarter)
                n_bars = int(bars)
                curr_bar = curr_ts_bar + n_bars
                if last_bar != curr_bar:
                    last_bar = curr_bar
                    pts = str(curr_ts) if self.pa.debug & 0x1 else ""
                    fout.write(f"\n  % | % bar {curr_bar + 1} {pts}\n ")
                key = note.key
                ksym = "r"
                jump = ""
                if key >= 0:
                    ksym = syms[key % 12]
                    jump = ""
                    if key - pre_key > 6:
                        jump = "'"
                    elif key - pre_key < -6:
                        jump = ","
                dur = self.midi_dur_to_ly_dur(note.duration)
                new_dur = ""
                if last_dur != dur:
                    last_dur = dur
                    new_dur = dur
                fout.write(f" {ksym}{jump}{new_dur}")
                pre_key = key
            fout.write("\n}\n")

    def fill_rests(self, notes):
        gnotes = []
        curr_time = 0
        for note in notes:
            delta = note.abs_time - curr_time
            if delta > self.ticks_per_quarter/4:
                gnotes.append(Note(curr_time + self.pa.rest_shift, -1, delta)) # rest
            curr_time = note.end_time()
            gnotes.append(note)
        return gnotes

    def midi_dur_to_ly_dur(self, midi_dur) -> str:
        tpq = self.ticks_per_quarter
        whole = 4*tpq
        grace = 11./12.
        ret = ""
        if midi_dur > grace * whole:
            ret = "1"
        elif midi_dur > grace * (3./4.) * whole:
            ret = "2."
        elif midi_dur > grace * (1./2.) * whole:
            ret = "2"
        elif midi_dur > grace * (3./8.) * whole:
            ret = "4."
        elif midi_dur > grace * (1./4.) * whole:
            ret = "4"
        elif midi_dur > grace * (3./16.) * whole:
            ret = "8."
        elif midi_dur > grace * (1./8.) * whole:
            ret = "8"
        elif midi_dur > grace * (3./32.) * whole:
            ret = "16."
        elif midi_dur > grace * (1./16.) * whole:
            ret = "16"
        elif midi_dur > grace * (1./32.) * whole:
            ret = "32"
        return ret
   
def parse_args(args: [str]):
    parser = argparse.ArgumentParser(
        "dumpnotes2ly", "Fetch notes for LilyPond from modimidi dump",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument(
        "-i", "--input",
        default="-",
        help="Dump output from modimidi, '-' for stdin")
    parser.add_argument(
        "-o", "--output",
        default="-",
        help="Lilypond notes sequences")
    parser.add_argument(
        "--flat",
        action="store_true",
        default=False,
        help="Use flats rather than defualt sharp")
    parser.add_argument(
        "--rest-shift",
        type=int,
        default=20,
        help="ticks shift of rests")
    parser.add_argument(
        "--debug",
        type=int,
        default=0,
        help="Debug flags")
    return parser.parse_args(args)

def main(args: [str]) -> int:
    parsed_args = parse_args(args)
    ow(f"{pprint.pformat(parsed_args)}\n")
    rc = Dump2Ly(parsed_args).run()
    return rc

if __name__ == "__main__":
    rc = main(sys.argv[1:])
    sys.exit(rc)
