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
  echo "midi inport '${midi_inport}' outport '${midi_outport}'"
  if ! [[ ${midi_inport} =~ ^[0-9]+$ ]]; then echo "invalid midi in"; fi
  if ! [[ ${midi_outport} =~ ^[0-9]+$ ]]; then echo "invalid midi outport"; fi

  logue-cli probe --inport ${midi_inport} --outport ${midi_outport} -v

  echo "use native tool"
  logue-cli load --unit patate.ntkdigunit --slot 9 --inport ${midi_inport} --outport ${midi_outport}

  # amidi_ioport="$( amidi -l | awk '/NTS-1/ {print $2}' )"
  # echo "amidi '${amidi_ioport}'"
  # if ! [[ ${amidi_ioport} =~ hw:[0-9,]+$ ]]; then echo "invalid amidi ioport"; fi
  
  # echo "use amidi"
  # logue_dump="$( logue-cli load -m osc --unit patate.ntkdigunit --slot 9 --inport ${midi_inport} --outport ${midi_outport} --debug 2> /dev/null || true )"
  # amidi_sysexhex="$( echo "${logue_dump}" | awk '/size/{got_size=1;}; got_size&&/>>> {.*}/{print;}' | awk '{gsub(/[[:punct:]]/, ""); print;}' )"
  # #  | awk 'END {gsub(/[[:punct:]]/, ""); print}' > foo.sysex
  # echo "**${amidi_sysexhex}**"
  # amidi -p "${amidi_ioport}" -S "${amidi_sysexhex}"

  logue-cli probe --inport ${midi_inport} --outport ${midi_outport} -v
fi
