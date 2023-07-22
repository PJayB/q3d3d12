#!/bin/bash
here="$(realpath "$(dirname "$0")")"
echo "cd '${here}' && cmake -S . -B build --preset mingw64 && cmake --build build" | \
    podman run -i "-v$here:$here" q3build /bin/bash
