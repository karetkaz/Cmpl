#!/bin/sh
if [ "$#" -eq 0 ]; then
	#~ xterm -e 
	"$0" "test.*.cvx"
	exit
fi

logFile=test.all.log
#~ logFile="&2"

echo>$logFile
for file in $*
do
	echo "**** running test: $file"
	echo>>$logFile "**** running test: $file"
	#~ ./main -la $logFile -gd2 -xv -w0 $file
	./main -gd2 -xv -w0 $file
	if [ "$?" -ne "0" ]; then
		echo "******** failed: $file"
	fi
done
