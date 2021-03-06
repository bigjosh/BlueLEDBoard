##Pins used  

To make the brain transplant, you remove the cpu (IC1) from the coltroler card and connect the pins from an Arduino like this...

![Alt text](images/CPU%20socket%20connections.jpg?raw=true)

| Pin on CPU socket  | Arduino Pin  | Function | Notes |
| :------------ |:---------------| :-----| :-----|
| 1-3      |  D8-D10 | Row Select Bits 0-2 | Row=0 the bottom row on the display. Row=7 does not select any row (used for shifting out cols). |
| 4      | Ground  |   Row Select Bit 3 | The display only has 7 rows, so this bit can be tied LOW. You could also connect this to an outpin pin to be able to blank the display with a single bit since setting this to 1 would select a non-existant row no matter what the other row select bits are.)  |
| 7 | D3 | Clock A        |   Note that the two clock signals are ORed together, so you could use either or both  | 
| 17 | Ground | Clock B | |
| 32 | D4 | Data | 
| 40 | 5V+ | Power | You can grab power form this pin to power the Arduino |

Note that pin 1 on the socket is the one closest to the "IC1" silk label. 

All the notes on figuring this out are here...

https://www.evernote.com/pub/bigjosh2/bluemanleds


##Stealing Digital Power

You also must provide power to the digital circuits on the board. This ws previously connected using a second set of wires coming off the back of the board leading to a 7 VDC supply, but these are unnessisary since all the digital logic can run on 5 volts already present on the board for driving the LEDs. You can make that connection either by sending 5 VDC into pin 40 of the CPU socket, or by making a solder jump here on the back of the baord...

![Alt text](images/Digital%20Power%20Jump.jpg?raw=true)

