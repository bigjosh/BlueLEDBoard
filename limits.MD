# Current limits

## Refresh Rate = 80 FPS

This is set in the daughtboard firmware. Since the LEDs much be off when shifting in a new row of data, and the rate that 
new data can be shifted in is limited, then the higher the frame rate the dimmer the display. 80 fps give s good combination of 
smooth and flicker free animation whchout loosing too much brightness.

## RS232 data rate = 1,000,000 baud

THis is set in both the Daughtboard firware and using `stty` on the master controller. This is fast enough to keep up with the 
daughterboard frame rate but also a normal baud rate that is supported by Linux and compatibile with normal cables. 

## LED bit shift rate = 8Mhz

The shift registers on the vinatge moduldes are rated for 20Mhz, but the daughter board only runs at 16Mhz and we are using the 
SPI hardware which is lmited to 1/2 clock speed. The current firmware does not utilize the 8Mhz fully becuase of gaps between outgoing
bytes. This is likely a bug in the chip and more work will probably find a worek around. 

## String Length = 84 chars (1 char = 5 columns)

This is set in the daughterboard firmware. Longer strings require more time to shift so look dimmer. This was set to accomidate the longest 
string in the existing installation. 





