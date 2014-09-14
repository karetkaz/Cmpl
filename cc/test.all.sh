#!/bin/sh
 #~ "./tests/*.gxc"
if [ "$#" -eq 0 ]; then
	xterm -e "$0" "test.*.cvx"
	exit
fi

logFile=out/test.all.log

echo>$logFile
for file in $*
do
	echo "**** running test: $file"
	echo>>$logFile "**** running test: $file"
	./main>>$logFile -gd-2 -x -wa $file
	if [ "$?" -ne "0" ]; then
		echo "******** failed: $file"
	fi
done
