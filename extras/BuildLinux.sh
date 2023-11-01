#!/bin/bash

export "CMPL_HOME=$(realpath "$(dirname "$(readlink -f "$0")")"/..)"
echo "cmpl home is: $CMPL_HOME"
cd "$CMPL_HOME" || exit 1

BIN=build/linux
BIN_WCC=$BIN.wcc
BIN_EMC=build/wasm

make clean BINDIR="$BIN"
make -j 12 cmpl libFile.so libGfx.so libOpenGL.so BINDIR="$BIN"

if [ -n "$BIN_EMC" ]; then
	make -j 12 cmpl.js libFile.wasm libGfx.wasm BINDIR="$BIN_EMC"
fi

# build and run test on 32 bit platform
WATCOM="$(echo ~/Dropbox/Software/bin/Watcom/rel2)"
if [ -n "$BIN_WCC" ] && [ -d "$WATCOM" ]; then
	export WATCOM="$WATCOM"
	export INCLUDE="$WATCOM/lh"
	export LIB="$WATCOM/lib386"
	PATH="$WATCOM/binl:$PATH"

	mkdir -p $BIN_WCC && cd $BIN_WCC
	owcc -xc -std=c99 -o cmpl $CMPL_HOME/src/*.c $CMPL_HOME/cmplStd/src/*.c
	cd $CMPL_HOME || exit 1
fi

echo>extras/Cmpl.md
DOC_FILES="$CMPL_HOME/extras/docs/*.md"
printf '%s\n' $DOC_FILES | while read file; do
	cat>>extras/Cmpl.md "$file"
	echo>>extras/Cmpl.md
done

# test the virtual machine
$BIN_WCC/cmpl>"$BIN_WCC-vm.dump.md" --test-vm
if ! $BIN/cmpl>"$BIN-vm.dump.md" --test-vm; then
	echo "virtual machine test failed"
	exit 1
fi

## dump api for scite including all libraries
if ! $BIN/cmpl -dump.scite extras/Cmpl.api "$BIN/libFile.so" "$BIN/libGfx.so"; then
	echo "failed to dump compiler api"
	exit 1
fi

TEST_FLAGS="$(echo -X+steps+fold+fast-stdin-glob-offsets -api/A/d/p -asm/n/s -ast -doc -use)"
$BIN_WCC/cmpl -debug/G/M $TEST_FLAGS -log/d/15 "$BIN_WCC.test-dump.ci" "cmplStd/test/test.ci"
if ! $BIN/cmpl -debug/G/M $TEST_FLAGS -log/d/15 "$BIN.test-dump.ci" "cmplStd/test/test.ci"; then
	echo "main test failed: $TEST_FLAGS"
	exit 1
fi

# dump symbols, assembly, syntax tree and global variables
$BIN/cmpl -debug/G/M $TEST_FLAGS -log/d/15 "$BIN.test.ci" -dump.ast.xml "$BIN.test-dump.xml" "$BIN/libFile.so" "$BIN/libGfx.so" "cmplStd/test/test.ci"
# dump profile data in json format
$BIN/cmpl -profile/t/P/G/M $TEST_FLAGS -dump.json "$BIN.test.json" "$BIN/libFile.so" "$BIN/libGfx.so" "cmplStd/test/test.ci"

read -rsn1 -p "Compilation finished, press enter to run tests"
echo
if [ -n "$REPLY" ]; then
	exit 0
fi

TEST_FILES="$CMPL_HOME/cmplStd/test/test.ci"
TEST_FILES="$TEST_FILES $(echo $CMPL_HOME/cmplStd/test/demo/*.ci)"
TEST_FILES="$TEST_FILES $(echo $CMPL_HOME/cmplStd/test/lang/*.ci)"
TEST_FILES="$TEST_FILES $(echo $CMPL_HOME/cmplStd/test/math/*.ci)"
TEST_FILES="$TEST_FILES $(echo $CMPL_HOME/cmplStd/test/text/*.ci)"
TEST_FILES="$TEST_FILES $(echo $CMPL_HOME/cmplStd/test/time/*.ci)"

TEST_FILES="$TEST_FILES $(echo $CMPL_HOME/cmplGfx/test/*.ci)"
TEST_FILES="$TEST_FILES $(echo $CMPL_HOME/cmplGfx/test/demo/*.ci)"
TEST_FILES="$TEST_FILES $(echo $CMPL_HOME/cmplGfx/test/demo.procedural/*.ci)"
TEST_FILES="$TEST_FILES $(echo $CMPL_HOME/cmplGfx/test/demo.widget/*.ci)"

TEST_FILES="$TEST_FILES $(echo $CMPL_HOME/cmplFile/test/*.ci)"
TEST_FILES="$TEST_FILES $(echo $CMPL_HOME/temp/cmplGfx/demo/*.ci)"

BIN="$CMPL_HOME/$BIN"
DUMP_FILE=$BIN-tests.exec.ci
$BIN/cmpl -log/d "$DUMP_FILE"
for file in $TEST_FILES
do
	if ! cd "$(dirname "$file")"; then
		echo "**** cannot run test ❌: $file"
		continue
	fi
	printf "**** test: %s\\r" "$file"
	if ! $BIN/cmpl -X-stdin+steps -run -log/a/d "$DUMP_FILE" "$BIN/libFile.so" "$BIN/libGfx.so" "$file"; then
		echo "****** test failed ❌: $file"
	else
		echo "**** test finished ✅: $file"
	fi
done

TEST_FILES="$(echo $CMPL_HOME/cmplGL/test/*.ci)"
for file in $TEST_FILES
do
	if ! cd "$(dirname "$file")"; then
		echo "**** cannot run test ❌: $file"
		continue
	fi
	printf "**** test: %s\\r" "$file"
	if ! $BIN/cmpl -X-stdin+steps -run -log/a/d "$DUMP_FILE" "$BIN/libFile.so" "$BIN/libOpenGL.so" "$file"; then
		echo "****** test failed ❌: $file"
	else
		echo "**** test finished ✅: $file"
	fi
done

# requirements to build and run on linux (ubuntu based distros)
# sudo apt install build-essential libjpeg-dev libpng-dev libsdl2-dev freeglut3-dev
