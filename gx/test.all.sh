#!/bin/sh
 #~ "./tests/*.gxc"
if [ "$#" -eq 0 ]; then
	xterm -e "$0" "./*.gxc ./tests/*.gxc"
	exit
fi
logFile=out/test.all.log
echo>$logFile
for file in $*
do
	echo "**** running test: $file"
	echo>>$logFile "**** running test: $file"
	./main>>$logFile -s "$file" arg1 arg2
	if [ "$?" -ne "0" ]; then
		echo "******** failed: $file"
	fi
done
