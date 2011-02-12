#!/bin/sh
# This script assumes it is being run by bash

# ======= needed functions ======
# abspath relpath curentpath
abspath ()
{
# treat "./" as a special case
if expr "$1" : '\./' >/dev/null; then
  b=`echo $1 | sed 's@\./@@'`
  echo $2/$b
  return 0
fi
b=$1
a=$2
# for each "../" we remove one directory from the current path
# and leading "../" from the relative path
# until we have the unique part in b and the abs prefix in a
while expr "$b" : '\.\./' >/dev/null
do
  b=`echo $b | sed 's@\.\./@@'`
  a=`echo $a | sed 's@/[^/]*$@@'`
done
# return the completed absolute path
echo $a/$b
}

# relpath abspath curentpath
relpath ()
{
# take "/" off beginning
a=`echo $1 | sed 's@^/@@'`
# take "/" off beginning and add to end
b=`echo $2 | sed 's@^/@@;s@$@/@'`
while true
do
  if [ "$b" = "" ]; then
    break;
  fi
  a1=`echo $a | sed 's@\([^/]*\)/.*@\1@'`
  b1=`echo $b | sed 's@\([^/]*\)/.*@\1@'`
  if [ "$a1" != "$b1" ]; then
    break;
  fi
  a=`echo $a | sed 's@[^/]*/@@'`
  b=`echo $b | sed 's@[^/]*/@@'`
done
# a now has the unique part of the path
c=""
# c will have the required number of "../"'s
while [ "$b" != "" ]
do
  c="../$c"
  b=`echo $b | sed 's@[^/]*/@@'`
done
# return the completed relative path
echo "$c$a"
}


# ====== MAIN ======
# Assume any relative path passed in is relative to the current directory
# Given $1 is a path to the source file
# Given $2 is a path of the link to be created
# Create a link that has the relative path to the source file
# That is, $1 converted relative to $2
# Check if $1 is absolute or already relative
#echo add_rel_link.sh $1 $2
if expr "$1" : '\/' >/dev/null; then
  # The source path is absolute, this is the expected case
  # Check if $2 is absolute or relative
  if expr "$2" : '\/' >/dev/null; then
    # The link path is already absolute, so just use it
    lpath=$2
  else
    # The link path is relative, this is the expected case
    # WARNING: don't use $PWD here, it won't work right
    # WARNING: pwd may be a shell alias.  Use /bin/pwd.
    cur=`/bin/pwd`
    lpath=`abspath $2 $cur`
  fi
  # take name off the end of the dest
  ldir=`echo $lpath | sed 's@/[^/]*$@@'`

  # If the original path and the ldir do not originate in the same
  # directory tree, we should just use absolute paths
  if [ "`echo $1 | cut -d/ -f2`" != "`echo $ldir | cut -d/ -f2`" ]; then
    spath=$1
  else
    spath=`relpath $1 $ldir`
  fi
  # use the completed relative path and the given destignation path
  echo ln -s $spath $2
  ln -s $spath $2
else
  # The source path is already relative, so just use it
  echo ln -s $1 $2
  ln -s $1 $2
fi
