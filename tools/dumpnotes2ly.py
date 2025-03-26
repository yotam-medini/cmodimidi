#!/usr/bin/env python
import argparse
import pprint
import sys

ow = sys.stdout.write

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
    return parser
    

def main(args: [str]) -> int:
    rc = 0
    parsed_args = parse_args(args)
    ow(f"{pprint.pformat(parsed_args)}\n")
    return rc

if __name__ == "__main__":
    rc = main(sys.argv[1:])
    sys.exit(rc)
