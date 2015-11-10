#!/usr/bin/env bash
# This script should be run at startup, probably via a chrontab @boot line.
# It sets up everything for the blueled system and starts the scrolling processes

#First set up the runtime directory structure 
mkdir /var/blueled
while read p; do
  mkdir /var/blueled/$p
  cp defaultmessage.txt /var/blueled/$p/message.txt
done </etc/blueled/portlist


