#!/bin/sh

set -e
set -u

make
logue-cli load --unit patate.ntkdigunit --slot 1 --inport 2 --outport 2

