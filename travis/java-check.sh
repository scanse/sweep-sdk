#!/bin/bash
if [[ "${TRAVIS_OS_NAME}" == "linux" ]]; then
    docker build -t "travis-sweep" .
    docker run -e GRADLE_TASK="build" "travis-sweep"
else
    pushd jsweep
    ./gradlew build
    popd
fi
