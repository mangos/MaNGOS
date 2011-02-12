#! /bin/sh
#
# $Id: generate_compile_stats.sh 84856 2009-03-16 20:03:52Z johnnyw $
#
# This script generate metrics html pages for either compile times or
# footprint.
#
# Compile times:
# Parse the build.txt file from an autobuild that was generated with the
# g++_metric.sh script, e.g., with CXX=g++_metric.sh which outputs
# compile times on a per object basis, and use the data to generate time
# series graphs with gnuplot.
#
# Footprint:
# Parse the build.txt file and and the *.map files, generated with LDFLAGS
# set to =-Xlinker -M -Xlinker -Map -Xlinker \$@.map and static_libs_only=1.
#
# For use with an autobuild, place a line something like this in the xml file,
# after the log file is closed, but before it's moved.
#
#  <command name="shell"  options="$ACE_ROOT/bin/generate_compile_stats.sh <path>/build.txt <destination> Footprint" />
#

###############################################################################
#
# usage
#
###############################################################################
usage ()
{
  echo "Usage: `basename $0` [--base=<dir>] [--name=<name>] [--compiler=compiler]"
  echo "       <input_file> <destination_directory> [target_file]"
  echo "       [Footprint|Compilation] [<date>] [<fudge_factor>]"
  echo ""
  echo "--base       This option can be used to set the base root directory to"
  echo "             something other than the default \$ACE_ROOT."
  echo "--name       This option can be used to set the software title to something"
  echo "             other than the default ACE+TAO+CIAO."
  echo "--compiler   This option can be used to set the compiler to something"
  echo "             other than the default gcc."
  echo "input_file   This is the compilation log file."
  echo "destination_directory This designates the location of the generated html."
  echo "target_file  This is similar to input_file, but should contain no errors."
  echo "date         Set the date used in all generated html pages."
  echo "fudge_factor Add the specified number of seconds to the compilation time"
  echo "             for each target."
  echo ""
  echo "Options must be specified in the order shown above."
  exit
}

###############################################################################
#
# parse_time
#
# this only works for english
# assumes the date is formatted like this:  Sat Apr 12 18:19:31 UTC 2003
# and outputs this: 2003/04/12-18:19
#
###############################################################################
parse_time ()
{
  # todo: add a format parameter
  local INDEX=0
  local PT_MONTH=""
  local PT_DAY=""
  local PT_YEAR=""
  local PT_HOUR=""
  local PT_MINUTE=""
  local PT_SECOND=""
  local PT_TIMEZONE=""

  read -a line
  for token in "${line[@]}"; do
    #echo "$INDEX = $token"
    case $INDEX in
      1 ) case $token in
            Jan ) PT_MONTH="01" ;;
            Feb ) PT_MONTH="02" ;;
            Mar ) PT_MONTH="03" ;;
            Apr ) PT_MONTH="04" ;;
            May ) PT_MONTH="05" ;;
            Jun ) PT_MONTH="06" ;;
            Jul ) PT_MONTH="07" ;;
            Aug ) PT_MONTH="08" ;;
            Sep ) PT_MONTH="09" ;;
            Oct ) PT_MONTH="10" ;;
            Nov ) PT_MONTH="11" ;;
            Dec ) PT_MONTH="12" ;;
          esac ;;
      2 ) PT_DAY="$token" ;;
      3 ) PT_HOUR="${token%%:*}"
          PT_MINUTE="${token%:*}"
          PT_MINUTE="${PT_MINUTE#*:}"
          PT_SECOND="${token##*:}" ;;
      4 ) PT_TIMEZONE="$token" ;;
      5 ) PT_YEAR="$token" ;;
    esac
    let INDEX=$INDEX+1
  done
  if [ "$1" = "debug" ]; then
    echo "month = $PT_MONTH"
    echo "day   = $PT_DAY"
    echo "year  = $PT_YEAR"
    echo "hour  = $PT_HOUR"
    echo "min   = $PT_MINUTE"
    echo "sec   = $PT_SECOND"
    echo "tz    = $PT_TIMEZONE"
  fi
  echo "$PT_YEAR/$PT_MONTH/$PT_DAY-$PT_HOUR:$PT_MINUTE"
}

