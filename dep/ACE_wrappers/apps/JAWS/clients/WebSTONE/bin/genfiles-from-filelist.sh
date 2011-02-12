#!/bin/sh
#

# get configuration
[ -n "$WEBSTONEROOT" ] || WEBSTONEROOT=`pwd`/..
. $WEBSTONEROOT/conf/testbed

case $# in
  1)
     FILELIST=$1
     ;;
  *)
     FILELIST=$WEBSTONEROOT/conf/filelist
     ;;
esac

TIMESTAMP=`date +"%y%m%d_%H%M"`
mkdir $TMPDIR/webstone-genfiles.$TIMESTAMP
cd $TMPDIR/webstone-genfiles.$TIMESTAMP

cat $FILELIST |
nawk '
($3 ~ /^\#[0-9]*/) {
  sub(/^\#/, "", $3);
  cmd = WEBSTONEROOT "/bin/genrand " $3 " ."$1
  print cmd;
  system(cmd);
  cmd = RCP " ." $1 " " SERVER ":" WEBDOCDIR
  print cmd;
  system(cmd);
  cmd = "rm -f ." $1
  print cmd;
  system(cmd);
  next
}
' $* WEBSTONEROOT=$WEBSTONEROOT RCP=$RCP SERVER=$SERVER WEBDOCDIR=$WEBDOCDIR

cd $TMPDIR
rm -rf $TMPDIR/webstone-genfiles.$TIMESTAMP

#end
