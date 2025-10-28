# remove current and create new bin directory for horizon-deadlocked-patch/
mkdir ./bin

# run docker and run "docker-init.sh"
docker run -v ${PWD}:/src ps2dev/ps2dev:v1.2.0 /bin/sh -c "cd src; sh docker-init.sh; make clean; make"

