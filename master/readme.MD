# blueled

The actual software that drives an LED string.  One instance of this program drives on LED string. Typically multiple copies of this program are launched automatically at startup by a service unit. 

Pass the filename of the serial device (typically something like `/dev/ttyUSBx`), the message you want to display, and the font file.

The program automatically reloads the message file each cycle and splices the new message onto the end of the previous one. Font file is currently only loaded when the program is first launched, but this could be changed although it would probably be best to load the font files in a separate thread to avoid stuttering the scrolling messages. 

##Message File

The message file is just a normal text file that contains the message you want to scroll. It is 8-bit ASCII and each character is generated based on the bitmaps in the current [font file](#font-file). It can also include embedded commands as listed below. 

###Commands  

The message file can have embedded commands that insert dynamically generated text into the  message at the location of the command, or alter the display of chars following the command in the message.

Every command consists of a `*`followed by a command specified. The command specified indicates which command you want to execute. Some commands also have a value that follows the letter.   

Command | Description 
---|---
`**` | Escape `*`. Just inserts a single `*` at the current location. 
`*Xx`| Insert the ASCII char `x` where `x` is a 2 digit hex number. This is particularly useful for extended codes above 80 that are hard to consistently type on different editors. `*X3F` inserts a `?` at the current location because Ascii code hex 3F is a question mark.
`*On`|Moves the carriage back `n` pixels to overstrike next char- just like a typewriter. `X*O6-` would generate an ugly looking `*` because it would first print the `X` and then move back 6 pixels (5 for the char width and 1 of padding) and then print an `-` over the `X`.  
`*Sn` | Stretch following chars by `n`. So `*S2` makes the following chars twice as wide.
`*Tf` | Insert various formats of the current time. The `f` is the format specified and can have any valid [strftime valid format code ](https://www.google.com/webhp?sourceid=chrome-instant&ion=1&espv=2&ie=UTF-8#q=strftime%20format%20specifiers)(drop the `%` in front of the code). So, `*TA` would print `Monday` (assuming today is Monday) because the `A` specifier is "replaced by the locale's full weekday name". You can string several time commands together to make pretty much any time string you want - `*TH:*TM` would be the current time as hh:mm. 
`*Pn` | Set interchar padding. This is the number of blank pixels between adjacent chars and defaults to 1. `*P0` runs the letters into each other and `*P3` makes words look very sparse.
`*D` | Inserts the name of the device this string is attacked to. Handy for debugging. 
`*G` | Inserts  the current ping time to google.com. Also handy for debugging.

##Font file

The font file is just a text file that describes the bitmaps for each of the 255 possible ASCII chars that can occur in the [Message File](#message-file). 

Chars are listed sequentially in the file. Each char starts with a line 

A char definition starts with `CHAR=` followed by the ASCII code as a 2 digit hex number. Note that the message file can easily reference hard-type-type codes using the `*X` command.  

Chars are always a fixed 7 pixels high (that is how big the LED modules are), but can be any width. Note that interchar padding is dynamically controlled in the message file (with the *Pn command), so chars should probably completely fill the box width-wise.

The width of the first line of pixels establishes the width of the char. 

The following are all seen as on pixels: `* X x @`. Anything else besides a space is considered an off pixel

Lines that start with a hash are ignored. Blank lines not part of char data also ignored.
 
####Example definition for dollar sign (which is ASCII 36 decimal, 24 hex) char in font file:

    CHAR=24
    ..X..
    .XXXX
    X.X..
    .XXX.
    ..X.X
    XXXX.
    ..X..
    
## Building 

The `blueled.cpp` is straight C and can be rebuilt using the included `makefile`.
    
# Support Scripts    

These scripts are all in straight BASH and really should be organized into their own directory. 

The `blueled.service` unit is installed to run at startup and launches one instance of `blueled` per connected daughterboard as listed in `/etc/blueled/portlist`.

The `grabdropbox` service and timer units download the latest message and font files from dropbox once per minute. It also checks if the font file has changed and will restart the `blueled` instances if it has. 

## Install

The `install.sh` script copies everything in the correct location and installs the systemd units. It should be run as `sudo` (it will warn if you try to run with insufficient privilege). 

The install script will not overwrite existing config files in `/etc/blueled` so it is safe to re-run it to re-install after any changes. 

