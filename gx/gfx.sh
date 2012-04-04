#!/bin/sh

cd "$(dirname "$(readlink -f "$0")")"

if [ ! -x main -a -e main ]
then
	chmod +x ./main
fi

./main "$@"
#~ xfce4-terminal -H -x ./main "$@"
#~ xterm -hold -e ./main "$@"
