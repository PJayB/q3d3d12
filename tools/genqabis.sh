#!/bin/bash
# todo: Delete me and put me in CMake
die() { echo "$*" >&2 ; exit 1 ; }

here="$(realpath "$(dirname "$0")")" || die "Can't determine current location"
input_dir="$(realpath "$here/../code/qabi")" || die "Missing input path"
output_dir="$(realpath -m "$here/../build/qabi")" || \
    die "Can't construct output path"
qabigen_exe="$here/QAbiGen.exe"
source_files=(
    "$here/QAbiGen/Program.cs"
)

mkdir -p "$output_dir"

mcs -out:"$qabigen_exe" "${source_files[@]}" || die "Failed to compile C#"

qabigen() {
    mono "$qabigen_exe" "$@"
}

get_missing_symbol() {
    sed -nr 's/.* Unknown data type: (.*)/\1/p'
}

build_qabi() {
    local input="$1"
    local output="$2"
    compile_messages="$(qabigen "$input" -o "$output" "${@:3}" 2>&1)" && return 0
    symbol="$(echo "$compile_messages" | get_missing_symbol)"
    if [ -n "$symbol" ]; then
        grep -rE "typedef .* $symbol" "$here/../code"
    fi
    return 1
}

for qabi in $input_dir/*.qabi; do
    build_qabi "$qabi" "$output_dir/" "$@" || die "Failed to compile QABI"
done
