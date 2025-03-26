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
        self.duration_ = None
        
    def duration(self) -> int:
        if self.duration_ is None:
            wholes = self.nn / (2**self.dd)
            quarters = 4 * wholes
            self.duration_ = 24 * quarters
        return self.duration_

    def __str__(self) -> str:
        return (
            f"TimeSignature(at={self.abs_time}, nn={self.nn}, dd={self.dd}"
            f", cc={self.cc}, bb={self.bb}, dur={self.duration()}")

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
        self.time_signatures = []
        self.tracks = []
        i_track = -1
        track = None
        ln = 0
        re_ts = (r".*AT=([0-9]*).*TimeSignature."
                 "nn=([0-9]*), dd=([0-9]*), cc=([0-9]*), bb=([0-9]*).*")
        re_noteon = (r".*AT=(\d+).*NoteOn."
                     "channel=(\d+), key=(\d+), velocity=(\d+)")
        re_noteoff = r".*AT=(\d+).*NoteOff.channel=(\d+), key=(\d+).*"
        note_on = None
        for line in fin.readlines():
            ln += 1
            note_off = None
            if line.startswith("Track["):
                i_track += 1
                track = Track()
                self.tracks.append(track)
                note_on = None
            if "TimeSignature" in line:
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
                        if ln < 75:
                            ew(f"note_on={note_on}\n")
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
                    if ln < 75:
                        ew(f"note_off={note_off}\n")
            if ((note_off is not None) and (note_on is not None) and
                (note_off.key == note_on.key)):
                duration = note_off.abs_time - note_on.abs_time
                note = Note(note_on.abs_time, note_on.key, duration)
                if ln < 75:
                    ew(f"note={note}\n")
                track.notes.append(note)
        ew(f"#(time_signatures)={len(self.time_signatures)}\n")
        
    def write_notes(self, ofn):
        for ti, track in enumerate(self.tracks):
            ew(f"Track[{ti}] #(notes)={len(track.notes)}\n")
   
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
    return parser.parse_args(args)
    
def main(args: [str]) -> int:
    parsed_args = parse_args(args)
    ow(f"{pprint.pformat(parsed_args)}\n")
    rc = Dump2Ly(parsed_args).run()
    return rc

if __name__ == "__main__":
    rc = main(sys.argv[1:])
    sys.exit(rc)
