#!/bin/sh
#
# $Id: g++_metric.sh 91813 2010-09-17 07:52:52Z johnnyw $
#
# This simple script is used to gather compile time metrics.  You can use
# it with make like this:
#
#  $ make CXX=g++_metric.sh
#

commandline=$@
# find the target and save it to a variable
until [ -z "$1" ] # test all command line parameters
do
  if [ "-o" = "$1" ]
  then
    shift
    target=$1
    break
  fi
  shift
done

# echo out "(%x)", the return value from g++, so the script processes the output
#  will only use times for successful compilations, i.e., "(0)".
/usr/bin/time -f "//compile time(%x): ${PWD#$ACE_ROOT/}/${target} %U %S" g++ $commandline

retval=$?

exit $retval
