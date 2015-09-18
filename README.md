# BlueLEDBoard
Brain transplant for vintage Blue Man Group scrolling LED boards

This code adds functionality while saving (and using!) almost all of the old hardware. To make the brain transplant, 
you remove the cpu (IC1) from the coltroler card and connect the pins from an Arduino like this...

| Pin on CPU socket  | Arduino Pin  | Function | Notes |
| :------------ |:---------------| :-----| :-----|
| 1-3      |  D8-D10 | Row Select Bits 0-2 | Row=0 the bottom row on the display. Row=7 does not select any row (used for shifting out cols). |
| 4      | Ground  |   Row Select Bit 3 | The display only has 7 rows, so this bit can be tied LOW. You could also connect this to an outpin pin to be able to blank the display with a single bit since setting this to 1 would select a non-existant row no matter what the other row select bits are.)  |
| zebra stripes | are neat        |    $1 |

Note that pin 1 on the socket is the one closest to the "IC1" silk label. 
