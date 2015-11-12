// Main code for driving the LEDs. Only opens the LED serial port as a standard file so no 
// hardware dependancies. 

// Yeay!

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>			// memset on linux
//#include <io.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>	// gettimeofday
#include <ctype.h>
//#include <sys/time.h>		


#include "blueled.h"
#include "font5x7.h"


unsigned char dots[ROWS][PADDED_COLS];


int fd;			// File desriptor for the serial port

unsigned char buffer[BUFFER_SIZE];

struct timeval lastSend = { 0,0 };


// Clean out any pending bytes in the serial read buffer 

void purgeSerial() {
	int flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);			// Turn off blocking
	char dummy;
	while( read(fd, &dummy, 1) > 0 );					// Keep reading any pending bytes in the buffer until none left
	fcntl(fd, F_SETFL, flags );						// Set the flags back to what they were before we went non-blocking
		
}

unsigned int lag=1;             // Number of syncs between scrolling steps

// Serialize the dot buffer out the Serial port
// Waits for the next sync pulse to actually send

void sendDots() {

	int dotCount = 0;

	unsigned char *p=buffer;

	for (int r= 0; r < ROWS; r++ ) {

		int bit = 0;

		int out = 0;

		for (int c = 0; c < PADDED_COLS; c++ ) {
		
			if (dots[r][c]) {

				out |= 1 << bit;

			}

			bit++;

			if (bit == 8) {

				*(p++) =out;


				dotCount++;

				out = 0;
				bit = 0;
			}

		}

	}

	char c;

	// Wait for sync form daughterboard so we send frames at correct speed
	// and don't overflow the buffer
	// We do this after everything is completely ready to go to make sure
	// we get max time to do computation between frames and to ensure
	// minimum delay until we actually send.
	// This depending on the non-intuitive behavior of read which is that it will
	// block until at leat 1 byte is available.

	for(unsigned int i=lag; i>0; i--) {             // Wait for _lag_ many syncs
        read(fd, &c, 1);
    }        
	
	// Ok, all clear to send a new frame buffer	

	write( fd,  buffer , BUFFER_SIZE  );

/*
	struct timeval t;
	unsigned long elapsedTime;

	gettimeofday(&t2, NULL);

	// compute and print the elapsed time in millisec
	elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000;      // sec to ms
	elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms

*/
//	sleep(100);

	//printf("padded=%d, rows=%d, buffersize=%d, dotCount=%d\r\n", PADDED_COLS, ROWS , BUFFER_SIZE ,  dotCount);

}


void clear() {

	memset(dots, 0x00, ROWS*PADDED_COLS);

}


// TODO: Import font at runtime from file
// Permit variable width

int draw5x7(int x, char c, int strech) {

	unsigned char *rowbits;
	
	if ( c<0x20 || c>0x80 ) {					// Only try supported chars to avoidarray  out of bounds. Also ignores line feeds/cr/
		return(0);
		
	}
	
	rowbits = &Font5x7[ (c - 0x20) * 5 ];
	
	int xoffset =0; 	// current col 
	
	for (int col = 0; col < 5; col++) {
		
		for( int s = 0; s<strech; s++ )	{			// Strech
		
			int dotCol = x + xoffset; 
	
			if (dotCol >= 0 && dotCol < COLS) {				// Check clipping rectangle
	
				for (int row = 0; row < 7 ; row++) {
	
					if (*rowbits & (1 << row)) {
	
						dots[row][dotCol] = 1;
	
					}
				}
			}
			
			xoffset++;
		}

		rowbits++;

	}
	
	return( xoffset );

}

//#define MESSAGE "This string of LEDs is connected to port %s of the master controller. It is currently <STRFTIME=%A %B %d, %Y at %r %Z>.   "

#define TIMESTRINGLEN 100		// Just a conservative guess

char timestringBuffer[TIMESTRINGLEN];

char speciferBuffer[] = { '%' , '_' , 0x00} ;    // The _ will be replaced with a char

const char *timestring( char specifier ) {
	
		time_t t;

		t = time(NULL);
		
		struct timeval start;	
		
		gettimeofday(&start, NULL);
   
    	speciferBuffer[1] = specifier;
		
    	if (strftime( timestringBuffer , TIMESTRINGLEN , speciferBuffer , localtime( &t ) ) ) {
											
		  return( timestringBuffer );
        
    	} else {
        
        	return( "" );
        
    	}
		
}

#define PATH_MAX 200
char pingString[PATH_MAX];
int pingRun=0;

const char *ping() {
	FILE *fp;
	int status;
	
	if (pingRun) return( pingString );
	
	fp = popen(" (ping -c 1  google.com || echo Fail )  | tail -1", "r");    // This mess is to handle the case where the ping fails, so the OR goes to the echo. Really linux, really there is no Way to have ping output just the results?
	if (fp == NULL) {
		/* Handle error */;
		return("[popen error]");			
	}
	
	
	while (fgets(pingString, PATH_MAX, fp) != NULL);	
	
	status = pclose(fp);
	
	sleep(1000);			// Pause for at least 1 second so it doesn't just look jerky
	pingRun=1;
	purgeSerial();			// That could have taking a sec, so clear any pening vertical refresh signals that came in while we waited to avoid jerking

  if (status) return("Failed!"); 
	return( pingString );
		
}

// remember the device name passed on the command arg

const char *devArg;


// Draw the string starting at col x
// Returns length of result in pixels


