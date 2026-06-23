#!/bin/bash
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
# mozjs builds first: its configure generates obj-switch/build/rust/mozbuild/
# buildconfig.rs and populates the cargo registry that jsrust depends on.
echo "############ Stage 1: mozjs C++ (-fPIC, no-ssp) ##"
bash "$HERE/build-mozjs.sh"
echo "############ Stage 2: jsrust (no_std) ############"
bash "$HERE/build-jsrust.sh"
echo "############ Stage 3: build test NROs ###########"
bash "$HERE/make-nro.sh"
echo "############ DONE ###############################"
