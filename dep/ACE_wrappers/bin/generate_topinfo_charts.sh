#!/bin/sh
#
# $Id: generate_topinfo_charts.sh 84658 2009-03-01 18:45:35Z johnnyw $
#

gnuplot <<_EOF_ >/dev/null 2>&1
  set xdata time
  set xtics rotate
  set timefmt '%Y/%m/%d-%H:%M'
  set xlabel 'Date (MM/DD)'
  set ylabel 'Size (KBytes) $4'
  set terminal png small size 800,600 color
  set output "$2"
  plot '$1' using 1:2 title '$3' w l
  exit
_EOF_

