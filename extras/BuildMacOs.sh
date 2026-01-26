#!/bin/bash

export "CMPL_HOME=$(realpath "$(dirname "$(readlink -f "$0")")"/..)"
echo "cmpl home is: $CMPL_HOME"
cd "$CMPL_HOME" || exit 1

CMPL_API="$CMPL_HOME/extras/Cmpl.api"
CMPL_DOC="$CMPL_HOME/extras/Cmpl.md"
CMPL_BIN="$CMPL_HOME/.build"
#BIN_EMC="$CMPL_BIN/wasm"

export BIN="$CMPL_BIN/macos"
#make clean BINDIR="$BIN"
make -j 12 cmpl libFile.dylib libGfx.dylib BINDIR="$BIN"

if [ -n "$BIN_EMC" ]; then
	make -j 12 cmpl.js libFile.wasm libGfx.wasm BINDIR="$BIN_EMC"
	$CMPL_HOME/extras/web/testsGen.sh>"$BIN_EMC/tests.js"
fi

if [ -n "$CMPL_DOC" ]; then
	echo>"$CMPL_DOC"
	DOC_FILES="$CMPL_HOME/extras/docs/*.md"
	printf '%s\n' $DOC_FILES | while read file; do
		cat>>"$CMPL_DOC" "$file"
		echo>>"$CMPL_DOC"
	done
fi

# test the virtual machine
if ! $BIN/cmpl>"$BIN-vm.dump.md" --test-vm; then
	echo "virtual machine test failed"
	exit 1
fi

## dump api for scite including all libraries
if [ -n "$CMPL_API" ] && ! $BIN/cmpl -dump.scite "$CMPL_API" "$BIN/libFile.dylib" "$BIN/libGfx.dylib"; then
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

TEST_FLAGS=-X-stdin+steps-offsets+native
read -rsn1 -p "press R or enter to run tests, C to compile only, any other key to skip testing: "
case "$REPLY" in
	"R" | "r" | "")
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
	elif ! $BIN/cmpl $TEST_FLAGS -log/a/d "$DUMP_FILE" "$BIN/libFile.dylib" "$BIN/libGfx.dylib" "$file"; then
		echo "****** test failed ❌: $file"
#	else
#		echo "**** test finished ✅: $file"
	fi
done

# requirements to build and run on mac (using homebrew)
#brew install sdl2 jpeg-turbo libpng
