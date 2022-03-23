#!/bin/sh

set -e
set -u

make
logue-cli load --unit osc_add.ntkdigunit --slot 4 --inport 2 --outport 2
