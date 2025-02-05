---
header-includes:
  - \usepackage{hyperref}
  - \hypersetup{colorlinks=true, linkcolor=blue, urlcolor=blue}
---
# modimidi
Command line midi player using
[fluidsynth](https://www.fluidsynth.org/api/index.html) implented in C++

## Version
The current version is 0.1.1

# Example

## Get midi file

Download the midi file of
[**J.S.Bach**](https://www.mutopiaproject.org/cgibin/make-table.cgi?Composer=BachJS)'s
[*Lobet den Herrn, alle Heiden*](https://www.mutopiaproject.org/ftp/BachJS/BWV230/bach_BWV_230_Lobet_den_Herrn_alle_Heiden/bach_BWV_230_Lobet_den_Herrn_alle_Heiden.mid)
from [Mutopia](https://www.mutopiaproject.org) project.
or via
```
$ BASENAME=${BASENAME}
$ wget https://www.mutopiaproject.org/ftp/BachJS/BWV230/${BASENAME}/${BASENAME}.mid

```

## Playing

Assume we want to study the tenor part of
*Lobet den Herrn, alle Heiden*.
We focus on the last fuge, but just the first 4 bars of tenor.
This is time segment [5:35, 5:44.400].
To have an emphasized tenor voice,
we reduce the other 3 track voices via ```--tmap``` option.
Since this Barouqe music we change the tuning to 415
(from the 440 default).
To ease practice we slow down the speed by factor of 0.8.

```
$ modimidi --progress -b 5:35 -e 5:42 --tuning 415 --tmap 1:40 2:40 4:40 -T 0.8 ${BASENAME}.mid
```
# Build

## Requirements

* [GNU/Linux](https://www.gnu.org/gnu/linux-and-gnu.en.html)
* [CMake](https://cmake.org/)
* [fluidsynth](https://www.fluidsynth.org/api) library (available by the libfluidsynth-dev package).
* Boost (C++ library for [program_options](https://www.boost.org/doc/libs/1_87_0/doc/html/program_options.html))
* [libfmt](https://fmt.dev)  (C++ library for formatting)

## Steps

```
git clone https://github.com/yotam-medini/cmodimidi.git
cd cmodimidi
mkdir build
cd build
cmake ..
make
ls -l modimidi
```

Then install ```modimidi``` anywhere you want.

# Running

## Requirements

Sound fonts file.
By default ``/usr/share/sounds/sf2/FluidR3_GM.sf2``
which is provided by the ``fluid-soundfont-gm`` package.

## Command Line Options

|  Option                        | &nbsp;&nbsp;&nbsp; | [<text style="color: green">Default</text>] Description |
|  -------------                 | -------            |:------------- |
|  ``-h``,``--help``             |                    | produce help message |
|  ``--version``                 |                    | print version and exit |
|  ``--midifile`` *filename*     |                    | Path of the midi file to be played. |
|                                |                    | Can also be the last argument. |
|   ``-b``,``--begin`` *time*    |                    | [<font color="green">0</font>] start *time* |
|   ``-e``,``--end``   *time*    |                    |  [<font color="green">$\infty$</font>] end *time* |
|   ``--delay`` *time*           |                    | [<font color="green">0.200</font>] Initial extra playing delay |
|   ``--batch-duration`` *time*  |                    | [<font color="green">10</font>] sequencer batch duration |
|     &nbsp;                     |     &nbsp;         | Determines the amount of events sent to the fluidsynth engine |
|   ``-T``,``--tempo`` *factor*  |                    | [<font color="green">1.0</font>] Speed Multiplier factor, the greater the faster |
|   ``-K``,``--adjust-key`` *n*  |                    | [<font color="green">0</font>] Tranpose by $n$ semitone |
|   ``--tuning`` *frequency*     |                    | [<font color="green">415</font>] Tuning *frequency* of A4 (central La) |
|   ``tmap`` *arg*               |                    | (Repeatable) Tracks velocity mappings <*track*>:<*low*>[,<*high*>] |
|   ``cmap`` *arg*               |                    | (Repeatable) Channel velocity mappings <*track*>:<*low*>[,<*high*>] |
|   ``s``,``--soundfont`` *path* |                    | [<font color="green">/usr/share/sounds/sf2/FluidR3_GM.sf2</font>]  |
|                &nbsp;          |    &nbsp;          | Path to sound font |
|   ``--info``                   |                    | print general information of the midi file |
|   ``--dump`` *path*            |                    | Dump midi events contents to file, '-' for ``stdout`` |
|   ``--noplay``                 |                    | Do not play, usefull with ``--info`` or ``--dump`` |
|   ``--progress``               |                    | Show progress |
|   ``--debug`` $bitsflags$      |                    | [<font color="green">0</font>] Debug flags |

# Notes
* The *time* value format is [*minutes*]:*seconds*[.*millisecs*]
* Both ``--tmap`` and ``--cmap`` can be given. If both applied to an event, then ``--cmap`` takes precedence.