#define DEFAULT_CHAR_PADDING 1

int drawString( int x , const char *s ) {
	
	int strech =1;
	int padding = DEFAULT_CHAR_PADDING;
	
	int xoffset=0;
	
	while (*s) {
		
		if (*s == '*' ) {
        
			s++;
			
			switch (*s) {
                
                case '*': {                // Two *'s just means escape out a single *
					s++;
                    xoffset += draw5x7(x+xoffset, '*' , strech);
                }
                break;                    
				
				case 'S': 	{			// Set strech
					s++;
					if (*s && isdigit(*s)) {
						strech = *s - '0';
						s++;
					} else {
                       xoffset += drawString( x+xoffset , " [S without strech amount] " ); 
                    }
					
				}  
				break;
				
				case 'T': {				// Insert time
				
					s++;
					
                    if (*s && isalpha(*s)) {						
					    xoffset += drawString( x+xoffset , timestring( *s ) );
					    xoffset += padding;
						s++;
                    } else {
                        xoffset += drawString( x+xoffset , " [T without format value] " );
                    }                        

					
				}
				break;
                
				case 'L': {				// Scroll lag
    				
					s++;
    				if (*s && isdigit(*s)) {
                        lag = *s - '0';
						s++;
                    } else {
                        xoffset += drawString( x+xoffset , " [L without lag digit] " );                        
    				}

    				
				}
				break;
                
				case 'P': {				// Interchar padding
    				s++;
    				if (*s && isdigit(*s)) {
        				padding = *s - '0';
						s++;
        			} else {
            			xoffset += drawString( x+xoffset , " [P without padding digit] " );
        			}

        				
    			}
    			break;
                
                

				case 'D': {				// Insert device name 
					s++;
					xoffset += drawString( x+xoffset , devArg );
					xoffset += padding;
					
				}
				break;


				case 'G': {				// Ping Google...
					s++;		
					const char *p = ping();		// Get the ping message
					xoffset += drawString( x+xoffset , p );
					xoffset += padding;
					
				}
				break;

				
			}		
			
		} else {
			

			xoffset+=draw5x7(x+xoffset, *s , strech);
	
			xoffset += padding;
			
			s++;
		}
	}
	
	return(xoffset);
		
}

// Draw the string repeasedly to the left until the display is full
// x is the col where you want to stop drawing. The end of the string will stop there.
// if x<=0 then nothing is drawn

void drawStringFillLeft( int x , int len, const char *s ) {
	
	if (len==0) return;		// Special case: we can never fill using a zero len string
	
	while (x>0) {
		
		x-=len;
		
		drawString( x , s );
				
	}
	
}

void drawStringFillRight( int x , int len, const char *s ) {

	if (len==0) return;		// Special case: we can never fill using a zero len string
	
	while (x<COLS) {
				
		drawString( x , s );
		
		x+= len;
				
	}
	
}


int main(int argc, char **argv)
{
	printf("BlueMan master controller Serial LED Driver\r\n");
	
	if (argc!=3) {
		printf( "Usage: blueled deviceName messagefile\n\r");
        printf(" Where: deviceName is the path to connected serial device (typically /dev/ttyXXXXX)\r\n");
		printf("        messagefile is the name of a file with the message to scroll\r\n");
        printf("        Reads a series of display strings from stdin and pumps them to the LED display\r\n");
		return(2);
	}


	fd = open(argv[1] , O_RDWR );

	if (fd == -1 ) {
		printf("failed to open serial device %s\r\n", argv[1]);
		return(1);
	} else {
		printf("Success opening serial device %s\r\n", argv[1]);   
        devArg = argv[1];
	}

    sleep(1000); 		// Let bootloader timeout

	char *prevMessage = (char *)malloc(1);		// Keep a copy of the previous message for smooth transitions
	prevMessage[0]=0x00;
	int prevWidth=0;
	
    size_t len = 0;

    purgeSerial();  // start fresh to avoid jerks

    while (1) {
		
		FILE *f = fopen( argv[2] , "r");
		
		if (!f) {
			printf("Could not open %s.\r\n", argv[2]);
			exit(3);
		}

		// Find length of file		
		fseek(f, 0L, SEEK_END);
		int messageLen = ftell(f);
		
		//Go backj to begining
		fseek(f, 0L, SEEK_SET);
				
		char *message = (char *)malloc( messageLen );
		
		if (!message) {
			printf("Malloc failed!\r\n");
			return(4);
		} 
		
		fread(message, 1 , messageLen , f );
		
		fclose(f);
				
		pingRun=0;		// Only run ping once per cycle
        lag=1;          // Default normal scroll speed

		int width = drawString( 0 , message );
					
		// First we transision from the previous message to the new none
		// we always end with the last message col of the message on the last col of the display, so pick up from there using the previous message 
		
		for( int s= COLS; s >0 ; s-- ) {
			clear();
			drawStringFillLeft( s , prevWidth , prevMessage );
			drawStringFillRight( s , width , message );
			sendDots();
		}
		
		// Ok, now we are past the end of the previous message, now scroll out just the new message
		

		for(int s = 0 ; s > (COLS-width) ; s--) {		// Start of rightmost copy of message

			clear();
			drawStringFillRight( s , width , message );
			sendDots();

		}
		
		free(prevMessage);
		prevMessage=message;
		prevWidth=width;
		
	}
	
	// Never get here
    
    free(prevMessage);
    
	return 0;
}
