#!/bin/sh
# Update ACE/TAO source tree to the latest numeric-assigned version.
# $Id: update-ace+tao.sh 91813 2010-09-17 07:52:52Z johnnyw $

# Define helper function to extract version number into tag format
d='\([0-9]*\)'
version() { sed -n -e "s,.*$1 version $d\.$d\.$d.*,$1-\1_\2_\3,p" \
                   -e "s,.*$1 version $d\.$d.*,$1-\1_\2,p"; }

# Use ccvs instead of cvs since it supports SOCKS5, if that environment found.
if [ ! -z "$SOCKS5_USER" ]; then cvs () { ccvs $*; } fi

old_ace_version=`version <VERSION ACE`
old_tao_version=`version <TAO/VERSION TAO`

# Abort with message if no values in variables
if [ -z $old_ace_version ]; then echo No existing ACE version; exit 1; fi
if [ -z $old_tao_version ]; then echo No existing TAO version; exit 1; fi

cvs update -A VERSION TAO/VERSION GNUmakefile

ace_version=`version <VERSION ACE`
tao_version=`version <TAO/VERSION TAO`

# Abort with message if no values in variables
if [ -z $ace_version ]; then echo No ACE version after update; exit 1; fi
if [ -z $tao_version ]; then echo No TAO version after update; exit 1; fi

echo Old software version tags: $old_ace_version $old_tao_version
echo New software version tags: $ace_version $tao_version

# Conserve net bandwidth if no change was observed
if [ $old_ace_version != $ace_version ] || [ x"$1"x = x"force"x ]; then
  cvs -q update -Pd -r $ace_version `make -s show_controlled_files`
fi
if [ $old_tao_version != $tao_version ] || [ x"$1"x = x"force"x ]; then
  cvs -q update -Pd -r $tao_version TAO
fi

