#!/bin/sh

cd tests
#~ files to compile
files="test.*.cvx"

#~ compile flags
flags="-gd2 -x -astff -asm39 -w9"
flags="-gd2 -x -w0"

logFile=../out/log.gcc.cvx

echo>$logFile
for file in $files
do
	echo "**** running test: $file"
	echo>>$logFile "**** running test: $file"
	../main -std../stdlib.cvx -la $logFile $flags $file
	if [ "$?" -ne "0" ]; then
		echo "******** failed: $file"
	fi
done
