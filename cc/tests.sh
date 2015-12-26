#!/bin/sh

#~ cd tests
#~ files to compile
files="tests/test.*.cvx"

#~ compile flags
flags="-api -ast -asm"
#~ flags="-debug/C/B/L/F"
#~ flags="-debug/v"
flags="-profile"
#~ flags="-run"

outDir=out/test
logFile=$outDir/log.test.log
dumpPrefix=$outDir/profile

rm -f $logFile
mkdir -p $outDir
for file in $files
do
	testName=${file%.cvx}
	dumpFile=$dumpPrefix${testName#tests/test}.cvx
	dumpFileJs=$dumpPrefix${testName#tests/test}.json
	#~ dumpFile=$outDir/${testName#tests/}.cvx

	echo "! running test: $file"
	#~ echo>>$logFile "**** running test: $file"
	#~ ./main -log/a $logFile -dump.json $dumpFileJs $flags $file
	#~ ./main -log/a $logFile -dump $dumpFile $flags $file
	./main -dump.json $dumpFileJs $flags $file
	if [ "$?" -ne "0" ]; then
		echo "$file:1: test failed!"
	fi
done
