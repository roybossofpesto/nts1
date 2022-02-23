#!/bin/sh

set -e
set -u

make

if [ "${OS}" = "Windows_NT" ]
then
  logue-cli load --unit hosho.ntkdigunit --slot 2 --inport 2 --outport 3
else
  logue-cli load --unit hosho.ntkdigunit --slot 2 --inport 2 --outport 2
fi
