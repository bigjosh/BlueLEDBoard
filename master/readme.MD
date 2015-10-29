### master

The actual software that drives an LED string. Pass the filename of the serial device (typically something like `/dev/ttyUSBx`) and the message you want to display

The message can have the following commands embeded in it...

Command | Description
---|---
`*Sn` | Strech chars by `n`. So `*S2` makes the following chars twice as wide
`*T`| Insert the current time/date with blinking colons

### ./go _device_

Set up the port and run `master`.

### ./goll

Run `go` on all ports that match `/dev/ttyUSB?`

### ./killall

Kill all running processes that match `master`.

### ./upload _device_

Upload the `BlueLEDBoard.cpp.hex` firmware to _device_ using `avrdude`.

### ./uploadall

Upload the `BlueLEDBoard.cpp.hex` firmware to all ports that match `/dev/ttyUSB?`

