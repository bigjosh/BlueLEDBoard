#!/usr/bin/env bash
#One time install the blueled system

#build the executable
make
# Damn, I've read like 10 articles about where I *should* put the exeutable and cant figure out if this is right. Really linux?
cp blueled /usr/local/bin

if [ ! -e /etc/blueled ]; then
  mkdir /etc/blueled
fi

if [ ! -e /etc/blueled/defaultmessage.txt ]; then
    cp defaultmessage.txt /etc/blueled/
fi

# TODO: cp service files and enable 

if [ ! -e /etc/blueled/portlist ]; then
   # Assumes all ports are in /dev/ttyUSB?. Might not be true if you are using different hardware than the normal USB serial ocypuses.
   for f in /dev/ttyUSB?; do 
       basename $f >>/etc/blueled/portlist
   done
   echo "Please edit /etc/blueled/portlist and make sure it has the correct ports for actual attached controller boards"
fi
