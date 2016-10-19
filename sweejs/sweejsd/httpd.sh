#!/usr/bin/env bash

set -o errexit
set -o pipefail
set -o nounset

readonly port=${1:-8080}

python3 -m http.server 8080
