#!/bin/bash
#grab updated messages files from dropbox and atomically copy to running controllers

#make a temp file to download into 

dbget () { 
  TMPFILE=`mktemp`
  wget "$1" -O $TMPFILE
   
  if [ $? -eq 0 ]      ## WGET success?
   then
     # atomically move the new file
     mv -f $TMPFILE /tmp/blueled/$2/message.txt
   else
     #delete any left over output file 
     rm $TMPFILE
   fi  
}

unzip -d $PWD $TMPFILE
rm $TMPFILE


dbget "https://www.dropbox.com/s/rjtx9s7yabyw8n0/ttyUSB0.txt?dl=0" ttyUSB0
dbget "https://www.dropbox.com/s/7cz0fsznlvt5rw7/ttyUSB1.txt?dl=0" ttyUSB1

