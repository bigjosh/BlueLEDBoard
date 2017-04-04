#!/bin/bash
#grab updated messages files from dropbox and atomically copy to running controllers

#make a temp file to download into 

dbget () {
  if [ -e /tmp/blueled/$2 ]; then    ## Dont bother getting the file if port as now been set up
  
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
     
  fi  
}



dbget "https://www.dropbox.com/s/qff4av2zhk783sw/ttyUSB0.txt?dl=0" ttyUSB0
dbget "https://www.dropbox.com/s/qb2f4xsblxw77bi/ttyUSB1.txt?dl=0" ttyUSB1
dbget "https://www.dropbox.com/s/yndfgmppofuqrxd/ttyUSB3.txt?dl=0" ttyUSB3
dbget "https://www.dropbox.com/s/bqv8tlfr1oom5dy/ttyUSB4.txt?dl=0" ttyUSB4
dbget "https://www.dropbox.com/s/xobellkt6kr8l11/ttyUSB5.txt?dl=0" ttyUSB5
dbget "https://www.dropbox.com/s/cp93qrrtwcxchdx/ttyUSB7.txt?dl=0" ttyUSB7

# Now we check to see if there is a new font file

TMPFONTFILE=`mktemp`
wget "https://www.dropbox.com/s/ww2byk2zfkbptz2/font.txt?dl=0" -O $TMPFONTFILE

if ! diff  $TMPFONTFILE /tmp/blueled/font.txt    ## is the new font different?
  then
  
  ## Font file changed! Copy to correct location when running instances will read from...
  
  mv $TMPFONTFILE /tmp/blueled/font.txt
  
  ## Now reset the running instances so they get the new font.
  ## I knwo this is brutal, but it is simple and works. We cna come up
  ## with a gentler aproach if/when ever nessisary
  
  systemctl stop blueled.service
  systemctl start blueled.service
  
else

  # clean up dupe file
  rm $TMPFONTFILE
  
fi
