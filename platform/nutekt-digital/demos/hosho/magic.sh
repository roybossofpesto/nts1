#!/bin/sh

set -e
set -u

make
logue-cli load --unit hosho.ntkdigunit --slot 2 --inport 4 --outport 4
