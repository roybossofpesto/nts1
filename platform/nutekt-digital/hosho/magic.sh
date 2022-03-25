#!/bin/sh

set -e
set -u

make

if [ "${OS}" = "Windows_NT" ]
then
  logue-cli load --unit hosho.ntkdigunit --slot 2 --inport 0 --outport 1
else
  logue-cli load --unit hosho.ntkdigunit --slot 2 --inport 2 --outport 2
fi
