#!/usr/bin/env bash
# This script will start up a single instance of the LED master controller pointing to the specified device
# note: only pass the device name and not the /dev directory
# Typically called from startall
# It kills any previously running instance on that device
# Note that it seems like I should be using /var for all this stuff, but then this file would not 
# work when run by non-root

if [ ! -e /tmp/blueled ]; then
    mkdir /tmp/blueled
fi

if [ ! -e /tmp/blueled/$1 ]; then
    mkdir /tmp/blueled/$1
    cp /etc/blueled/defaultmessage.txt /tmp/blueled/$1/message.txt
fi

if [ -e /tmp/blueled/$1/pid ]; then
  echo killing $(cat /tmp/blueled/$1/pid)
  kill $(cat /tmp/blueled/$1/pid)
fi 

stty -F /dev/$1 1000000 raw clocal -hupcl -echo
blueled /dev/$1  /tmp/blueled/$1/message.txt &
echo $! >/tmp/blueled/$1/pid
