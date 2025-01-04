#!/bin/bash

if [ ! -d "$BIN" ] && [ -d "$CMPL_HOME/.build/cmake-build-release" ]; then
	BIN="$CMPL_HOME/.build/cmake-build-release"
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
	CMPL_FLAGS="$CMPL_FLAGS -asm/0 -run"
	if [ -z "$2" ]; then
		file=$(basename -- "$1")
		lines=0
	else
		file=$(basename -- "$2")
		lines="$1"
	fi
fi

if ! cd "$(dirname "$file")"; then
	exit 1
fi

if ! [ "$lines" -gt "0" ]; then
	$BIN/Cmpl $CMPL_FLAGS "$file"
	exit 1
fi

diff -y --suppress-common-lines <(tail -"$((lines+2))" "$file") <(echo "/* - expected output:" ; $BIN/cmpl $CMPL_FLAGS "$file" ; echo "*/")
#diff -y --suppress-common-lines <(tail -"$((lines+1))" "$file" | head -"$lines") <($BIN/Cmpl $CMPL_FLAGS "$file")
#diff -y <(tail -"$((lines+1))" "$file" | head -"$lines") <($BIN/Cmpl $CMPL_FLAGS "$file")

#echo "expected:" && echo "/* - expected output:" ; $BIN/Cmpl $CMPL_FLAGS "$file" ; echo "*/"
#echo "returned:" && tail -"$((lines+2))" "$file"

exit $?
