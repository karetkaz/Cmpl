#!/bin/sh

#~ cd tests
#~ files to compile
files="tests/test.*.cvx"

#~ compile flags -apiff -astff -asm39"
#~ flags="-api -ast -asm"
#~ flags="-run"
#~ flags="-debug"
flags="-profile"

logFile=out/log.test.log
dumpPrefix=out/profile

cat>$logFile
for file in $files
do
	testName=${file%.cvx}
	dumpFile=$dumpPrefix${testName#tests/test}.cvx
	dumpFileJs=$dumpPrefix${testName#tests/test}.json

	echo "!** running test: $file"
	#~ echo>>$logFile "**** running test: $file"
	./main -log/a $logFile -dump.json $dumpFileJs $flags $file
	#~ ./main -dump.json $dumpFileJs $flags $file
	if [ "$?" -ne "0" ]; then
		echo ">** test failed: $file"
	fi
done
