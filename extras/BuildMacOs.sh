#!/bin/bash

export "CMPL_HOME=$(realpath "$(dirname "$(readlink -f "$0")")"/..)"
echo "cmpl home is: $CMPL_HOME"
cd "$CMPL_HOME" || exit 1

export BIN="$CMPL_HOME/.build/macos"
BIN_EMC=.build/wasm

#make clean BINDIR="$BIN"
make -j 12 cmpl libFile.dylib libGfx.dylib BINDIR="$BIN"

if [ -n "$BIN_EMC" ]; then
	make -j 12 cmpl.js libFile.wasm libGfx.wasm BINDIR="$BIN_EMC"
	$CMPL_HOME/extras/web/testsGen.sh>"$BIN_EMC/tests.js"
fi

echo>extras/Cmpl.md
DOC_FILES="$CMPL_HOME/extras/docs/*.md"
printf '%s\n' $DOC_FILES | while read file; do
	cat>>extras/Cmpl.md "$file"
	echo>>extras/Cmpl.md
done

# test the virtual machine
if ! $BIN/cmpl>"$BIN-vm.dump.md" --test-vm; then
	echo "virtual machine test failed"
	exit 1
fi

## dump api for scite including all libraries
if ! $BIN/cmpl -dump.scite extras/Cmpl.api "$BIN/libFile.dylib" "$BIN/libGfx.dylib"; then
	echo "failed to dump compiler api"
	exit 1
fi

TEST_FLAGS="$(echo -X+steps+fold+fast-stdin-glob-offsets -api/A/d/p -asm/n/s -ast -doc -use)"
if ! $BIN/cmpl -debug/G/M $TEST_FLAGS -log/d/15 "$BIN.test-std.cmpl" "cmplStd/test/test.cmpl"; then
	echo "main test failed: $TEST_FLAGS"
	exit 1
fi

# dump symbols, assembly, syntax tree and global variables
$BIN/cmpl -debug/G/M $TEST_FLAGS -log/d/15 "$BIN.test.cmpl" -dump.ast.xml "$BIN.test.xml" "$BIN/libFile.dylib" "$BIN/libGfx.dylib" "cmplStd/test/test.cmpl"
$BIN/cmpl -X-offsets -api/A/d/p -doc -log/d "$BIN.test.api.cmpl" "$BIN/libFile.dylib" "$BIN/libGfx.dylib" "cmplStd/test/test.cmpl"
$BIN/cmpl -X-offsets -asm/A/d/p/n/s -log/d "$BIN.test.asm.cmpl" "$BIN/libFile.dylib" "$BIN/libGfx.dylib" "cmplStd/test/test.cmpl"
$BIN/cmpl -profile/t/P/G/M $TEST_FLAGS -dump.json "$BIN.test.json" "$BIN/libFile.dylib" "$BIN/libGfx.dylib" "cmplStd/test/test.cmpl"

read -rsn1 -p "Compilation finished, press enter to run tests" && echo
if [ -n "$REPLY" ]; then
	exit 0
fi

TEST_FILES="$CMPL_HOME/cmplFile/test/*.cmpl"

TEST_FILES="$TEST_FILES $(echo $CMPL_HOME/cmplStd/test/*.cmpl)"
TEST_FILES="$TEST_FILES $(echo $CMPL_HOME/cmplStd/test/**/*.cmpl)"
TEST_FILES="$TEST_FILES $(echo $CMPL_HOME/cmplStd/tmp/test/*.cmpl)"
TEST_FILES="$TEST_FILES $(echo $CMPL_HOME/cmplStd/tmp/test/**/*.cmpl)"
TEST_FILES="$TEST_FILES $(echo $CMPL_HOME/cmplStd/test/extra/**/*.cmpl)"

TEST_FILES="$TEST_FILES $(echo $CMPL_HOME/cmplGfx/test/*.cmpl)"
TEST_FILES="$TEST_FILES $(echo $CMPL_HOME/cmplGfx/test/**/*.cmpl)"
TEST_FILES="$TEST_FILES $(echo $CMPL_HOME/cmplGfx/tmp/test/*.cmpl)"
TEST_FILES="$TEST_FILES $(echo $CMPL_HOME/cmplGfx/tmp/test/**/*.cmpl)"

DUMP_FILE=$BIN-tests.exec.cmpl
$BIN/cmpl -log/d "$DUMP_FILE"
for file in $TEST_FILES
do
	if ! cd "$(dirname "$file")"; then
		echo "**** cannot run test ❌: $file"
		continue
	fi
	printf "**** test: %-99s\\r" "$file"
	if [[ "$(head -n 1 "$file")" == "#!/usr/bin/env -S \${CMPL_HOME}/extras/CmplDiffTest.sh"* ]]; then
		if ! "$file"; then
			echo "****** test failed ❌: $file"
		fi
		continue
	elif ! $BIN/cmpl -X-stdin+steps-offsets+native -run -log/a/d "$DUMP_FILE" "$BIN/libFile.dylib" "$BIN/libGfx.dylib" "$file"; then
		echo "****** test failed ❌: $file"
#	else
#		echo "**** test finished ✅: $file"
	fi
done

# requirements to build and run on mac (using homebrew)
#brew install sdl2 jpeg-turbo libpng
