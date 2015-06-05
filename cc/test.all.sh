#!/bin/sh

#~ files to compile
files="test.*.cvx"
#~ files="test.it.files.cvx"

#~ compile flags
#~ TODO: flags=-gd2 -x -w0

logFile=out/log.gcc.cvx

echo>$logFile
for file in $files
do
	echo "**** running test: $file"
	echo>>$logFile "**** running test: $file"
	./main -la $logFile -gd2 -asm -ast -xv -w0 $file
	if [ "$?" -ne "0" ]; then
		echo "******** failed: $file"
	fi
done