###############################################################################
#
# strip_date
#
# grab date from line with following format:
# ################### End [Fri Apr 11 00:18:31 2003 UTC]
# and return it in this format: Fri Apr 11 00:18:31 UTC 2003 which is
# what parse_time() expects
#
###############################################################################
strip_date ()
{
  local INDEX=0
  local TEMP_DATE=""
  local DATE=""
  read -a line
  for token in "${line[@]}"; do
    #echo "$INDEX = $token"
    case $INDEX in
       2 ) DATE=${token#[} ;;
       7 ) DATE="$DATE ${token%]} $TEMP_DATE" ;;
       # this is a hack since the autobuild scripts don't format the date
       # correctly...  :-(
       6 ) TEMP_DATE=$token ;;
       * ) DATE="$DATE $token" ;;
    esac
    let INDEX=$INDEX+1
  done
  echo $DATE
}

###############################################################################
#
# parse
#
# Parse the commandline and validate the inputs
#
###############################################################################
parse ()
{
  echo "parse()"
  while [ $# -gt 1 ]; do
    if [ -n "`echo $1 | grep '^--base=.*'`" ]; then
      BASE_ROOT=`echo $1 | sed 's/^--base=//'`
      shift
    elif [ -n "`echo $1 | grep '^--name=.*'`" ]; then
      BASE_TITLE=`echo $1 | sed 's/^--name=//'`
      shift
    elif [ -n "`echo $1 | grep '^--compiler.*'`" ]; then
      COMPILER=`echo $1 | sed 's/^--compiler=//'`
      shift
    else
      break
    fi
  done

  # set input file and destination (required)
  if [ $# -gt 1 ]; then
    INFILE=$1
    DEST=$2

    if ! [ -e "$INFILE" ]; then
      echo "input_file $INFILE does not exist."
      usage
    fi
  else
    usage
  fi

  # set the target file from command line
  if [ $# -gt 2 ]; then
    TARGETS=$3
  else
   TARGETS=$INFILE
  fi

  # set type of metric from command line
  if [ $# -gt 3 ]; then
    METRIC=$4
  else
    METRIC="Compilation"
  fi
  echo "metric = ($METRIC)"

  # set the date from command line
  if [ $# -gt 4 ]; then
    DATE=$5
  else
    DATE=`tail -n 1 $INFILE | strip_date | parse_time`
  fi
  echo "date = ($DATE)"

  # set fudge factor from commandline (for testing)
  if [ $# -gt 5 ]; then
    FUDGE_FACTOR=$6
  else
    FUDGE_FACTOR=0
  fi
}

###############################################################################
#
# gen_chart
#
# Generate the actual charts and move them to ${DEST}
#
###############################################################################
gen_chart ()
{
  local object=$1
  local DEST=$2
  local TYPE=$3
  local EXT="txt"
  local YLABEL="Compile Time (Seconds)"
  local FACTOR=1
  local low=$4
  local high=$5

  if [ "$TYPE" = "Footprint" ]; then
    EXT="size"
    if [ ${high} -gt 1024 ]; then
      YLABEL="Footprint (KBytes)"
      FACTOR=1024
    else
      YLABEL="Footprint (Bytes)"
      FACTOR=1
    fi
  fi

  let low="${low}/${FACTOR}"
  let high="${high}/${FACTOR}"

  sort -t'/' -k1n -k2n -k3n ${DEST}/data/${object}.${EXT} | grep -E ^2 > tmp.txt

  gnuplot <<EOF
    set data style lp l
    set time "$DATE"
    set xdata time
    set timefmt "%Y/%m/%d-%H:%M"
    set format x "%Y/%m/%d"
    set xtics rotate
    set xlabel 'Date (YYYY/MM/DD)' 0,-3
    set ylabel "${YLABEL}"
    set terminal png small size 1024,768 color
    set yrange [0:]
    set output "${DEST}/images/${object}_${TYPE}.png"
    set title "${object//___//}"
    plot 'tmp.txt' using 1:(\$2/$FACTOR) notitle w points, 'tmp.txt' using 1:(\$2/$FACTOR) notitle w l lt 3 lw 4
    exit
EOF

  # here's how to reduce the scale
  #  plot '$1' using 1:(\$2/1024.0) title '$3' w l
}

###############################################################################
#
# create_dirs
#
# Make sure hidden directory tree exists, and create it if it doesn't
#
###############################################################################
create_dirs ()
{
  echo "create_dirs() '$1'"
  if ! [ -d "${1}" ]; then
    mkdir -p ${1}
  fi
  if ! [ -d "${1}data" ]; then
    mkdir -p ${1}data
  fi
  if ! [ -d "${1}images" ]; then
    mkdir -p ${1}images
  fi
}

###############################################################################
#
# process_file
#
# Process the the $INPUT file
#
###############################################################################
process_file ()
{
  echo "process_file()"

  local CURRENT_TIME=0
  local CURRENT_OBJECT=""
  local CURRENT_PATH=""
  local seconds=0
  local hundreths=0

  while read target usertime systemtime; do

    # get path
    CURRENT_PATH=${target%/*}

    # strip off the hidden directory if needbe
    CURRENT_PATH=${CURRENT_PATH%/.*}

    # replace all "/" with "___"
    # (so we can keep them all in the same directory)
    CURRENT_PATH=${CURRENT_PATH//\//___}

    # strip path off of target
    CURRENT_OBJECT=${CURRENT_PATH}___${target##*/}
    #echo "target = $target, object = $CURRENT_OBJECT,  path = $CURRENT_PATH"
    #echo "usertime = $usertime, systemtime = $systemtime"

    let seconds="${usertime%.*}+${systemtime%.*}"

    # it's just easier to grab the values first, since .0n causes problems...
    userdec="${usertime#*.}"
    userdec=${userdec#0}
    systemdec="${systemtime#*.}"
    systemdec=${systemdec#0}
    let hundreths="${userdec}+${systemdec}"

    let CURRENT_TIME="(${seconds}*100 + ${hundreths})+$FUDGE_FACTOR"
    echo $DATE $CURRENT_TIME >> ${DEST}/data/${CURRENT_OBJECT}.txt

  done # while
}

###############################################################################
#
# composite_list
#
###############################################################################
composite_list ()
{
  local FOUND_OBJ=0
  local BASE_OBJ_FLAG=0
  local BASE_OBJ=""
  local OBJ_LIST=""
  local DIR_LINE=0
  local DIR=""
  local INDEX=0

  while read -a line; do
    DIR_LINE=0
    INDEX=0

    for i in "${line[@]}"; do
      if [ $DIR_LINE -eq 1 ]; then

        # only process when entering a directory
        if [ $INDEX -eq 1 ] && [ "$i" = "Leaving" ]; then
          DIR=""
          break
        fi

        if [ $INDEX -eq 3 ]; then
          DIR="${i%?}"                        # strip off last "'"
          DIR="${DIR#*$BASE_ROOT/}"           # strip off $BASE_ROOT
          DIR="${DIR//\//___}___"             # replace "/" with "___"
          break
        else
          let INDEX="$INDEX+1"
          continue
        fi
      fi

      # if it was a "make" line then continue to the next token which will
      # continue to process above
      if [ "${i%[*}" = "make" ] || [ "${i%:*}" = "make" ]; then
        DIR=""
        let DIR_LINE=1
        let INDEX="$INDEX+1"
        continue
      fi

      # not an "make" line, so process it here.
      if [ $BASE_OBJ_FLAG -eq 1 ]; then
         BASE_OBJ="${DIR}${i##.*/}"
         # strip off lib numbers
         if [ "$BASE_OBJ" != "${BASE_OBJ%.so.*}" ]; then
           BASE_OBJ=${BASE_OBJ%.so.*}.so
         fi
         BASE_OBJ_FLAG=0
      elif [ "$i" = "-o" ]; then
        # found our base object, set flag so we can grab the next one
        BASE_OBJ_FLAG=1
      elif [ "$i" = "${i#-}" -a "$i" = "${i#/}" -a "$i" != "${i%.o}" ]; then
        OBJ_LIST="$OBJ_LIST ${DIR}${i##*/}"
        FOUND_OBJ=1
      fi
    done # for
    if [ $FOUND_OBJ -eq 1 ]; then
      echo "$BASE_OBJ : $OBJ_LIST"
      FOUND_OBJ=0
      OBJ_LIST=""
      BASE_OBJ=""
    fi
  done # while
}

###############################################################################
#
# create_composite_list
#
###############################################################################
create_composite_list ()
{
  echo "create_composite_list()"
  local INFILE=$1

  # create a pattern file
  echo "\-L" > ${DEST}/comp_match.txt
  echo "Entering directory" >> ${DEST}/comp_match.txt

  # grep out the entering directory line and all the link lines,
  # but only keep entering directory lines preceeding link lines.
  cat $INFILE | grep -f ${DEST}/comp_match.txt | grep -B1 "\-L" \
   | grep -ve "--" | composite_list > ${DEST}/composites.txt
}

###############################################################################
#
# library_list
#
###############################################################################
library_list ()
{
  local FOUND_OBJ=0
  local BASE_OBJ_FLAG=0
  local BASE_OBJ=""
  local OBJ_LIST=""
  local DIR_LINE=0
  local DIR=""
  local INDEX=0

  while read -a line; do
    DIR_LINE=0
    INDEX=0
    for i in "${line[@]}"; do
      if [ $DIR_LINE -eq 1 ]; then
        if [ $INDEX -eq 3 ]; then
          DIR="${i%?}"                        # strip off last "'"
          DIR="${DIR#*$BASE_ROOT/}"           # strip off $BASE_ROOT
          DIR="${DIR//\//___}___"             # replace "/" with "___"
          break
        else
          let INDEX="$INDEX+1"
          continue
        fi
      fi

      # if it was a "make" line then continue to the next token which will
      # continue to process above
      if [ "${i%[*}" = "make" ]; then
        let DIR_LINE=1
        let INDEX="$INDEX+1"
        continue
      fi

      # not a "make" line, so process it here.  We are interested in the
      # 3rd, and last,  token, i.e., lib*.a
      let INDEX="$INDEX+1"
      if [ $INDEX -eq 3 ]; then
        echo "$DIR$i"
      fi
    done # for
  done # while
}

###############################################################################
#
# create_library_list
#
###############################################################################
create_library_list ()
{
  echo "create_library_list()"
  local INFILE=$1

  # create a pattern file
  echo "chmod" > ${DEST}/lib_match.txt
  echo "Entering directory" >> ${DEST}/lib_match.txt

  # grep out the entering directory line and all the link lines,
  # but only keep entering directory lines preceeding link lines.
  cat $INFILE | grep -f ${DEST}/lib_match.txt | grep -B1 "chmod" \
   | grep -ve "--" | library_list > ${DEST}/libraries.txt
}

###############################################################################
#
# rollup_compile_times
#
###############################################################################
rollup_compile_times ()
{
  echo "process_composite_objects()"
  local TOTAL_TIME=0
  local temp
  local tdate=""
  local ttime=0
  local lpath="${DEST}/data/"

  # rollup compile times
  while read outfile colon infiles; do
    #echo "$outfile ----- $infiles"
    for i in $infiles; do
      temp=`head -n 1 ${lpath}${i}.txt`
      tdate=${temp%% *}
      let ttime="${temp##* }"

      if [ "$tdate" = "$DATE" ]; then
        let TOTAL_TIME="$TOTAL_TIME + ${ttime}"
      fi
    done # for
    echo "$DATE $TOTAL_TIME" >> ${lpath}${outfile}.txt
    let TOTAL_TIME=0
  done # while
}

###############################################################################
#
# footprint
#
###############################################################################
footprint ()
{
  echo "footprint()"
  local TYPE="$1"
  local fpath=""
  local lpath="${DEST}/data/"
  local FILE=""
  local SIZE=""

  # Remove the old size_composites.txt and create a new one since
  # we have all the info we need from the size command on a library.
  if [ "$TYPE" = "LIB" ] && [ -e ${DEST}/size_composites.txt ]; then
    rm ${DEST}/size_composites.txt
  fi

  # go through all the targets and get the sizes of the target and
  # each dependent object and write it to a *.size file.
  while read outfile colon infiles; do
    # reconstitue file name
    FILE="$BASE_ROOT/${outfile//___//}"

    if [ -e $FILE ]; then
      #echo "inside if"
      SIZE=`size $FILE | grep -v text | awk '{s += \$4} END {print s}'`
      echo "$DATE $SIZE" >> $lpath/${outfile}.size

      # only process the included objects if it's a library
      if [ "$TYPE" = "LIB" ]; then
        fpath=${FILE%/*}
        # now, do the same for all the objects in the file (if any)
        size $FILE |
        grep -v text |
        awk '{print $4 " : " $6}' | process_included $fpath $lpath $FILE
      fi
    fi

  done # while
}

###############################################################################
#
# process_included
#
###############################################################################
process_included ()
{
  echo "process_included()"
  local fpath=$1
  local lpath=$2
  local LIBRARY=$3
  local FILE=""
  local OUTFILE=""

  # while we are here, and have the info, go ahead and write out
  # size dependencies for each library.
  LIBRARY="${LIBRARY#*$BASE_ROOT/}"   # strip off $BASE_ROOT
  LIBRARY="${LIBRARY//\//___}"        # replace "/" with "___"
  echo -n  "$LIBRARY : " >> ${DEST}/size_composites.txt

  while read size colon file; do
    FILE=$fpath/$file
    OUTFILE="${FILE#*$BASE_ROOT/}"     # strip off $BASE_ROOT
    OUTFILE="${OUTFILE//\//___}"       # replace "/" with "___"
    #echo "size = ($size)"
    echo "$DATE $size" >> $lpath/${OUTFILE}.size

    # add the object
    echo -n "$OUTFILE " >> ${DEST}/size_composites.txt

  done
  # add newline
  echo "" >> ${DEST}/size_composites.txt

}

###############################################################################
#
# process_map_file
#
###############################################################################
process_map_file ()
{
  # this is way too noisy...
  #echo "process_map_file()"
  local index=0
  local INFILE=$1
  local FILE=""

  # Read in the map file.  The first section is all we are interested
  # in right now.  As soon as we see "Memory Configuration" we are done.
  while read line; do
    let index=$index+1
    # Skip the first 2 lines, then read in all the odd lines, they are the
    # objects that must be loaded.  The following line, even lines, is the
    # object that caused it to be loaded.
    if [ $index -lt 3 ] || [ "$line" = "" ]; then
      continue
    fi

    # The line looks like this:
    #/ace_root_path/ace/libACE.a(Malloc.o)
    # need to find the real library path, since these libs will
    # always be in ace/, but the objects will be in the original
    # location.
    lib="${line##/*/}"    # strip off path, but only if it starts a line
    lib="${lib%%.a*}"     # strip off rest of line
    lib="$lib.a"          # put back the .a to make it unambiguous
    libpath=`grep $lib ${DEST}/libraries.txt`
    path="${libpath%___lib*}"         # strip off libname
    FILE="${line#*(}"             # strip off everything up to object name
    FILE="${FILE%%)*}"            # strip off rest of line
    FILE="${path}___${FILE}"
    echo -n "$FILE " >> ${DEST}/size_composites.txt
  done

  echo "" >> ${DEST}/size_composites.txt

}

###############################################################################
#
# create_size_composites
#
###############################################################################
create_size_composites ()
{
  echo "create_size_composites()"
  local FILE=""

  while read outfile colon infiles; do
    # reconstitue file name
    FILE="$BASE_ROOT/${outfile//___//}.map"
    if [ -e $FILE ]; then
      echo -n "$outfile : " >> ${DEST}/size_composites.txt
      # only process lines that don't begin with a space
      cat $FILE | sed -e '/Memory Configuration/,$d' \
        | sed -e '/Allocating common symbols/,$d' \
        | grep -v "^ " | process_map_file $FILE
    fi
    #break
  done

}

###############################################################################
#
# create_images
#
###############################################################################
create_images ()
{
  echo "create_images()"

  local DEST=$1
  local TYPE=$2
  local LOW=0
  local HIGH=5000
  local STEP=0
  local TMP=0

  while read object; do
    if [ -e $object ] && [ `sort -t'/' -k1n -k2n -k3n $object | grep -E ^2 | tail -n 1 | cut -d' ' -f2` ]; then
      let TMP=`sort -t'/' -k1n -k2n -k3n $object | grep -E ^2 | tail -n 1 | cut -d' ' -f2`
      let TMP=$TMP*16/10
      STEP=100
      HIGH=0
      while [ $HIGH -eq 0 ]; do
        if [ $TMP -lt $STEP ]; then
          HIGH=$STEP
        fi
        let STEP=$STEP*15/10
      done

      if [ "$TYPE" = "Footprint" ]; then
        object="${object%.size}"
      else
        object="${object%.txt}"
      fi

      gen_chart "${object##*/}" ${DEST} ${TYPE} ${LOW} ${HIGH} >/dev/null 2>&1
    fi
  done

}

###############################################################################
#
# create_index_page
#
###############################################################################
create_index_page ()
{
  local TYPE="$1"
  local TITLE="$TYPE metrics for $BASE_TITLE"

  echo "<html>"
  echo "<head><title>$TITLE</title></head>"
  echo '<style><!--'
  echo 'body,td,a,p,.h{font-family:arial,sans-serif;}'
  echo '.h{font-size: 20px;}'
  echo '.q{text-decoration:none; color:#0000cc;}'
  echo '//-->'
  echo '</style>'
  echo '<body text = "#000000" link="#000fff" vlink="#ff0f0f" bgcolor="#ffffff">'
  echo "<br><center><h1>$TITLE</h1></center><br><hr>"
  if [ $BASE_TITLE = $DEFAULT_TITLE ]; then
    echo '<p>One of the goals of the PCES-TENA project is to decrease compile times.'
  else
    echo '<p>'
  fi
  echo '   Metrics are gathered nightly on all
           objects in the '$BASE_TITLE' distribution and displayed here.'
  echo '<ul>'
  if [ $BASE_TITLE = $DEFAULT_TITLE ]; then
    echo "<li><a href=\"ace_${TYPE}.html\">ACE</a>"
    echo "<li><a href=\"tao_${TYPE}.html\">TAO</a>"
    echo "<li><a href=\"ciao_${TYPE}.html\">CIAO</a>"
  else
    echo "<li><a href=\"all_${TYPE}.html\">ALL</a>"
  fi
  echo '</ul>'
  echo '<hr>'

  echo '<P>All the experiments run on the system described below. '
  echo 'The machine is running Linux ('

  if [ -e "/etc/SuSE-release" ]; then
    cat /etc/SuSE-release
  fi

  if [ -e "/etc/redhat-release" ]; then
    cat /etc/redhat-release
  fi

  echo "), and we use " $COMPILER " version "

  $COMPILER -dumpversion > ${DEST}/compilerversion.txt 2>&1
  cat ${DEST}/compilerversion.txt

  echo ' to compile '$BASE_TITLE'. </P>'

  if [ -z "$MPC_ROOT" ]; then
    MPC_ROOT=$ACE_ROOT/MPC
  fi

  CFG_FILES=$ACE_ROOT/ace/config.h
  if [ -r $ACE_ROOT/bin/MakeProjectCreator/config/default.features ]; then
    CFG_FILES="$CFG_FILES $ACE_ROOT/bin/MakeProjectCreator/config/default.features"
  elif [ -r $MPC_ROOT/config/default.features ]; then
    CFG_FILES="$CFG_FILES $MPC_ROOT/config/default.features"
  fi
  CFG_FILES="$CFG_FILES $ACE_ROOT/include/makeinclude/platform_macros.GNU"

  echo '<TABLE border="2"><TBODY>'
  for cfg_file in $CFG_FILES; do
    if [ -r $cfg_file ]; then
      echo "<TR><TD>ACE+TAO+CIAO Configuration</TD><TD>`basename $cfg_file`</TD></TR>"
      echo '<TR><TD colspan="2"><PRE>'
      cat $cfg_file
      echo '</PRE></TD></TR>'
    fi
  done

  echo '<TR><TD>CPU Information</TD><TD>/proc/cpuinfo</TD></TR>'
  echo '<TR><TD colspan="2"><PRE>'

  cat /proc/cpuinfo

  echo '</PRE></TD></TR><TR><TD>Available Memory</TD><TD>/proc/meminfo</TD></TR>'
  echo '<TR><TD colspan="2"><PRE>'

  cat /proc/meminfo

  echo '</PRE></TD></TR><TR><TD>OS Version</TD><TD>uname -a</TD></TR>'
  echo '<TR><TD colspan="2"><PRE>'

  /bin/uname -a

  echo '</PRE></TD></TR><TR><TD>Compiler Version</TD><TD>'$COMPILER' -v</TD></TR>'
  echo '<TR><TD colspan="2">'

  $COMPILER -v > ${DEST}/compiler.txt 2>&1
  cat ${DEST}/compiler.txt

  if [ -e "/lib/libc.so.6" ]; then
    echo '</TD></TR><TR><TD>Library Version</TD><TD>/lib/libc.so.6</TD></TR>'
    echo '<TR><TD colspan="2"><PRE>'

    /lib/libc.so.6 | sed -e 's/</\&lt;/g' -e 's/>/\&gt;/g'
  fi

  echo '</PRE></TD></TR></TBODY></TABLE>'
  echo '</body></html>'
}

###############################################################################
#
# create_page
#
###############################################################################
create_page ()
{
  # always strip off "TAO___" / "CIAO___"
  local BASE=$1
  local TYPE=$2
  local EXT=""
  local BASE_NAME=${BASE#TAO___}
  local BASE_NAME=${BASE#CIAO___}
  local TITLE="${TYPE} metrics for ${BASE_NAME//___//}"

  if [ "$TYPE" = "Compilation" ]; then
    EXT="txt"
    UNITS="100th's of seconds"
  else
    EXT="size"
    UNITS="Bytes"
  fi

  # header
  echo "<html>"
  echo "<head><title>$TITLE</title></head>"
  echo '<style><!--'
  echo 'body,td,a,p,.h{font-family:arial,sans-serif;}'
  echo '.h{font-size: 20px;}'
  echo '.q{text-decoration:none; color:#0000cc;}'
  echo '//-->'
  echo '</style>'
  echo '<body text = "#000000" link="#000fff" vlink="#ff0f0f" bgcolor="#ffffff">'
  echo "<br><center><h1>$TITLE</h1></center><br>"
  if [ -e "${DEST}/images/${BASE}_${TYPE}.png" ]; then
    echo '<DIV align="center"><P>'
    echo "<IMG alt=\"$BASE\" border=0 src=\"images/${BASE}_${TYPE}.png\""
    echo 'width="800" height="600"></P></DIV>'
  fi

  echo "<br><hr><br>"
  echo "<center><h2>Detail (${DATE})</h2></center>"

  echo '<TABLE border="2"><TBODY><TR><TD rowspan=2><b>Object</b></TD>'
  echo '<TD colspan="4" align=center><b>Last Compile</b></TD></TR>'
  echo "<TR><TD align=center><b>Date</b></TD><TD align=center><b>$UNITS</b></TD>"
  echo '<TD align=center><b>%chg</b></TD><TD align=center><b>Data</b></TD></TR>'
  while read i; do
    if [ -e "${DEST}/data/${i}.${EXT}" ]; then
      LAST=0 PRE=0 VAL_TMP=0 VAL_INT=0 VAL_SIGN="+"
      echo '<TR><TD>'
      if [ -e "${DEST}/${i}_${TYPE}.html" ]; then
        # strip off "TAO___" if it exists
        NAME=${i#TAO___}
        # strip off "CIAO___" if it exists
        NAME=${i#CIAO___}
        echo "<a href=\"${i}_${TYPE}.html\">${NAME//___//}</a>"
      elif [ -e "${DEST}/images/${i}_${TYPE}.png" ]; then
        # since you'll only have images if it's a composite, strip off the
        # path for the name
        if [ "$TYPE" = "Footprint" ]; then
          # if we are doing footprint, add library
          llib=`grep -e "lib.*\.a" ${DEST}/size_composites.txt | grep ${i} | awk '{print $1}'`
          #echo "lib $llib"
          #llib="${llib% :}"
          llib="${llib//___//}"
          NAME="${llib}(${i##*___})"
        else
          NAME="${i##*___}"
        fi
        echo "<a href=\"images/${i}_${TYPE}.png\">${NAME}</a>"
      else
        echo "${i##*___}"
      fi
      echo '</TD><TD>'
      echo `sort -t'/' -k1n -k2n -k3n  ${DEST}/data/${i}.${EXT} | grep -E ^2 | tail -n1  | cut -d" " -f1`
      let LAST=`sort -t'/' -k1n -k2n -k3n ${DEST}/data/${i}.${EXT} | grep -E ^2 | tail -n1 | cut -d" " -f2`
      echo "</TD><TD align=right>$LAST</TD>"
      let PRE=`sort -t'/' -k1n -k2n -k3n ${DEST}/data/${i}.${EXT} | grep -E ^2 | tail -n2 | head -n1 | cut -d" " -f2`
      let VAL_TMP="((($LAST+1)-($PRE+1))*1000)/($PRE+1)"
      if [ $VAL_TMP -lt 0 ]; then
        VAL_SIGN="-"
        let VAL_TMP="-1*$VAL_TMP"
      elif [ $VAL_TMP -eq 0 ]; then
        VAL_SIGN=
      fi
      let VAL_INT="$VAL_TMP/10"
      let VAL_TENTH="$VAL_TMP-($VAL_INT*10)"
      echo "<TD align=right>${VAL_SIGN}${VAL_INT}.${VAL_TENTH}</TD>"
      echo "<TD align=right><a href=\"data/${i}.${EXT}\">Data</a></TD>"
      ecoo "</TR>"
    else
      echo '<TR><TD>'
      echo "${i}"
      echo '</TD><TD>'
      echo '?'
      echo "</TD><TD align=right>?</TD>"
      echo "<TD align=right>?</TD><TD align=right>?</TD></TR>"
    fi
  done # for
  echo '</TBODY></TABLE>'

  # footer
  echo '</body></html>'

}

###############################################################################
#
# sort_list
#
###############################################################################
sort_list ()
{
  # sort the dependency files
  if [ -e ${DEST}/tmp_list ]; then
    rm ${DEST}/tmp_list
  fi

  touch ${DEST}/tmp_list
  for i in $@; do
    echo "$i" >> ${DEST}/tmp_list
    #echo $i
  done

  # sort eats underscores, soo...
  sed "s/___/000/g" ${DEST}/tmp_list | sort -f | sed "s/000/___/g"
}

###############################################################################
#
# create_html
#
###############################################################################
create_html ()
{
  echo "create_html()"

  local DEST=$1
  local TYPE=$2
  local ALL_BASE=""
  local ACE_OBJS=""
  local TAO_OBJS=""
  local CIAO_OBJS=""

  while read base colon files; do
    # create individual page for app/lib

    sort_list ${files} | create_page ${base} ${TYPE} \
      > ${DEST}/${base}_${TYPE}.html
    if [ "${base}" != "${base#TAO___CIAO}" ]; then
      CIAO_OBJS="${CIAO_OBJS} ${base}"
    elif [ "${base}" != "${base#TAO}" ]; then
      TAO_OBJS="${TAO_OBJS} ${base}"
    else
      ACE_OBJS="${ACE_OBJS} ${base}"
    fi
    ALL_OBJS="${ALL_BASE} ${base}"
  done

  # create main page
  create_index_page ${TYPE} > ${DEST}/index.html

  if [ "${TYPE}" = "Compilation" ] || [ "${TYPE}" = "Footprint" ]; then
    if [ $BASE_TITLE = $DEFAULT_TITLE ]; then
      name="ace_${TYPE}.html"
      sort_list ${ACE_OBJS} | create_page "ACE" ${TYPE} > ${DEST}/${name}

      name="tao_${TYPE}.html"
      sort_list ${TAO_OBJS} | create_page "TAO" ${TYPE} > ${DEST}/${name}

      name="ciao_${TYPE}.html"
      sort_list ${CIAO_OBJS} | create_page "CIAO" ${TYPE} > ${DEST}/${name}
    else
      name="all_${TYPE}.html"
      sort_list ${ACE_OBJS} | create_page $BASE_TITLE ${TYPE} > ${DEST}/${name}
    fi
  fi
}

###############################################################################
#
# main program
#
###############################################################################

INFILE=""
DEST=""
TARGETS=""
DATE=""
METRIC="Compilation"
FUDGE_FACTOR=0
BASE_ROOT=$ACE_ROOT
DEFAULT_TITLE=ACE+TAO+CIAO
BASE_TITLE=$DEFAULT_TITLE
COMPILER="gcc"

parse $@
create_dirs "${DEST}/"
create_dirs "${DEST}/"

if [ "$METRIC" = "Compilation" ]; then

  ########################################################
  # compile times

  # grab the compile time metrics for objects only and process them
  grep "compile time(0):" $INFILE | grep "\.o" | cut -d' ' -f3,4,5 | process_file

  # Create ${DEST}/composites.txt with entries like this:
  # tests___OS_Test :  tests___OS_Test.o tests___Main.o
  create_composite_list $TARGETS

  # compile times
  cat ${DEST}/composites.txt | rollup_compile_times
  find ${DEST}/data/ -name "*.txt" | create_images ${DEST} "Compilation"
  cat ${DEST}/composites.txt | create_html ${DEST} "Compilation"

elif [ "$METRIC" = "Footprint" ]; then

  ########################################################
  # footprint

  # Create ${DEST}/libraries.txt with entries like this:
  # ace___libACE.a
  create_library_list $TARGETS

  # Create ${DEST}/composites.txt with entries like this:
  # tests___OS_Test :  tests___OS_Test.o tests___Main.o
  create_composite_list $TARGETS

  # Run size on the executables and append results to *.size file.
  cat ${DEST}/composites.txt | footprint

  # Run size on the libraries and append results to *.size for the
  # library and each contained object.
  # It also creates ${DEST}/size_composites.txt based on size output for
  # libraries with entries like this:
  # ace___libACE.a : ace___ACE.o ace___Addr.o
  cat ${DEST}/libraries.txt | footprint LIB

  # Add executables to ${DEST}/size_composites.txt based on output
  # from the map files (created with LDFLAGS=-Xlinker -M -Xlinker
  # -Map -Xlinker $(@).map).  Find the map files of we want based on
  # entries in ${DEST}/composites.txt.
  cat ${DEST}/composites.txt | create_size_composites

  find ${DEST}/data/ -name "*.size" | create_images ${DEST} "Footprint"
  cat ${DEST}/size_composites.txt | create_html ${DEST} "Footprint"

else
  echo "metric type ($METRIC) not recognized"
  usage
fi
