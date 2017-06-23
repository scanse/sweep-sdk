#!/bin/bash
task="$1"
if [[ "${TRAVIS_OS_NAME}" == "linux" ]]; then
    docker build -t "travis-sweep" .
    docker run -e GRADLE_TASK="$task" "travis-sweep"
else
    pushd jsweep
    ./gradlew "$task"
    popd
fi
