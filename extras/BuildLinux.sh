#!/bin/bash

export "CMPL_HOME=$(realpath "$(dirname "$(readlink -f "$0")")"/..)"
echo "cmpl home is: $CMPL_HOME"
cd "$CMPL_HOME" || exit 1

export CMPL_BIN="$CMPL_HOME/.build/linux"
export CMPL_BIN="$(realpath ~)/Desktop/Cmpl"

CMPL_API="$CMPL_BIN/Cmpl.api"
CMPL_DOC="$CMPL_BIN/Cmpl.md"
#BIN_EMC="$CMPL_BIN/wasm"
#BIN_WCC="$CMPL_BIN/wcc"

#make clean BINDIR="$CMPL_BIN"
make -j 12 cmpl libFile.so libGfx.so BINDIR="$CMPL_BIN"

if [ -n "$BIN_EMC" ]; then
	make -j 12 cmpl.js libFile.wasm libGfx.wasm BINDIR="$BIN_EMC"
	$CMPL_HOME/extras/web/testsGen.sh>"$BIN_EMC/tests.js"
fi

# build and run test on 32 bit platform
WATCOM="$(echo ~/bin/Apps/ow_daily/rel2/)"
if [ -n "$BIN_WCC" ] && [ -d "$WATCOM" ]; then
	export WATCOM="$WATCOM"
	export INCLUDE="$WATCOM/lh:$CMPL_HOME/include"
	export LIB="$WATCOM/lib386"
	PATH="$WATCOM/binl:$PATH"

	obj_files=""
	for src in "$CMPL_HOME/src"/*.c "$CMPL_HOME/src/os_etc/unknown.c" "$CMPL_HOME/cmplStd/src"/*.c
	do
		obj="${src#"$CMPL_HOME/"}"
		obj="$BIN_WCC/${obj%.c}.o"
		obj_files="$obj_files $obj"
		mkdir -p "$(dirname "$obj")"
		wcc386 -q -za99 -fr -fo="$obj" "$src"
	done
	wcl386 -q -fe=$BIN_WCC/cmpl "$obj_files"
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
[ -f $BIN_WCC/cmpl ] && $BIN_WCC/cmpl>"$BIN_WCC/vm.dump.md" --test-vm
if ! $CMPL_BIN/cmpl>"$CMPL_BIN/vm.dump.md" --test-vm; then
	echo "virtual machine test failed"
	exit 1
fi

## dump api for scite including all libraries
if [ -n "$CMPL_API" ] && ! $CMPL_BIN/cmpl -dump.scite "$CMPL_API" "$CMPL_BIN/libFile.so" "$CMPL_BIN/libGfx.so"; then
	echo "failed to dump compiler api"
	exit 1
fi

TEST_FLAGS="$(echo -X+steps+fold+fast-stdin-glob-offsets -api/A/d/p -asm/n/s -ast -doc -use)"
[ -f $BIN_WCC/cmpl ] && $BIN_WCC/cmpl -debug/G/M $TEST_FLAGS -log/d/15 "$BIN_WCC.test-dump.cmpl" "cmplStd/test/test.cmpl"
if ! $CMPL_BIN/cmpl -debug/G/M $TEST_FLAGS -log/d/15 "$CMPL_BIN/test-std.cmpl" "cmplStd/test/test.cmpl"; then
	echo "main test failed: $TEST_FLAGS"
	exit 1
fi

# dump symbols, assembly, syntax tree and global variables
$CMPL_BIN/cmpl -debug/G/M $TEST_FLAGS -log/d/15 "$CMPL_BIN/test.cmpl" -dump.ast.xml "$CMPL_BIN/test.xml" "$CMPL_BIN/libFile.so" "$CMPL_BIN/libGfx.so" "cmplStd/test/test.cmpl"
$CMPL_BIN/cmpl -X-offsets -api/A/d/p -doc -log/d "$CMPL_BIN/test.api.cmpl" "$CMPL_BIN/libFile.so" "$CMPL_BIN/libGfx.so" "cmplStd/test/test.cmpl"
$CMPL_BIN/cmpl -X-offsets -asm/A/d/p/n/s -log/d "$CMPL_BIN/test.asm.cmpl" "$CMPL_BIN/libFile.so" "$CMPL_BIN/libGfx.so" "cmplStd/test/test.cmpl"
$CMPL_BIN/cmpl -profile/t/P/G/M $TEST_FLAGS -dump.json "$CMPL_BIN/test.json" "$CMPL_BIN/libFile.so" "$CMPL_BIN/libGfx.so" "cmplStd/test/test.cmpl"

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
	elif ! $CMPL_BIN/cmpl $TEST_FLAGS -log/a/d "$DUMP_FILE" "$CMPL_BIN/libFile.so" "$CMPL_BIN/libGfx.so" "$file"; then
		echo "****** test failed ❌: $file"
#	else
#		echo "**** test finished ✅: $file"
	fi
done

# requirements to build and run on linux (debian based)
# sudo apt install build-essential libjpeg-dev libpng-dev libsdl2-dev freeglut3-dev
