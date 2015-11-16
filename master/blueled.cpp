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

#define DISPLAY_COLS COLS			// Definate this seporately in case we have a string different length than the definated buffer size

#if (DISPLAY_COLS > PADDED_COLS)

	#error The display cols must be less than the size of the buffer or we will get corruptions

#endif

typedef struct {
	
	int width;					// index to the starting byte in cols[] for this char. The first byte holds the start in cols of the first col of data. Skip to the next index byte to find when it ends
	unsigned char *cols;		// the actual bit data for each col. Malloced as char loaded since we don't know the width ahead of time. 
	
} chartype;

								// a font is an array of 256 chartypes, indexed by ascii. A null cols[] indicates the char is undefined

int parseHexDigit( char c ) {
	if (isxdigit(c)) {
		if (isdigit(c)) {
			return( c - '0');
		} else {
			return( tolower(c) -'a' + 10);
		}
	} else {
		return(0);
	}	
}

#define ONPIXELS "*#@Xx"

// Returns null on error

chartype *importFont( const char *fname ) {

	FILE *f = fopen( fname , "r" );
	if (!f) {
		printf(  "Could not open font file %s\r\n",fname);
		return(NULL);
	}
	
	chartype *font = (chartype *) calloc( 256 , sizeof( chartype));
	
	
	if (!font) {
		printf(  "Could not allocate font memory\r\n");
		return(NULL);		
	}

	char * line = NULL;
	size_t len = 0;
	ssize_t read;
		
	unsigned c=0;  
	int r=0;									// Current Row	
	
    while ((read = getline(&line, &len, f)) != -1) {
		
//		printf("Retrieved line of length %zu :\n", read);
//		printf("[%c]:>%s<", line[0] , line);
		
		
		if ( line[0] !='#' ) {		// ignore comments 
		
		
			if ( !strncmp(  "CHAR=" , line , 5 ) ) {			// New char?
			
			
				if (isxdigit( line[5] ) && isxdigit(line[6] )) {
					
					c =  (parseHexDigit(  line[5] ) * 16 ) + parseHexDigit( line[6]) ;															
					r =0;
									
				} 
				
			} else {			// read bit data
			
				if (r==0) {			// first row of new char
								
					// First discover the width of this new char
				
					int w=0;		
					
					for(int l=0; l<strlen(line);l++) {
						if (isprint( line[l])) {						
							w=l+1;
						}
					}
										
					font[c].width = w;
					
					if (font[c].cols != NULL ) {
						
						free( font[c].cols );			// So we don't leak mem incase this char was defined multipule times 
					}
					
					font[c].cols = (unsigned char *) calloc( w , 1 ); 
					
					// Ok, the new char slot is all set up
					
				}
				
				if (r<7) {			// Reading in rows?
				
					for(int col=0; col< font[c].width ;col++) {
						
						if ( strchr( ONPIXELS , line[col] )) {
							
							font[c].cols[col] |= (1<<r);
						}
					}					
					
					r++;	
				}
				
								
			}
			
			
		}
	}
	
	fclose(f);
	
	if (line) {
		free(line);
	}
       
	return(font);
	
}

static unsigned char undefinedCharCols[] = {0x7F, 0x41, 0x41, 0x41, 0x7F}; 	// A box that looks like this []	

static chartype undefinedChar = {
	5, 	
	undefinedCharCols	
};

// TODO: Import font at runtime from file
// Permit variable width

