#!/usr/bin/env bash
# This script takes a serias of photos on an attached SLR to be post processed and eventually assembled into 
# a map of every LED location in the display

# run though all of the connected boards based on ports listed in /etc/blueled/portlist

mv calibration_shots calibration_shots_$(date +%H%M%S).jpg

mkdir calibration_shots

gphoto2 --capture-image-and-download --auto-detect --filename=start$(date +%H%M%S).jpg

for p in ttyUSB0 ttyUSB1 ttyUSB3 ttyUSB4 ttyUSB5 ttyUSB7 
do
    #turn off
    ./blueled /dev/$p -C100

done

gphoto2 --capture-image-and-download --auto-detect --filename=off$(date +%H%M%S).jpg


for p in ttyUSB0 ttyUSB1 ttyUSB3 ttyUSB4 ttyUSB5 ttyUSB7 
do

  for i in {0..7}
    do
  
      #turn row at a time
      ./blueled /dev/$p -R$1
      gphoto2 --capture-image-and-download --auto-detect 
      mv -f capt0000.jpg calibration_shots/$p-R$(printf "%03d" $i).jpg    
      
    done
done



for p in  ttyUSB4 ttyUSB5 ttyUSB7 
do
  
  for i in {0..50}
  do
    
    #Set the correct col
    ./blueled /dev/$p -C$i
    gphoto2 --capture-image-and-download --auto-detect 
    mv -f capt0000.jpg calibration_shots/$p-$(printf "%04d" $i).jpg    
          
  done
  
done 

