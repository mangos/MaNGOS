#!/bin/sh

SYSTEM=`uname -s`
IPCS="ipcs"
IPCRM="ipcrm -s"

if [ "$SYSTEM" = "Darwin" ]; then
  USER=`id | sed 's/(.*//; s/uid=//'`
  IPCS="ngvipc -s"
  IPCRM="ngvipc -s -R"
elif [ -z "$USER" ]; then
  USER=`id | sed 's/).*//; s/.*(//'`
fi


case "$SYSTEM" in
  "Linux" )
    ipcs -a | grep $USER | awk '{ print ($2) }' | xargs -r ipcrm sem;
    ;;
  * )
    semids=`$IPCS | grep "^s" | grep $USER | awk '{ print ($2) }'`
    for p in $semids
      do $IPCRM $p
    done
    ;;
esac
