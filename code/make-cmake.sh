#!/bin/bash
lib="$1"

cmake_file="$1/CMakeLists.txt"

generate_cmake() {
    echo "add_library($1"
    echo "    STATIC"

    find "$1" \( -name '*.cpp' -or -name '*.c' -or -name '*.h' \) \
        -printf '    %p\n' | sed "s:$1/::g" | sort

    echo ")"
}

generate_cmake "$1" > "$cmake_file"
code "$cmake_file"
