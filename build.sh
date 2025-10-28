#!/bin/bash
set -e

# Check if bin directory exists, if not create it, otherwise clear its contents
if [ -d bin ]; then
  # Directory exists, so delete all files inside it
  rm -rf bin/*
else
  # Directory doesn't exist, so create it
  mkdir bin
fi

# run docker and run "docker-init.sh"
docker run -v "${PWD}":/src ps2dev/ps2dev:v1.2.0 /bin/sh -c "cd src; sh docker-init.sh; make clean; make"