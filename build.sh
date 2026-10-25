#!/bin/bash
set -e

# Check if bin directory exists, if not create it
if [ ! -d bin ]; then
  mkdir bin
fi

# run docker and run "docker-init.sh"
docker run --platform linux/amd64 -v "${PWD}":/src ps2dev/ps2dev:v1.2.0 /bin/sh -c "cd src; sh docker-init.sh; make clean; make"

# docker run -it --platform linux/amd64 -v "${PWD}":/src ps2dev/ps2dev:v1.2.0
