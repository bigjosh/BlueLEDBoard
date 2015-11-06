# Setup instructions

1.	Take the Raspberry Pi to space where it will be living and plug it in to power using the included cable and adapter. You should see blinking lights. Text or call me to let me know it is up so I can check if the pre-configured Wifi connection is good.  Once it is working with the known-good adapter and cable, you can switch to a hardwired power supply, be really careful not swap polarity or connect to more than 5 volts DC. Pi’s are really sensitive. 
2.	Take one controller card and unplug IC1 and IC4. 
3. Take a daughterboard and plug it into socket IC1. You can do this with power on or off. The pigtail should come out on the side opposite the row of capacitors like this…

 

   Be careful that the pins of the daughterboard line up with the socket and click in good. You should see a test pattern show up on the LEDs that looks like horizontal wipe, a vertical wipe, and finally a hash pattern. If you see nothing, could be bad power, board upside, or misaligned pins. If you see movement but not a nice test pattern, check that all the pins are seated well. 
4. Repeat steps 2 & 3 for each of the 8 daughterboards and controller cards.
3.	Plug the two USB Serial cables into the two free ports on the Pi. It doesn’t matter which plug goes in which USB socket. 
4.	Connect a pigtail to a USB serial cable DB9, using a serial cable if necessary. It doesn’t matter which pigtail goes to which DB9. Assuming the Wifi connection in step #1 was good, when you plug it in you should see a scrolling test message on the LEDs. If you don’t see the test message and you are using a serial cable, try adding a null-modem on either end of the serial cable (but not both).  You can also try connecting the pigtail directly into the USB cable DB9 if it will reach. Repeat for all 8 daughterboards.
5.	If you have all the cables hooked up and see test messages on all the strings, then all clear to mount everything. Try to keep everything running while you are mounting to immediately notice if anything gets messed up. 
