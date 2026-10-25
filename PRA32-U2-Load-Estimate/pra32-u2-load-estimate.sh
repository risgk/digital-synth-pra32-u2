#!/bin/sh
# PRA32-U2 Load Estimate: builds PRA32-U2, PRA32-U2/M, PRA32-U2/P, and PRA32-U2/M/P with arduino-cli,
# as the GitHub Actions workflow builds the UF2 files, into "build" here, and estimates the load of
# PRA32-U2 and PRA32-U2/M/P in the cases below with pra32-u2-load-estimate.py, and prints the tables
# of README.md, with the RAM of all 4
#
#   sh PRA32-U2-Load-Estimate/pra32-u2-load-estimate.sh
#
# Needs arduino-cli with the Arduino-Pico core and the MIDI Library (see README.md here), python3, and:
# pip install unicorn capstone pyelftools. Tested on Linux
set -e
cd "$(dirname "$0")"
FQBN='rp2040:rp2040:rpipico2:flash=4194304_0,freq=150,arch=arm,opt=Small,os=none,profile=Disabled,rtti=Disabled,stackprotect=Disabled,exceptions=Disabled,dbgport=Disabled,dbglvl=None,usbstack=tinyusb,ipbtstack=ipv4only,uploadmethod=default'

# All 4 are built, for their RAM; the load is estimated for PRA32-U2, the lightest, and PRA32-U2/M/P,
# which has all of them (PRA32-U2/M and /P differ from these little: the panel's I2C is left out)
VARIANTS='u2 m p mp'
ESTIMATED='u2 mp'
for v in $VARIANTS; do
  src=build/src-$v/Digital-Synth-PRA32-U2
  rm -rf build/src-$v
  mkdir -p $src
  cp ../Digital-Synth-PRA32-U2/* $src/
  case $v in m|mp) cp ../Digital-Synth-PRA32-U2-M/Digital-Synth-PRA32-U2-M.ino $src/Digital-Synth-PRA32-U2.ino ;; esac
  case $v in p|mp)  # Without "sed -i", which differs between GNU and BSD (macOS)
    sed 's|^//#define PRA32_U2_USE_CONTROL_PANEL |#define PRA32_U2_USE_CONTROL_PANEL |' $src/Digital-Synth-PRA32-U2.ino > $src/ino.tmp
    mv $src/ino.tmp $src/Digital-Synth-PRA32-U2.ino ;;
  esac
  arduino-cli compile --fqbn "$FQBN" --build-path build/$v $src > build/$v.log
done

version=$(sed -n 's/^#define PRA32_U2_VERSION *"\(v[0-9.]*\).*/\1/p' ../Digital-Synth-PRA32-U2/Digital-Synth-PRA32-U2.ino)
# A maximum over 100% (1333 us) is marked with "⚠", and one over 95% with "△"; each is also written to
# build/warnings.txt, which the workflow shows as a warning
: > build/warnings.txt
estimate() {  # variant, case, options: prints "core 0 max% / core 1 max%"
  v=$1
  name=$2
  shift 2
  python3 pra32-u2-load-estimate.py build/$v/Digital-Synth-PRA32-U2.ino.elf --brief "$@" |
    awk -v v="$v" -v name="$name" -v warnings=build/warnings.txt '
      function mark(p, core) {
        if (p > 100) { printf "%s, %s: core %d %.1f%%, over 1333 us\n", v, name, core, p >> warnings; return " ⚠" }
        if (p > 95) { printf "%s, %s: core %d %.1f%%, little margin\n", v, name, core, p >> warnings; return " △" }
        return ""
      }
      { printf "%.0f%%%s / %.0f%%%s", $2, mark($2, 0), $4, mark($4, 1) }'
}
row() {  # case, variants, options
  name=$1
  vs=$2
  shift 2
  line="| $name |"
  for v in $ESTIMATED; do
    case " $vs " in *" $v "*) line="$line $(estimate $v "$name" "$@") |" ;; *) line="$line - |" ;; esac
  done
  echo "$line"
}

echo "#### $version"
echo
echo "| Case (core 0 / core 1, max) | PRA32-U2 | PRA32-U2/M/P |"
echo "| :-- | --: | --: |"
# The heaviest: #5 Sync Lead (Osc 1 Wave Sqr) with Delay Mode Rev, in Poly (PRA32-U2), and as the
# 4 Synths in Mono by MIDI Channel 16 (Layering, PRA32-U2/M/P); for the Program Changes, written to
# the user programs #0 (and #1-3 for the Sub Synths) for the time of the estimate
POLY='--program 5 --cc 18=0,35=127 --write 0'
SYNTHS='--program 1:5,2:5,3:5,4:5 --cc 1:35=127 --write 1:0,2:1,3:2,4:3 --notes 16:60,16:64,16:67,16:71'
NOTE_ON_OFF='904864804840'  # Note On and Off of the note 72
EVERY_30_BYTES="$(printf 'FE%.0s' $(seq 28))"  # 28 Active Sensings: about 10 ms by UART MIDI, 6 loops by USB MIDI
row '#0 Synth Pad, 4 notes' "$ESTIMATED" --program 0
row 'Poly: #5 Sync Lead (Osc 1 Wave Sqr) in Poly with Delay Mode Rev, 4 notes' "$ESTIMATED" $POLY
row 'Poly, with Note On and Off of another note by UART MIDI and USB MIDI' "$ESTIMATED" $POLY \
  --uart-midi $NOTE_ON_OFF --usb-midi $NOTE_ON_OFF
row 'Poly, with Program Change #0 (Poly itself) every 30 bytes by UART MIDI and USB MIDI' "$ESTIMATED" $POLY \
  --uart-midi C000$EVERY_30_BYTES --usb-midi C000$EVERY_30_BYTES
row '4 Synths: #5 Sync Lead (Mono, and Delay Mode Rev) by MIDI Channel 16 (Layering), 4 notes' 'mp' $SYNTHS
row '4 Synths, with Note On and Off of another note by MIDI Channel 16 by UART MIDI and USB MIDI' 'mp' $SYNTHS \
  --uart-midi 9F48648F4840 --usb-midi 9F48648F4840
row '4 Synths, with Program Change #0 (4 Synths itself) by MIDI Channel 16 every 30 bytes by UART MIDI and USB MIDI' 'mp' $SYNTHS \
  --uart-midi CF00$EVERY_30_BYTES --usb-midi CF00$EVERY_30_BYTES
row '4 Synths, with Program Change #0 (4 Synths itself) by MIDI Channel 16 back to back by UART MIDI and USB MIDI' 'mp' $SYNTHS \
  --uart-midi CF00 --usb-midi CF00
echo
echo "⚠: over 100% (1333 us), △: over 95%"
echo
echo "| RAM (global variables) | PRA32-U2 | PRA32-U2/M | PRA32-U2/P | PRA32-U2/M/P |"
echo "| :-- | --: | --: | --: | --: |"
line='| |'
for v in $VARIANTS; do
  line="$line $(sed -n 's/^Global variables use \([0-9]*\) bytes (\([0-9]*%\)).*/\1 (\2)/p' build/$v.log) |"
done
echo "$line"
