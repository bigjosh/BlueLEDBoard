# BlueLEDBoard


Brain transplant for vintage Blue Man Group scrolling LED boards. This code adds functionality while saving (and using!) almost all of the old hardware. 

![Alt text](images/demo.jpg?raw=true)

## Demo

https://www.youtube.com/watch?v=ddl3uFbX3zA

## Hardware

A custom daughter-board replaces the the CPU on each vintage 
controller card. This board takes over all the signals necessary to drive the LED strings. 

The board includes a microprocessor that is responsible for the high-speed and timing critical task of constantly refreshing the LED display one row at a time.

The board is connected by pigtail to a female DB9 RS232 serial port. 

The board is designed to act like a standard Arduino UNO, making it possible to use tools like AVRDUDE to download firmware updates over the serial connection.

  
## Software

### Daughterboard

The software running on the daughterboard has an internal display buffer that keeps a bitmaped image of the pixels to be displayed on the LED string. It uses an internal timer to constantly send out clocked bitstreams to refresh the LEDs string one row at a time. 

The daughterboard is also constantly listening for new incoming data on the serial port that is uses to update the internal display buffer.

### Master Controller
The daughter boards are connected via RS232 the the master controller software which runs on a Raspberry PI. This code orchestrates the activity for the entire installation and sends update to the individual strings as necessary.       

### User Interface
The Raspberry PI also hosts code that lets the user update the messages and schedule changes.  


## Connections

The connections between the daughterboards and the master controller are all straight RS232 running at 1Mbps. The daughterboards are DTEs and the master is DCE. The daughterboards can be plugged directly into the master if the wires reach, otherwise "extension" M-F cables can be used. You can also use standard pin-swapping RS232 cables if you also add a null modem at one end to switch the pins back.

In normal operation, only 2 wires are needed to transmit image data from the master to the daughterboards. Both TX and RX connections are needed to support remote firmware upgrades, and the CTX line is needed to support automatic resetting of the daughterboards to initiate a firmware update cycle.   

Currently the Raspberry PI master controller uses two of these USB to RS232 dongles to connect to all 8 daughterboards....
 
http://amzn.to/1QEHOEJ

But any RS232 connection would work fine since the master software uses the device file names to write to the daughterboard links.


