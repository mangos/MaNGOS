#!/usr/bin/python

import sys
import re
import os
import string

print """WARNING:  this script is dumb.  I mean, really, really dumb.  Every file is treated
as a text file, so if you are checking in any binary files, YOU MUST set a non-text
MIME type by hand, otherwise it WILL be corrupted by the checkout process.
A better approach will be to add the unmatched files to the config file in
ACE/docs/svn/config (and update yours!) so others won't have to put up with them 
in the future.

To use this program, copy and paste the output from the svn command into standard
input.
"""

foo = raw_input("That being said, if you want to continue, press enter")

sin, sout = os.popen2 ("svn info")
sin.close ()
os.wait ()

url = ""
root = ""
path = ""

for line in sout.readlines ():
    if line.startswith ("URL: "):
        url = line.replace ("URL: ", "")[:-1]
    if line.startswith ("Repository Root: "):
        root = line.replace ("Repository Root: ", "")[:-1]

path = url.replace (root, "")[1:] + '/'
files = ""

eol_style = " svn ps svn:eol-style native "
keywords = " svn ps svn:keywords 'Author Date Id Revision' "

for line in sys.stdin.readlines ():
    ln = line[0:line.find (':')] + ' '
    ln = ln.replace (path,"")
    os.system (eol_style + ln)
    os.system (keywords + ln)



