#!/bin/sh

cd tests
#~ files to compile
files="test.*.cvx"

#~ compile flags
flags="-dbg -apiff -astff -asm39"
#~ flags="-run"

logFile=out/dump
#~ echo>$logFile
for file in $files
do
	echo "**** running test: tests/$file"
	#~ echo>>$logFile "**** running test: $file"
	#~ ../main -std../stdlib.cvx -la $logFile $flags $file
	../main -std../stdlib.cvx -log $logFile.$file $flags $file
	#~ ../main -std../stdlib.cvx $flags $file
	if [ "$?" -ne "0" ]; then
		echo "******** failed: $file"
	fi
done
