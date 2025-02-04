---
header-includes:
  - \usepackage{hyperref}
  - \hypersetup{colorlinks=true, linkcolor=blue, urlcolor=blue}
---
# modimidi
Command line midi player using 
[fluidsynth](https://www.fluidsynth.org/api/index.html) implented in C++

## Version
The current version is 0.1.0

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
```
$ modimidi 0x1 --tuning 415 -b 5:35 -e 5:42 --progress ${BASENAME}.mid
```


