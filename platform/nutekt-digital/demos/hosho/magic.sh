#!/bin/sh

set -e
set -u

make
logue-cli load --unit hosho.ntkdigunit --slot 2 --inport 2 --outport 2
