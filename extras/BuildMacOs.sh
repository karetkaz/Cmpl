#!/bin/bash

export "CMPL_HOME=$(realpath "$(dirname "$(readlink -f "$0")")"/..)"
echo "cmpl home is: $CMPL_HOME"
cd "$CMPL_HOME" || exit 1

export CMPL_BIN="$(realpath ~)/Workspace/build/Cmpl"
export CMPL_BIN="$CMPL_HOME/.build/macos"

CMPL_API="$CMPL_HOME/extras/Cmpl.api"
CMPL_DOC="$CMPL_HOME/extras/Cmpl.md"
BIN_EMC="$CMPL_HOME/.build/wasm"

#make clean BINDIR="$CMPL_BIN"
make -j 12 cmpl libFile.dylib libGfx.dylib BINDIR="$CMPL_BIN" || exit 1

if [ -n "$BIN_EMC" ]; then
	make -j 12 cmpl.js libFile.wasm libGfx.wasm BINDIR="$BIN_EMC" || exit 1
	$CMPL_HOME/extras/web/testsGen.sh>"$BIN_EMC/../tests.js"
fi

if [ -n "$CMPL_DOC" ] && ! $CMPL_BIN/cmpl -doc/a -dump.md "$CMPL_DOC" "$CMPL_BIN/libFile.dylib" "$CMPL_BIN/libGfx.dylib" "cmplDoc/Cmpl.cmpl"; then
	echo "failed to generate documentation"
	exit 1
fi

if ! $CMPL_BIN/cmpl -profile -ast -asm -doc/a -dump.json "$CMPL_BIN/../cmpl.json" "$CMPL_BIN/libFile.dylib" "$CMPL_BIN/libGfx.dylib" "cmplDoc/Cmpl.cmpl"; then
	echo "failed to generate json export"
	exit 1
fi

## dump api for scite including all libraries
if [ -n "$CMPL_API" ] && ! $CMPL_BIN/cmpl -dump.scite "$CMPL_API" "$CMPL_BIN/libFile.dylib" "$CMPL_BIN/libGfx.dylib"; then
	echo "failed to dump compiler api"
	exit 1
fi

TEST_FLAGS="$(echo -X+steps+fold+fast-stdin-glob-offsets -api/A/d/p -asm/n/s -ast -doc -use)"
if ! $CMPL_BIN/cmpl -debug/G/M $TEST_FLAGS -log/d/15 "$CMPL_BIN/test-std.cmpl" "cmplStd/test/test.cmpl"; then
	echo "main test failed: $TEST_FLAGS"
	exit 1
fi

# dump symbols, assembly, syntax tree and global variables
$CMPL_BIN/cmpl -debug/G/M $TEST_FLAGS -log/d/15 "$CMPL_BIN/test.cmpl" -dump.ast.xml "$CMPL_BIN/test.xml" "$CMPL_BIN/libFile.dylib" "$CMPL_BIN/libGfx.dylib" "cmplStd/test/test.cmpl"
$CMPL_BIN/cmpl -X-offsets -api/A/d/p -doc -log/d "$CMPL_BIN/test.api.cmpl" "$CMPL_BIN/libFile.dylib" "$CMPL_BIN/libGfx.dylib" "cmplStd/test/test.cmpl"
$CMPL_BIN/cmpl -X-offsets -asm/A/d/p/n/s -log/d "$CMPL_BIN/test.asm.cmpl" "$CMPL_BIN/libFile.dylib" "$CMPL_BIN/libGfx.dylib" "cmplStd/test/test.cmpl"
$CMPL_BIN/cmpl -profile/t/P/G/M $TEST_FLAGS -dump.json "$CMPL_BIN/test.json" "$CMPL_BIN/libFile.dylib" "$CMPL_BIN/libGfx.dylib" "cmplStd/test/test.cmpl"

TEST_FLAGS=-X-stdin+steps-offsets+native
read -rsn1 -p "press T or enter to run in test mode, R for normal run, C to compile only, any other key to skip testing: "
case "$REPLY" in
	"T" | "t" | "")
		TEST_FLAGS="$TEST_FLAGS+test -run"
		echo "run in test mode"
		;;

	"R" | "r")
		TEST_FLAGS="$TEST_FLAGS -run"
		echo "run tests"
		;;

	"C" | "c")
		echo "compile tests"
		;;

	*)
		echo "skip tests"
		exit 0
		;;
esac

TEST_FILES="$CMPL_HOME/cmplFile/test/*.cmpl"

TEST_FILES="$TEST_FILES $(echo $CMPL_HOME/cmplStd/test/*.cmpl)"
TEST_FILES="$TEST_FILES $(echo $CMPL_HOME/cmplStd/test/**/*.cmpl)"
TEST_FILES="$TEST_FILES $(echo $CMPL_HOME/wip/cmplStd/test/*.cmpl)"
TEST_FILES="$TEST_FILES $(echo $CMPL_HOME/wip/cmplStd/test/**/*.cmpl)"
TEST_FILES="$TEST_FILES $(echo $CMPL_HOME/cmplStd/test/extra/**/*.cmpl)"

TEST_FILES="$TEST_FILES $(echo $CMPL_HOME/cmplGfx/test/*.cmpl)"
TEST_FILES="$TEST_FILES $(echo $CMPL_HOME/cmplGfx/test/**/*.cmpl)"
TEST_FILES="$TEST_FILES $(echo $CMPL_HOME/wip/cmplGfx/test/*.cmpl)"
TEST_FILES="$TEST_FILES $(echo $CMPL_HOME/wip/cmplGfx/test/**/*.cmpl)"

DUMP_FILE="$CMPL_BIN/tests.exec.cmpl"
$CMPL_BIN/cmpl -log/d "$DUMP_FILE"
for file in $TEST_FILES
do
	if ! cd "$(dirname "$file")"; then
		echo "**** cannot run test ❌: $file"
		continue
	fi
	printf "**** test: %-120s\\r" "$file"
	if [[ "$(head -n 1 "$file")" == "#!/usr/bin/env -S \${CMPL_HOME}/extras/CmplDiffTest.sh"* ]]; then
		if ! "$file"; then echo "****** test failed ❌: $file"; fi
		continue
	elif ! $CMPL_BIN/cmpl $TEST_FLAGS -log/a/d "$DUMP_FILE" "$CMPL_BIN/libFile.dylib" "$CMPL_BIN/libGfx.dylib" "$file"; then
		echo "****** test failed ❌: $file"
#	else
#		echo "**** test finished ✅: $file"
	fi
done

# requirements to build and run on mac (using homebrew)
#brew install sdl2 jpeg-turbo libpng
