#!/bin/sh

#~ rebuild project
make -C src clean build

#~ cd tests
#~ files to compile
files="tests/test.*.cvx"

#~ compile flags
flags="-run"
flags="-debug"
flags="-profile"

outDir=out/test
logFile=$outDir/tests.log
apiFile=$outDir/dump.api.cvx
apiFileJs=$outDir/dump.api.json
dumpPrefix=$outDir/prof

rm -f $logFile
mkdir -p $outDir

#~ export api
./main -debug/L/p/G/h -api/a/d/p/u -asm/a/n/s -ast -dump $apiFile
./main -api -ast -asm -dump.json $apiFileJs

#~ run each test
for file in $files
do
	testName=${file%.cvx}
	dumpFile=$dumpPrefix${testName#tests/test}.cvx
	dumpFileJs=$dumpPrefix${testName#tests/test}.json
	#~ dumpFile=$outDir/${testName#tests/}.cvx

	echo "! running test: $file"
	#~ echo>>$logFile "**** running test: $file"
	./main -log/a $logFile -dump.json $dumpFileJs $flags $file
	#~ ./main -log/a $logFile -dump $dumpFile $flags $file
	#~ ./main -dump.json $dumpFileJs $flags $file
	if [ "$?" -ne "0" ]; then
		echo "$file:1: test failed!"
	fi
done
