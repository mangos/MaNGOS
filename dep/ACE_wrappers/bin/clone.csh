#!/bin/csh

set src_root=`pwd`
set dst_root=`abspath $1`

set subdirs=`find * -type d -print`

mkdir $dst_root
set files=`find * \( -type d -prune \) -o -type f -print`

if ($#files) then
	ln $files $dst_root
endif

if ($#subdirs) then
	foreach subdir ($subdirs)
		cd $src_root
		mkdir $dst_root/$subdir
		cd $src_root/$subdir
		set files=`find * \( -type d -prune \) -o -type f -print`
		if ($#files) then
			ln $files $dst_root/$subdir
		endif
	end
endif
exit 0
