# Current limits

## Refresh Rate = 80 FPS

This is set in the daughterboard firmware. Since the LEDs must be off when shifting in a new row of data, and the rate that 
new data can be shifted in is limited, then the higher the frame rate the dimmer the display. 80 fps gives a good combination of 
smooth and flicker free animation without loosing too much brightness.

Note that the text scrolling only looks smooth when it is an integer multiple of the frame rate.  

## RS232 data rate = 1,000,000 baud

This is set in both the Daughterboard firware and using `stty` on the master controller. This is fast enough to keep up with the 
daughterboard frame rate but also a normal baud rate that is supported by Linux and is compatibile with normal cables. 
## LED bit shift rate = 8Mhz

The shift registers on the vinatge modules are rated for 20Mhz, but the daughter board only runs at 16Mhz and we are using the 
SPI hardware which is lmited to 1/2 the clock speed. The current firmware does not utilize the 8Mhz fully becuase of gaps between outgoing
bytes. This is likely a bug in the chip and more work will probably find a work around.

The current Las Vegas display is running with a clock rate of only 1MHz because there were stability problems with higher clock rates on long strings due to electrical characteristics of the clock line, which is a single unbuffered wire.   

## String Length = 504 columnms

This is set in the daughterboard firmware. Longer strings require more time to shift so look dimmer. This was set to accommodate the longest 
string in the existing installation. 





