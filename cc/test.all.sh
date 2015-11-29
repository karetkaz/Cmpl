#!/bin/sh

#~ cd tests
#~ files to compile
files="tests/test.*.cvx"

#~ compile flags -apiff -astff -asm39"
#~ flags="-api -ast -asm"
#~ flags="-run"
#~ flags="-debug"
flags="-profile"

logPrefix=out/log
dumpPrefix=out/profile

for file in $files
do
	echo "**** running test: $file"
	testName=${file%.cvx}
	#~ logFile=$logPrefix${testName#test}.log
	logFile=$logPrefix.test.log
	dumpFile=$dumpPrefix${testName#tests/test}.cvx
	dumpFileJs=$dumpPrefix${testName#tests/test}.json
	#~ echo>>$logFile "**** running test: $file"
	#~ ./main -std../stdlib.cvx $flags $file
	#~ ./main -std../stdlib.cvx -dump $dumpFile -log $logFile $flags $file
	./main -log/a $logFile -dump.json $dumpFileJs $flags $file
	if [ "$?" -ne "0" ]; then
		echo "******** failed: $file"
	fi
done
