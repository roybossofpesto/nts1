#!/bin/sh

set -e
set -u

make

alias logue-cli="/home/pgueth/git/nts1/tools/logue-cli/logue-cli-linux64-0.07-2b/logue-cli"


if [ "${OS:-""}" = "Windows_NT" ]
then
  logue-cli load --unit kick.ntkdigunit --slot 1 --inport 0 --outport 1
else
  logue-cli load --unit kick.ntkdigunit --slot 1 --inport 2 --outport 2
fi
