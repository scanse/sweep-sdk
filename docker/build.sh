#!/bin/bash

set -ex

# set cmake as cmake28
ln -s "$(which cmake28)" /opt/sweep/bin/cmake

gradleTask="$GRADLE_TASK"
if [[ "$gradleTask" == "" ]]; then gradleTask="build"; fi

cd /opt/sweep
pushd libsweep
rm -rf build
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DDUMMY=On
cmake --build .
popd

pushd jsweep
./gradlew "$gradleTask"
popd
