#!/bin/bash

if [ ! -d "$CMPL_BIN" ] && [ -f "$CMPL_HOME/.build/cmake-build-release/cmpl" ]; then
	CMPL_BIN="$CMPL_HOME/.build/cmake-build-release"
	echo "using cmpl from folder: $CMPL_BIN"
fi
if [ ! -d "$CMPL_BIN" ] && [ -f "$CMPL_HOME/.build/cmake-build-debug/cmpl" ]; then
	CMPL_BIN="$CMPL_HOME/.build/cmake-build-debug"
	echo "using cmpl from folder: $CMPL_BIN"
fi
if [ ! -d "$CMPL_BIN" ]; then
	echo "cmpl binary not detected"
	exit 1
fi

MARKER="/* - expected output:"
CMPL_FLAGS="-X-offsets-steps-warn"

if [ $# -eq 0 ]; then
	echo "usage: $(basename "$0") [cmpl-flags...] <file>"
	exit 1
fi

# no cmpl flags given (just the file): fall back to a plain run
if [ $# -eq 1 ]; then
	CMPL_FLAGS="$CMPL_FLAGS -asm/s/0 -run/g/t"
else
	while [ $# -gt 1 ]; do
		CMPL_FLAGS="$CMPL_FLAGS $1"
		shift
	done
fi

file=$(basename -- "$1")
if ! cd "$(dirname -- "$1")"; then
	exit 1
fi

# locate the trailing `/* - expected output: ... */` comment: everything from
# there to the end of the file is compared against a fresh run, so there is
# no line count to keep in sync by hand anymore.
markerLine=$(grep -n -F -x "$MARKER" "$file" | tail -1 | cut -d: -f1)
if [ -z "$markerLine" ]; then
	echo "no '$MARKER' block found in '$file'" >&2
	echo "add one (even empty, terminated with '*/') and run CmplDiffPatch.sh to fill it in" >&2
	exit 1
fi

diff -y -W 160 --suppress-common-lines <(tail -n +"$markerLine" "$file") <(echo "$MARKER" ; $CMPL_BIN/cmpl $CMPL_FLAGS "$file" ; echo "*/")
exit $?
