#!/bin/bash
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
echo "############ Stage 1: jsrust (no_std) ############"
bash "$HERE/build-jsrust.sh"
echo "############ Stage 2: mozjs C++ (-fPIC, no-ssp) ##"
bash "$HERE/build-mozjs.sh"
echo "############ Stage 3: build test NROs ###########"
bash "$HERE/make-nro.sh"
echo "############ DONE ###############################"
