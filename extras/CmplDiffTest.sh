#!/bin/bash

if [ ! -d "$BIN" ] && [ -f "$CMPL_HOME/.build/cmake-build-release/Cmpl" ]; then
	BIN="$CMPL_HOME/.build/cmake-build-release"
	echo "using cmpl from folder: $BIN"
fi
if [ ! -d "$BIN" ] && [ -f "$CMPL_HOME/.build/cmake-build-debug/Cmpl" ]; then
	BIN="$CMPL_HOME/.build/cmake-build-debug"
	echo "using cmpl from folder: $BIN"
fi
if [ ! -d "$BIN" ]; then
	echo "cmpl binary not detected"
	exit 1
fi

CMPL_FLAGS="-X-offsets-steps"

#this prints all arguments
if [ $# -gt 2 ]; then
	while test $# -gt 2
	do
		CMPL_FLAGS="$CMPL_FLAGS $1"
		shift
	done

	file=$(basename -- "$2")
	lines="$1"
else
	CMPL_FLAGS="$CMPL_FLAGS -asm/s/0 -run/g"
	if [ $# -gt 1 ]; then
		file=$(basename -- "$2")
		lines="$1"
	else
		file=$(basename -- "$1")
		lines=0
	fi
fi

if ! cd "$(dirname "$file")"; then
	exit 1
fi

if [ "$lines" -lt 0 ]; then
	diff>"$file.patch" -u -L "$file" -L "$file"  <(tail -"$((-lines+2))" "$file") <(echo "/* - expected output:" ; $BIN/cmpl $CMPL_FLAGS "$file" ; echo "*/")
	echo "patch file created: $file.patch"
	exit 1
fi

if ! [ "$lines" -gt 0 ]; then
	echo "/* - expected output:"
	$BIN/cmpl $CMPL_FLAGS "$file"
	echo "*/"
	exit 1
fi

diff -y -W 160 --suppress-common-lines <(tail -"$((lines+2))" "$file") <(echo "/* - expected output:" ; $BIN/cmpl $CMPL_FLAGS "$file" ; echo "*/")
#echo "expected:" && echo "/* - expected output:" ; $BIN/Cmpl $CMPL_FLAGS "$file" ; echo "*/"
#echo "returned:" && tail -"$((lines+2))" "$file"

exit $?
