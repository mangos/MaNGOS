#!/bin/sh
#
# $Id: performance_stats.sh 84707 2009-03-04 10:04:06Z johnnyw $
#

if [ $# -lt 2 ]; then
  echo "Usage: $0 <ACE_ROOT> <DEST>"
  exit 0
fi

ACE_ROOT=$1
DEST=$2
DATE=`date +%Y/%m/%d-%H:%M`

mkdir -p $DEST/images
mkdir -p $DEST/data
COMPILER="gcc"

COMMON_TESTS="AMI DII DSI Deferred Single_Threaded Thread_Per_Connection Thread_Pool AMH_Single_Threaded"

SEQUENCE_TESTS="AMI DII DSI Deferred Single_Threaded Thread_Per_Connection Thread_Pool AMH_Single_Threaded"

SEQ_TEST_TYPE="octet long short char double longlong"

cd $DEST/source

/bin/sync
sleep 10
(
  cd $ACE_ROOT/performance-tests/TCP;
  ./run_test.pl
) >tcp_test.log 2>&1

if grep -q 'Client throughput: ' tcp_test.log; then
  (
    echo -n $DATE " ";
    awk '/^Client throughput:/ {print $3}' tcp_test.log
  ) >> TCP.txt;
fi

/bin/sync
sleep 10
(
   cd $ACE_ROOT/TAO/performance-tests/Latency/Single_Threaded;
   ./default_configuration.pl > $DEST/source/Default.log 2>&1
)
if grep -q 'Total throughput: ' Default.log; then
  (
    echo -n $DATE " ";
    awk '/^Total throughput:/ {print $3}' Default.log
  ) >> Default.txt
fi

/bin/sync
sleep 2
(
   cd $ACE_ROOT/TAO/performance-tests/Sequence_Latency/Single_Threaded;

   for i in $SEQ_TEST_TYPE; do
     (
       ./default_configuration.pl -t $i > $DEST/source/Sequence_Default_${i}.log 2>&1
       if grep -q 'Total throughput: ' $DEST/source/Sequence_Default_${i}.log; then
         (
           echo -n $DATE " ";
           awk '/^Total throughput:/ {print $3}' $DEST/source/Sequence_Default_${i}.log
         ) >> $DEST/source/Sequence_Default_${i}.txt
       fi
     )
   done

)

for i in $COMMON_TESTS; do
  /bin/sync
  sleep 10
  (
     cd $ACE_ROOT/TAO/performance-tests/Latency/${i};
     ./run_test.pl > $DEST/source/${i}.log 2>&1
  )
  if grep -q 'Total throughput: ' ${i}.log; then
    (
       echo -n $DATE " ";
       awk '/^Total throughput:/ {print $3}' $DEST/source/${i}.log
    ) >> ${i}.txt
  fi
done

for i in $SEQUENCE_TESTS; do
  /bin/sync
  sleep 10
  (
     cd $ACE_ROOT/TAO/performance-tests/Sequence_Latency/${i};
     for j in $SEQ_TEST_TYPE; do
         (
             ./run_test.pl -t $j > $DEST/source/Sequence_${i}_${j}.log 2>&1

             if grep -q 'Total throughput: ' $DEST/source/Sequence_${i}_${j}.log; then
                 (
                     echo -n $DATE " ";
                     awk '/^Total throughput:/ {print $3}' $DEST/source/Sequence_${i}_${j}.log
                 ) >> $DEST/source/Sequence_${i}_${j}.txt
             fi
         )
     done
  )

done

for i in $COMMON_TESTS TCP Default; do
  $ACE_ROOT/bin/generate_performance_chart.sh ${i}.txt ${i}.png "$i" 1024,768
  /bin/cp ${i}.png $DEST/images/${i}.png
  /usr/bin/tac ${i}.txt > $DEST/data/${i}.txt
  /usr/bin/tail -5 ${i}.txt > $DEST/data/LAST_${i}.txt
done

for i in $SEQ_TEST_TYPE ; do
  $ACE_ROOT/bin/generate_performance_chart.sh Sequence_Default_${i}.txt Sequence_Default_${i}.png "Default Configuration for $i sequences" 1024,768
  /bin/cp Sequence_Default_${i}.png $DEST/images/Sequence_Default_${i}.png
  /usr/bin/tac $DEST/source/Sequence_Default_${i}.txt > $DEST/data/Sequence_Default_${i}.txt
  /usr/bin/tail -5 $DEST/source/Sequence_Default_${i}.txt > $DEST/data/LAST_Sequence_Default_${i}.txt
done

for i in $SEQUENCE_TESTS; do
for j in $SEQ_TEST_TYPE; do
  $ACE_ROOT/bin/generate_performance_chart.sh Sequence_${i}_${j}.txt Sequence_${i}_${j}.png "Sequence_$i_$j" 1024,768
  /bin/cp Sequence_${i}_${j}.png $DEST/images/Sequence_${i}_${j}.png
  /usr/bin/tac Sequence_${i}_${j}.txt > $DEST/data/Sequence_${i}_${j}.txt
  /usr/bin/tail -5 Sequence_${i}_${j}.txt > $DEST/data/LAST_Sequence_${i}_${j}.txt
done
done

gnuplot <<_EOF_ >/dev/null 2>&1
    set xdata time
    set timefmt '%Y/%m/%d-%H:%M'
    set xlabel 'Date (YYYYMMDD)'
    set xtics rotate
    set ylabel 'Throughput (Requests/Second)'
    set yrange [0:]
    set terminal png small size 1024,768 color
    set output "/dev/null"
    plot 'AMI.txt' using 1:2 title 'AMI' w l
    replot 'DII.txt' using 1:2 title 'DII' w l
    replot 'DSI.txt' using 1:2 title 'DSI' w l
    replot 'Deferred.txt' using 1:2 title 'Deferred' w l
    replot 'Single_Threaded.txt' using 1:2 title 'Single_Threaded' w l
    replot 'Default.txt' using 1:2 title 'Single_Threaded (Defaults)' w l
    replot 'Thread_Per_Connection.txt' using 1:2 title 'Thread_Per_Connection' w l
    replot 'Thread_Pool.txt' using 1:2 title 'Thread_Pool' w l
    replot 'AMH_Single_Threaded.txt' using 1:2 title 'AMH' w l
    replot 'TCP.txt' using 1:2 title 'TCP/IP' w l
    set output "All.png"
    replot
    exit
_EOF_

gnuplot <<_EOF_ >/dev/null 2>&1
    set xdata time
    set timefmt '%Y/%m/%d-%H:%M'
    set xlabel 'Date (YYYYMMDD)'
    set xtics rotate
    set ylabel 'Throughput (Requests/Second)'
    set yrange [0:]
    set terminal png small size 1024,768 color
    set output "/dev/null"
    plot 'AMI.txt' using 1:2 title 'AMI' w l
    replot 'DII.txt' using 1:2 title 'DII' w l
    replot 'DSI.txt' using 1:2 title 'DSI' w l
    replot 'Deferred.txt' using 1:2 title 'Deferred' w l
    replot 'Single_Threaded.txt' using 1:2 title 'Single_Threaded' w l
    replot 'Default.txt' using 1:2 title 'Single_Threaded (Defaults)' w l
    replot 'Thread_Per_Connection.txt' using 1:2 title 'Thread_Per_Connection' w l
    replot 'Thread_Pool.txt' using 1:2 title 'Thread_Pool' w l
    replot 'AMH_Single_Threaded.txt' using 1:2 title 'AMH' w l
    set output "CORBA.png"
    replot
    exit
_EOF_

/bin/cp CORBA.png All.png $DEST/images/

cd $DEST/data
/bin/uname -a > uname.txt
$COMPILER -v > gcc.txt 2>&1
$COMPILER -dumpversion > gccversion.txt 2>&1
/lib/libc.so.6 | sed -e 's/</\&lt;/g' -e 's/>/\&gt;/g' > libc.txt
cat /proc/cpuinfo > cpuinfo.txt
cat /proc/meminfo > meminfo.txt
if [ -e "/etc/SuSE-release" ]; then
  cat /etc/SuSE-release > linuxversion.txt
fi
if [ -e "/etc/redhat-release" ]; then
  cat /etc/redhat-release > linuxversion.txt
fi

cat $ACE_ROOT/ace/config.h > config.h.txt
cat $ACE_ROOT/include/makeinclude/platform_macros.GNU > platform_macros.GNU.txt
cat $ACE_ROOT/bin/MakeProjectCreator/config/default.features > default.features.txt
cp $ACE_ROOT/html/Stats/* $DEST
