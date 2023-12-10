#!/bin/sh

set -e
set -u

make

alias logue-cli="/home/pgueth/git/nts1/tools/logue-cli/logue-cli-linux64-0.07-2b/logue-cli"

if [ "${OS:-""}" = "Windows_NT" ]
then
  logue-cli load --unit hosho.ntkdigunit --slot 1 --inport 1 --outport 1
else
  logue-cli load --unit hosho.ntkdigunit --slot 11 --inport 1 --outport 1
fi
