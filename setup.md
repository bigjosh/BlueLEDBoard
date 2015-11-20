# Setup instructions

## Rasberry PI

### blueled install

You can download the full blueled package from github by entering...

`git clone https://github.com/bigjosh/BlueLEDBoard.git`

...and then install it by changing into the `master` directory with...

` cd BlueLed/master`

...and running...

`sudo ./install.sh`.

The install script in the `master` directory takes care of all the blueled specific setup by coping files to where they need to go and starting up the services. You should then do...

`nano /etc/blueled/portlist`

...and make sure it contains the correct list of serial port devices that have daughterboards attached to them. 

The reboot the Pi and everythign should start scrolling!

### other software

Additionally, you'll probably need to...

1. Get networking set up so the Pi can get to the internet. This could be as easy as plugging in an ethernet cable, or harder if you need to connect to a Wifi access point.
2. Change the username and/or password from the default so random people don't try to log in and mess stuff up. 
3. Install a remote access path like Weaved or PageKite so you can remotely log into the board to make changes and update software.
4. Install AVRDUDE if you want to be able to download new firmware into the daughterboards.


## Hardware

1.	Take the Raspberry Pi to space where it will be living and plug it in to power using the included cable and adapter. You should see blinking lights. Text or call me to let me know it is up so I can check if the pre-configured Wifi connection is good.  Once it is working with the known-good adapter and cable, you can switch to a hardwired power supply, be really careful not swap polarity or connect to more than 5 volts DC. Pi’s are really sensitive. 
2.	Take one controller card and unplug IC1 and IC4. 
3. Take a daughterboard and plug it into socket IC1. You can do this with power on or off. The pigtail should come out on the side opposite the row of capacitors like this…

 

   Be careful that the pins of the daughterboard line up with the socket and click in good. You should see a test pattern show up on the LEDs that looks like horizontal wipe, a vertical wipe, and finally a hash pattern. If you see nothing, could be bad power, board upside, or misaligned pins. If you see movement but not a nice test pattern, check that all the pins are seated well. 
4. Repeat steps 2 & 3 for each of the 8 daughterboards and controller cards.
3.	Plug the two USB Serial cables into the two free ports on the Pi. It doesn’t matter which plug goes in which USB socket. 
4.	Connect a pigtail to a USB serial cable DB9, using a serial cable if necessary. It doesn’t matter which pigtail goes to which DB9. Assuming the Wifi connection in step #1 was good, when you plug it in you should see a scrolling test message on the LEDs. If you don’t see the test message and you are using a serial cable, try adding a null-modem on either end of the serial cable (but not both).  You can also try connecting the pigtail directly into the USB cable DB9 if it will reach. Repeat for all 8 daughterboards.
5.	If you have all the cables hooked up and see good test messages on all the strings, then all clear to mount everything. Try to keep everything running while you are mounting to immediately notice if anything gets messed up. 
