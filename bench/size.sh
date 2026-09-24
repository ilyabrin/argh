#!/bin/sh
# Code-size benchmark: .text growth over a parser-free baseline.
# Usage: sh bench/size.sh [compiler]   (default: cc)
set -e
CC=${1:-cc}
DIR=$(cd "$(dirname "$0")" && pwd)
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT
FLAGS="-std=c99 -Os -ffunction-sections -fdata-sections -Wl,--gc-sections"
EXE=
case "$(uname -s)" in
    Darwin) FLAGS="-std=c99 -Os -Wl,-dead_strip" ;;
    MINGW* | MSYS* | CYGWIN*) EXE=.exe ;;
esac

text() {
    t=$(size "$1$EXE" | awk 'NR==2 {print $1}')
    [ -n "$t" ] || { echo "size failed for $1$EXE" >&2; exit 1; }
    echo "$t"
}

for p in none argh getopt; do
    $CC $FLAGS -o "$OUT/$p$EXE" "$DIR/size_$p.c"
done
base=$(text "$OUT/none")
echo "Compiler: $($CC --version | head -n 1)"
echo "Flags:    $FLAGS"
echo
printf '%-14s %10s\n' "parser" ".text added"
printf '%-14s %8s B\n' "argh v0.1" $(( $(text "$OUT/argh") - base ))
printf '%-14s %8s B\n' "getopt_long" $(( $(text "$OUT/getopt") - base ))
