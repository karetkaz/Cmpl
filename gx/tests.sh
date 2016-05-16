#!/bin/sh

logFile=out/test.all.log
echo>$logFile
for file in ./tests/*.gxc
do
	echo "**** running test: $file"
	echo>>$logFile "**** running test: $file"
	./main>>$logFile -s "$file" arg1 arg2
	if [ "$?" -ne "0" ]; then
		echo "******** failed: $file"
	fi
done
