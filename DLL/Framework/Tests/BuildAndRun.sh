#!/bin/sh
# Builds and runs the Framework unit tests with g++ or clang++, proving the core builds without MSVC or Windows.
# BuildAndRun.ps1 is the MSVC counterpart. Needs a C++20 compiler with std::format (g++ 13, Xcode 15).
#
# Run locally:   sh DLL/Framework/Tests/BuildAndRun.sh     (CXX=clang++ picks the compiler)

set -u

cxx="${CXX:-c++}"
tests="$(cd "$(dirname "$0")" && pwd)"
framework="$(dirname "$tests")"
mods="$(dirname "$framework")/Mods"
out="$tests/bin/portable"
mkdir -p "$out/obj"

flags="-std=c++20 -pthread"

echo "=== Building the framework with $cxx ==="
status=0
for src in "$framework"/*.cpp "$mods"/MetronomeSongClock.cpp "$mods"/MetronomeClickMixer.cpp "$mods"/MetronomeCountIn.cpp "$mods"/MetronomeWav.cpp; do
    $cxx $flags -c "$src" -o "$out/obj/$(basename "$src" .cpp).o" || { echo "BUILD FAILED: $src"; status=1; }
done
[ "$status" -eq 0 ] || exit 1

rm -f "$out/libframework.a"
ar rcs "$out/libframework.a" "$out"/obj/*.o

failed=""
for test in "$tests"/*Tests.cpp; do
    name="$(basename "$test" .cpp)"
    echo "=== $name ==="
    if ! $cxx $flags "$test" "$out/libframework.a" -o "$out/$name"; then
        failed="$failed $name(build)"
        continue
    fi
    "$out/$name" || failed="$failed $name(run)"
done

echo
if [ -n "$failed" ]; then
    echo "FRAMEWORK TESTS FAILED:$failed"
    exit 1
fi
echo "ALL FRAMEWORK TESTS PASSED"
