# master

The actual software that drives an LED string. Pass the filename of the serial device (typically something like `/dev/ttyUSBx`), the message you want to display, and the font file.

##Message File

The message file is just a normal text file that contains the message you want to scroll. It is 8-bit ascii and each character is generated based on the bitmaps in the current font file.

#Commands  

The message file can have embedded commands that insert dynamically generated text into the  message at the location of the command, or alter the display of chars following the command in the message.

Every command consists of a `*`followed by a command specifier. The command specifier indicates which command you want to execute. Some commands also have a value that follows the letter.   

Command | Description 
---|---
`**` | Escape `*`. Just inserts a single `*` at the current location. 
`*Xx`| Insert the ascii char `x` where `x` is a 2 digit hex number. This is particularly useful for extended codes above 80 that are hard to consistently type on different editors. `*X3F` inserts a `?` at the current location because Ascii code hex 3F is a question mark.
`*On`|Moves the carriage back `n` pixels to overstrike next char- just like a typewriter. `X*O6-` would generate an ugly looking `*` becuase it would first print the `X` and then move back 6 pixels (5 for the char width and 1 of padding) and then print an `-` over the `X`.  
`*Sn` | Stretch following chars by `n`. So `*S2` makes the following chars twice as wide.
`*Tf` | Insert various formats of the current time. The `f` is the format specifier and can have any valid [strftime valid format code ](https://www.google.com/webhp?sourceid=chrome-instant&ion=1&espv=2&ie=UTF-8#q=strftime%20format%20specifiers)(drop the `%` in front of the code). So, `*TA` would print `Monday` (assuming today is Monday) because the `A` specifier is "replaced by the locale's full weekday name". You can string several time commands together to make pretty much any time string you want - `*TH:*TM` would be the current time as hh:mm. 
`*Pn` | Set interchar padding. This is the number of blank pixels between adjacent chars and defaults to 1. `*P0` runs the letters into each other and `*P3` makes words look very sparse.
`*D` | Inserts the name of the device this string is attacked to. Handy for debugging. 
`*G` | Inserts  the current ping time to google.com. Also handy for debugging. 