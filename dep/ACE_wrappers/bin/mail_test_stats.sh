export TREE_ROOT=$HOME/ACE/latest
export ACE_ROOT=$TREE_ROOT/ACE_wrappers
export TAO_ROOT=$ACE_ROOT/TAO
export CIAO_ROOT=$TAO_ROOT/CIAO
mkdir -p $TREE_ROOT
cd $TREE_ROOT
svn co svn://svn.dre.vanderbilt.edu/DOC/Middleware/sets-anon/ACE+TAO+CIAO .
cd $ACE_ROOT/bin
rm *Tests.txt
rm *TestRev.txt
rm *Ignore.txt
rm *Builds.txt
./diff-builds-and-group-fixed-tests-only.sh

MAILTO="devo-group@list.isis.vanderbilt.edu"
MAIL="mail -S smtp=zimbra.remedy.nl"
MAILFROM="jwillemsen@remedy.nl"

MAIL_ATTACHMENTS=
for fn in `ls *Tests.txt`; do
   MAIL_ATTACHMENTS=$MAIL_ATTACHMENTS+"-a $fn "
done
for fn in `ls *NoTestRev.txt`; do
   MAIL_ATTACHMENTS=$MAIL_ATTACHMENTS+"-a $fn "
done
CURRENTDATE=`date -u +%Y_%m_%d`
mailfile="/tmp/rsmailfile"
{
   echo "Sending test statistics for" $CURRENTDATE
   echo
   cat *NoTestRev.txt
   echo
   echo "Sending with revision number"
   cat *Tests.txt
   echo
   echo "Sending results per build"
   cat *Builds.txt
} > $mailfile

$MAIL -r $MAILFROM -s "ACE/TAO/CIAO test statistics for $CURRENTDATE" $MAILTO < $mailfile

rm -f $mailfile