int draw5x7(int x, char c, int strech , chartype *font) {

	
	chartype *thisChar = &font[c];
			
	if (thisChar->cols == NULL )	{ 		// Undefined char
	
		thisChar = &undefinedChar;			// SHow the block
	
	}
	
	int width = thisChar->width;
			
	int xoffset =0; 	// current col 
	
	for (int col = 0; col < width; col++) {
		
		unsigned char rowbits = thisChar->cols[col];
		
		for( int s = 0; s<strech; s++ )	{			// Strech
		
			int dotCol = x + xoffset; 
	
			if (dotCol >= 0 && dotCol < DISPLAY_COLS) {				// Check clipping rectangle
	
				for (int row = 0; row < 7 ; row++) {
	
					if ( rowbits & (1 << row)) {
	
						dots[row][dotCol] = 1;
	
					}
				}
			}
			
			xoffset++;
		}

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

int drawString( int x , const char *s , chartype *font ) {
	
	int strech =1;
	int padding = DEFAULT_CHAR_PADDING;
	
	int xoffset=0;
	
	while (*s) {
		
		if (*s == '*' ) {
        
			s++;
			
			switch (*s) {
                
                case '*': {                // Two *'s just means escape out a single *
					s++;
                    xoffset += draw5x7(x+xoffset, '*' , strech , font );
                }
                break;                    
				
				case 'S': 	{			// Set strech
					s++;
					if (*s && isdigit(*s)) {
						strech = *s - '0';
						s++;
					} else {
                       xoffset += drawString( x+xoffset , " [S without strech amount] " , font ); 	// Dont put a * in the error message or you get an infinate loop
                    }
					
				}  
				break;
				
				case 'T': {				// Insert time
				
					s++;
					
                    if (*s && isalpha(*s)) {						
					    xoffset += drawString( x+xoffset , timestring( *s ) , font );
					    xoffset += padding;
						s++;
                    } else {
                        xoffset += drawString( x+xoffset , " [T without format value] " , font );
                    }                        

					
				}
				break;
                
				case 'L': {				// Scroll lag
    				
					s++;
    				if (*s && isdigit(*s)) {
                        lag = *s - '0';
						s++;
                    } else {
                        xoffset += drawString( x+xoffset , " [L without lag digit] " , font );                        
    				}

    				
				}
				break;
                
				case 'P': {				// Interchar padding
    				s++;
    				if (*s && isdigit(*s)) {
        				padding = *s - '0';
						s++;
        			} else {
            			xoffset += drawString( x+xoffset , " [P without padding digit] " , font );
        			}

        				
    			}
    			break;
                
                

				case 'D': {				// Insert device name 
					s++;
					xoffset += drawString( x+xoffset , devArg , font );
					xoffset += padding;
					
				}
				break;


				case 'G': {				// Ping Google...
					s++;		
					const char *p = ping();		// Get the ping message
					xoffset += drawString( x+xoffset , p , font );
					xoffset += padding;
					
				}
				break;

				
			}		
			
		} else {
			

			xoffset+=draw5x7(x+xoffset, *s , strech , font );
	
			xoffset += padding;
			
			s++;
		}
	}
	
	return(xoffset);
		
}

// Draw the string repeasedly to the left until the display is full
// x is the col where you want to stop drawing. The end of the string will stop there.
// if x<=0 then nothing is drawn

void drawStringFillLeft( int x , int len, const char *s , chartype *font ) {
	
	if (len==0) return;		// Special case: we can never fill using a zero len string
	
	while (x>0) {
		
		x-=len;
		
		drawString( x , s , font );
				
	}
	
}



// A utility function to dump a font to a text file on stdout

void dumpFont( chartype *font) {
	
	
	printf(
			"# This is a blueled font file.\r\n"
			"# Lines that start with a hash are ignored. Blank lines not part of char data also ignored.\r\n"
			"# A char definition starts with CHAR= followed by the ascii code as a 2 digit hex number.\r\n"
			"# All chars are 7 pixels high and can be variable width.\r\n"
			"# The width of the first line of pixels estabishes the width of the char.\r\n"
			"# The following are all seen as on pixels: * X x @\r\n"
			"# Anything else besides a space is considered an off pixel\r\n"
			"# \r\n"
	);
			
	for( int b =0; b <256; b++ ) {
		
		if (font[b].cols != NULL) {				// defined char?

			if (isprint(b)) {
				printf("#%x hex = '%c'\r\n",b,b);
			} else {
				printf("#%x hex = (unprintable)\r\n",b);				
			}		
			printf("CHAR=%x\r\n",b);
			
			
			int left=0;
			int right=font[b].width-1;
			
			// Trim blank cols from the left
			// Exploits the fact that blank cols have no bits set so are arethmically 0
			// we do left < right rather then <= so that we are always left with at least 1 blank col 
			
			while (left<right && font[b].cols[left]==0 ) {
				left++;
			}
			
			// Trim blank cols from the left
			// Exploits the fact that blank cols have no bits set so are arethmically 0 
			
			while (left<right && font[b].cols[right]==0 ) {
				right--;
			}
			
			unsigned char *cols = font[b].cols;
			
			for(int r=0;r<7;r++) {
				
				for(int c=left; c<=right;c++) {
					
					int p = font[ b ].cols[c]  & (1<<r);
					
					if (p) {
						printf("X");
						
					} else {
						printf(".");					
					}				
				
				}
				printf("\r\n");					
			}
			
			printf("\r\n");
					
		}			
	}	
	
}

void printDebug( const char *s ) {
	
	
}

int main(int argc, char **argv)
{
	printf("BlueMan master controller Serial LED Driver\r\n");
	
	if (argc<2) {
		printf("no device file speficied.\r\n");
		
	}	
	
	fd = open(argv[1] , O_RDWR );
	
	if (fd == -1 ) {
			printf("failed to open serial device %s\r\n", argv[1]);
			return(1);
	}
	
	printDebug("Success opening serial device %s\r\n");
	devArg = argv[1];
	
	if (argc == 3) {
		
		if (argv[2][0]=='-' && argv[2][1]=='C') {	// Fill a single Col
			int c = atoi( argv[2] + 2 );
      
			clear();
		
			int start=c*10;
			int end=start+10;
			printf("Lighting col %d-%d...", start , end );
			
			for(int x=start; x< end;x++) {
					for(int r=0;r<ROWS;r++) {
							if (x>=0 && x<DISPLAY_COLS) {                    
								dots[r][x] = 1;
							}
					}
			}
						
			sendDots();
			printf("done.\r\n");
			exit(0);
					
		}
	

		if (argv[2][0]=='-' && argv[2][1]=='R') {	// Fill a single Col
			int r = atoi( argv[2] + 2 );
      
			clear();
		
			printf("Lighting row %d...", r );

			if (r>=0 && r<ROWS) {                    
				for(int c=0; c<DISPLAY_COLS;c++) {
					dots[r][c] = 1;
				}
			}
						
			sendDots();
			printf("done.\r\n");
			exit(0);
					
		}
			
	}
	
//	dumpFont();
//	exit(0);

	if (argc!=4) {
		printf( "Usage: blueled deviceName messagefile fontfile\n\r");
        printf(" Where: deviceName is the path to connected serial device (typically /dev/ttyXXXXX)\r\n");
		printf("        messagefile is the name of a file with the message to scroll\r\n");
        printf("        Reads a series of display strings from stdin and pumps them to the LED display\r\n");
		return(2);
	}
	
	printDebug("Reading font from %s....\r\n");

	chartype *font = importFont( argv[3] );
	
	if (!font) {
		printf( "Could not import font from file %s.\r\n", argv[3] );		
		exit(5);
	}

	/*	
	dumpFont( font );
	exit(0);
	*/
	

	char *prevMessage = (char *)malloc(1);		// Keep a copy of the previous message for smooth transitions
	prevMessage[0]=0x00;
	int prevWidth=0;
	
    size_t len = 0;

    purgeSerial();  // start fresh to avoid jerks
	
	
    while (1) {

		printDebug("Opening Message file...\r\n");

		
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
		
		printDebug("Mallocing memory....!\r\n");
		
				
		char *message = (char *)malloc( messageLen + 1 );			// Save room for null terminator
		
		
		if (!message) {
			printf("Malloc failed!\r\n");
			return(4);
		} 
		
		
		printDebug("Reading message...\r\n");
		
		fread(message, 1 , messageLen , f );
		message[messageLen]=0x00;						// Make null terminated string


		printDebug("Closing message file...\r\n");
		
		fclose(f);

		printDebug("Sizing width...\r\n");

		pingRun=0;		// Only run ping once per cycle
        lag=1;          // Default normal scroll speed

		// Dummy draw just to get the pixel width
		int width = drawString( 0 , message , font );
		
		
		// On the first frame of each pass, the 1st col of the message will be in the last col of the display
		// On the last frame of each pass, the last col of the message will be in the last col of the display 
		// This way there is a seamless transision from pass to pass
		// We also repeast the message to fill in the gap if the message is shorter than the display
		// We also keep the previous message to scroll that off and to have a seamless transision between messages.  
					
		// First we transision from the previous message to the new none
		// we always end with the last message col of the message on the last col of the display, so pick up from there using the previous message 
		
		
		printDebug("Scrolling splice!\r\n");
		
		for( int s= DISPLAY_COLS-1; (s > 0) && ( s>= DISPLAY_COLS-width)  ; s-- ) {		// Keep drawing until the prev message is off the display or until the new message is fully on the display
			clear();
			drawStringFillLeft( s , prevWidth , prevMessage , font );
			drawString( s , message , font );
			sendDots();
		}
		
		// Ok, now we are past the end of the previous message, now scroll out just the new message
		
		printDebug("Scrolling new message!\r\n");

		for(int s = 0 ; s >= (DISPLAY_COLS-width) ; s--) {		// Start of rightmost copy of message

			clear();
			drawString( s , message , font );
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
