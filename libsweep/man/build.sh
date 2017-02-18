#!/usr/bin/env bash

# Builds man pages from markdown files.
# Usage: ./build.sh page.md page.1

set -o errexit
set -o pipefail
set -o nounset

if [ $# -ne 2 ]; then
    exit 1
fi

pandoc --standalone "${1}" --output "${2}"
