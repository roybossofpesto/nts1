#!/bin/bash

set -e
set -u

make

if [ "${OS:-""}" = "Windows_NT" ]
then
  logue-cli load --unit patate.ntkdigunit --slot 10 --inport 0 --outport 1
else
  midi_dump="$( logue-cli probe --list )"
  midi_dump="$( awk '/NTS-1/{ print $1":"$2; }' <<< "${midi_dump}" )"
  midi_inport="$( echo "${midi_dump}" | awk 'BEGIN { FS=":"; } /in:([0-9]+):/{ print $2; }' )"
  midi_outport="$( echo "${midi_dump}" | awk 'BEGIN { FS=":"; } /out:([0-9]+):/{ print $2; }' )"
  amidi_ioport="$( amidi -l | awk '/NTS-1/ {print $2}' )"
  echo "midi inport '${midi_inport}' outport '${midi_outport}'"
  # echo "amidi '${amidi_ioport}'"
  if ! [[ ${midi_inport} =~ ^[0-9]+$ ]]; then echo "invalid midi in"; fi
  if ! [[ ${midi_outport} =~ ^[0-9]+$ ]]; then echo "invalid midi outport"; fi
  # if ! [[ ${amidi_ioport} =~ hw:[0-9,]+$ ]]; then echo "invalid amidi ioport"; fi
  logue-cli probe --inport ${midi_inport} --outport ${midi_outport} -v
  
  sleep 1
  echo "use native tool"
  logue-cli load --unit patate.ntkdigunit --slot 9 --inport ${midi_inport} --outport ${midi_outport}

  echo "use amidi"
  # logue_dump="$( logue-cli load --unit patate.ntkdigunit --slot 12 --inport ${midi_inport} --outport ${midi_outport} --debug 2> /dev/null || true )"
  # echo "${logue_dump}" | awk 'END {gsub(/[[:punct:]]/, ""); print}' > foo.sysex

  logue-cli probe --inport ${midi_inport} --outport ${midi_outport} -v
fi
