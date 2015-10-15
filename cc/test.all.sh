#!/bin/sh

cd tests
#~ files to compile
files="test.*.cvx"

#~ compile flags
flags="-gd2 -x -apiff -astff -asm39 -w100"
#~ flags="-gd2 -x -w15"

logFile=out/dump
#~ echo>$logFile
for file in $files
do
	echo "**** running test: $file"
	#~ echo>>$logFile "**** running test: $file"
	#~ ../main -std../stdlib.cvx -la $logFile $flags $file
	../main -std../stdlib.cvx -l $logFile.$file $flags $file
	if [ "$?" -ne "0" ]; then
		echo "******** failed: $file"
	fi
done
