#!/bin/sh
#
# $Id: generate_performance_chart.sh 84708 2009-03-04 10:15:35Z johnnyw $
#

gnuplot <<_EOF_ >/dev/null 2>&1
    set xdata time
    set xtics rotate
    set format x "%Y/%m/%d"
    set timefmt '%Y/%m/%d-%H:%M'
    set xlabel 'Date (YYYY/MM/DD)' 0,-3
    set ylabel 'Throughput (Requests/Second)'
    set terminal png small size $4 color
    set yrange [0:]
    set output "$2"
    plot '$1' using 1:2 title '$3' w l
    exit
_EOF_
