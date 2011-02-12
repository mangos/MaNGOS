#!/bin/sh
#
# $Id: footprint_stats.sh 84643 2009-02-27 17:24:55Z johnnyw $
#


if [ $# -lt 2 ]; then
  echo "Usage: $0 [ACE_ROOT] [DEST]"
  exit 0
fi

ACE_ROOT=$1
DEST=$2
DATE=`date +%Y/%m/%d-%H:%M`

BINS="TAO/tests/ORB_init/ORB_init TAO/tests/ORB_destroy/ORB_destroy"

LIBS="ace/libACE.a \
    TAO/tao/libTAO.a \
    TAO/tao/libTAO_PortableServer.a \
    TAO/tao/libTAO_Strategies.a \
    TAO/tao/libTAO_SmartProxies.a \
    TAO/tao/libTAO_DynamicAny.a \
    TAO/tao/libTAO_DynamicInterface.a \
    TAO/tao/libTAO_IFR_Client.a \
    TAO/tao/libTAO_BiDirGIOP.a \
    TAO/tao/libTAO_Domain.a \
    TAO/tao/libTAO_IORManip.a \
    TAO/tao/libTAO_IORTable.a \
    TAO/tao/libTAO_TypeCodeFactory.a \
    TAO/tao/libTAO_RTCORBA.a \
    TAO/tao/libTAO_IORInterceptor.a \
    TAO/tao/libTAO_Messaging.a \
    TAO/tao/libTAO_ObjRefTemplate.a \
    TAO/tao/libTAO_Valuetype.a \
    TAO/tao/libTAO_RTScheduler.a \
    TAO/tao/libTAO_AnyTypeCode.a \
    TAO/tao/libTAO_PI.a \
    TAO/tao/libTAO_PI_Server.a \
    TAO/tao/libTAO_Codeset.a \
    TAO/tao/libTAO_CodecFactory.a \
    TAO/tao/libTAO_RTPortableServer.a"

cd $ACE_ROOT

mkdir -p $DEST/source
mkdir -p $DEST/data
mkdir -p $DEST/images

for i in $BINS; do
  b=`basename $i`
  if [ -x $i ]; then
    (
      echo -n $DATE " ";
      size $i |
      grep -v text |
      awk '{print $4}'
    ) >> $DEST/source/${b}_size.txt
  fi
done

for i in $LIBS; do
  b=`basename $i`;
  if [ -f $i ]; then
    (
      echo -n $DATE " ";
      size $i |
       awk '{s += $4} END {print s}'
    ) >> $DEST/source/${b}_size.txt
  fi
done

cd $DEST/source

for i in $LIBS $BINS; do
 b=`basename $i`
 /usr/bin/tac ${b}_size.txt > $DEST/data/${b}_size.txt
 /usr/bin/tail -5 ${b}_size.txt > $DEST/data/LAST_${b}_size.txt
 $ACE_ROOT/bin/generate_footprint_chart.sh ${b}_size.txt $DEST/images/${b}_size.png $b
done
