#update firmware on specified device	
avrdude -p atmega328p -P $1 -c arduino -b 115200 -D -v -U flash:w:BlueLEDBoard.cpp.standard.hex:i
