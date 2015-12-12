#!/bin/sh

#~ cd tests
#~ files to compile
files="tests/test.*.cvx"

#~ compile flags -apiff -astff -asm39"
#~ flags="-api -ast -asm"
#~ flags="-run"
#~ flags="-debug"
flags="-profile"

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

	echo "! running test: $file"
	#~ echo>>$logFile "**** running test: $file"
	./main -log/a $logFile -dump.json $dumpFileJs $flags $file
	#~ ./main -dump.json $dumpFileJs $flags $file
	if [ "$?" -ne "0" ]; then
		echo "$file:1: test failed!"
	fi
done
