#!/bin/sh
set -eu
cd "$(dirname "$0")"
make all
exec ./build/esaExample "$@"
