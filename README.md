# BlueLEDBoard

Brain transplant for vintage Blue Man Group scrolling LED boards. 

![Alt text](images/wall.jpg?raw=true)

## Demo

https://www.youtube.com/watch?v=ddl3uFbX3zA

## Highlights

* Saves (and even uses!) almost all of the old hardware.
* Easy to install into vintage controller boards - just pull the CPU chip out and plug the new daughterboard in. No modifications to the LED modules or connecting circuitry. 
* Daughterboards communicate to master controller using standard off-the-shelf RS232 DB9 links, connectors, and cables.
* Master controller can be any computer that supports RS232. Currently daughterboards are connected to a Windows machine for development and a Raspberry PI for deployment.
* Timing-critical row and col scanning happens in hardware on the daughterboards. Master controller sends high-level frame bit maps. 
* Daughterboards use hardware SPI to clock pixels out to LED modules at up to 8MHz.
* Daughterboards emulate an Arduino, so you can seamlessly download firmware updates in-place over the existing serial connections using the Arduino IDE or automatically via command line with AVRDUDE.
* The master controller software uses platform neural filenames for accessing daughterboard communication links, so highly portable. 
* Master controller software dynamically reads messages from flat text files, which can be updated in real-time. 
* User defined fonts specified in easily editable flat text files. 
* Extensible system for embedding display commands in messages files. 

## Overview of operation

1. Users edit message and font files and save them to a dropbox account. 
2. The Raspberry Pi master controller uses an internet connection to download the messages and font file from dropbox once a minute.
3. There is one copy of the mater controller software running for each string of LEDs. Each copy reads its messages and font files to generate a bitmap images of the on and off LED pixels for its attached string. 
4. 80 times per second, each copy of the master controller software transmitts a single frame of  bitmapped pixel data to its  daughter board over the RS232 link. To synchronize these frame updates, each daughterboard transmitts a vertical refresh signal back to the master controller each time it completes a display refresh cycle. 
5. Each daughter board keeps two buffers of pixel data - one that is actively being displayed and one that is being recieved from the master controller over the serial link. The two bufferes are swapped each time a full frame is recieved from the serial link.
6. The daughterboard refreshes the display one row at a time based on the data in the current pixel buffer.
7. To update a row, the daughterboard first turns of the previous row and then shifts the pxiels for the new row into the shift registers on the LED modules. It does this by putting the next pixel on the data line and toggling the clock line to shift all the short registers one stepo to the right. 
8. When all of the pxiels for a row have been loaded into ther shift registers, the daughterboard turns the row on by activating the row drivers on the controller card.
9. When the daughterboard gets to the end of the 7th (last) row, it sends a 'V' to the master controller. This lets the master control sync up to the refresh cycle to avoid visual tearing that could happen of the frame was updated partially though a refresh cycle. 



## Hardware

A custom daughter-board replaces the CPU on each vintage 
controller card. This board takes over all the signals necessary to drive the LED strings. 

The board includes a microprocessor that is responsible for the high-speed and timing critical task of constantly refreshing the LED display one row at a time.

The board is connected by pigtail to a female DB9 RS232 serial port. 

The board is designed to act like a standard Arduino UNO, making it possible to use tools like AVRDUDE to download firmware updates over the serial connection.

All the design files and instructions for the daughterboards are [here](Daughterboard PCB).


  
## Software

### Daughterboard

The software running on the daughterboard has an internal display buffer that keeps a bitmapped image of the pixels to be displayed on the LED string. It uses an internal timer to constantly send out clocked bitstreams to refresh the LEDs string one row at a time. 

The daughterboard is also constantly listening for new incoming data on the serial port that is uses to update the internal display buffer.

Daughterboard firmware is written in AVR GNU C and is available [here](Daughterboard%20PCB/firmware/Arduino/BlueLEDBoard) as an Arduino IDE style package. Note that you can actually download the firmware directly to a daughterboard over the serial connections using the Arduino IDE- just set the Arduino port to the serial port that the daughter board is connected to and the board type to "Uno". 

### Master Controller
The daughterboards are connected via RS232 the master controller software which currently runs on a Raspberry PI. This code orchestrates the activity for the entire installation and sends update to the individual strings as necessary.       

The master controller software written in standard C and is available [here](master/BlueManLEDMaster). Note the code can run on either a Windows or Linux master controller and a Linux `make` file is provided.

### User Interface
Currently there are scripts that run once per minute that download messages and fonts from a dropbox.com account. New messages are seamless spliced to the end of the currently running message. If a change in the font file is detected, all the connected strings are restarted with the updated font.  

These scripts are written in Linux BASH and run as systemd service and timer units. 

## Connections

The connections between the daughterboards and the master controller are all straight RS232 running at 1Mbps. The daughterboards are DTEs and the master is DCE. The daughterboards can be plugged directly into the master if the wires reach, otherwise "extension" M-F cables can be used. You can also use standard pin-swapping RS232 cables if you also add a null modem at one end to switch the pins back.

In normal operation, only TX and RX wires are needed. The master controller transmits image data to the daughter a frame at a time as a bitmap, and the daughterboard sends a single 'V' char back to the master controller after each vertical retrace to help synchronize frame delivery. 

Both TX and RX connections are needed to support remote firmware upgrades, and the CTX line is needed to support automatic resetting of the daughterboards to initiate a firmware update cycle.   

Currently the Raspberry PI master controller uses two of these USB to RS232 dongles to connect to all 8 daughterboards....
 
http://amzn.to/1QEHOEJ

But any RS232 connection would work fine since the master software uses the device file names to write to the daughterboard links.


## Limits 

Current software-defined limits are available [here](limits.MD). 

