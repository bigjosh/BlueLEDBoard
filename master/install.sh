#!/usr/bin/env bash
#One time install the blueled system
#must be run with sudo

#build the executable
make
# Damn, I've read like 10 articles about where I *should* put the exeutable and cant figure out if this is right. Really linux?
cp blueled /usr/local/bin

chmod +x *.sh

cp blueled*.sh /usr/local/bin 

if [ ! -e /etc/blueled ]; then
  mkdir /etc/blueled
fi

if [ ! -e /etc/blueled/defaultmessage.txt ]; then
    cp defaultmessage.txt /etc/blueled/
fi

#copy the serive unit files to the right place
sudo cp blueled.service $(pkg-config systemd --variable=systemdsystemunitdir)
#force a reload in case we are overwriting existing files with new versions
sudo systemctl daemon-reload
#set the service to run on boot
sudo systemctl enable blueled


if [ ! -e /etc/blueled/portlist ]; then
   # Assumes all ports are in /dev/ttyUSB?. Might not be true if you are using different hardware than the normal USB serial ocypuses.
   for f in /dev/ttyUSB?; do 
       basename $f >>/etc/blueled/portlist
   done
   echo "Please edit /etc/blueled/portlist and make sure it has the correct ports for actual attached controller boards"
   echo "then reboot or execute _sudo systemctrl start blueled_ to start"   
fi
