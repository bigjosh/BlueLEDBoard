#!/usr/bin/env bash
# start all of the connected boards based on ports listed in /etc/blueled/portlist

while read p; do
  blueledrunport.sh $p 
done </etc/blueled/portlist 