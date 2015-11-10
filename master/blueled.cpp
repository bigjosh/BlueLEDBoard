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


#define COMMAND_NULL    0x00  // Do nothing. We can then send a bunch of these in a row to resync.  
#define COMMAND_DISPLAY 0xFF  // Put the last recieved data on the display
#define COMMAND_REBOOT  0xFE  // Reboot if the bytes in the buffer match the string "BOOT"


int fd;			// File desriptor for the serial port

unsigned char buffer[BUFFER_SIZE];

struct timeval lastSend = { 0,0 };


#define MINIMUM_SEND_DELAY_MS 200			// Current boards run at 80 FPS, so this gives a little extra room between updates


void purgeSerial() {
	int flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);			// Turn off blocking
	char dummy;
	while( read(fd, &dummy, 1) > 0 );					// Keep reading any pending bytes in the buffer until none left
	fcntl(fd, F_SETFL, flags );						// Set the flags back to what they were before we went non-blocking
		
}

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

	read(fd, &c, 1);
	
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


int draw5x7(int x, char c, int strech) {

	unsigned char *rowbits = &Font5x7[ (c - 0x20) * 5 ];
	
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

#define TIMESTRINGFORMAT1 "%A %B %d, %Y at %I:%M:%S %p %Z"
#define TIMESTRINGFORMAT2 "%A %B %d, %Y at %I %M %S %p %Z"

char timestringBuffer[TIMESTRINGLEN];

char *timestring() {
	
		time_t t;

		t = time(NULL);
		
		struct timeval start;	
		
		gettimeofday(&start, NULL);
		
		if ( start.tv_usec>500000UL) {
				
				strftime( timestringBuffer , TIMESTRINGLEN , TIMESTRINGFORMAT1 , localtime( &t ) );
				
		} else {
			
				strftime( timestringBuffer , TIMESTRINGLEN , TIMESTRINGFORMAT2 , localtime( &t ) );
			
		} 	
							
		return( timestringBuffer );
		
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

// remeber the device name passed on the command arg

const char *devArg;


// Returns length of result in pixels

#define DEFAULT_CHAR_PADDING 1

int drawString( int x , const char *s ) {
	
	int strech =1;
	int padding = DEFAULT_CHAR_PADDING;
	
	int xoffset=0;
	
	while (*s) {
		
		if (*s == '*' && *(s+1) != '*' ) {			// * = Command, except 2 *'s means just a *
		
			s++;
			
			switch (*s) {
				
				case 'S': 	{			// Set strech
					s++;
					if (1 ||isdigit(*s)) {
						strech = *s - '0';
						s++;
					}
					
				}  
				break;
				
				case 'T': {				// Insert time
				
					s++;
					xoffset += drawString( x+xoffset , timestring() );
					xoffset += padding;
					
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

int main(int argc, char **argv)
{
	printf("Blueman master controller Serial LED Driver\r\n");
	
	if (argc!=3) {
		printf( "Usage: master device arg1=path to connected serial device, arg2=message to send\r\n");
		return(2);
	}

	//f = fopen(argv[1], "w+b");

	fd = open(argv[1] , O_RDWR );
//	fd = open(argv[1], O_BINARY | _O_RDWR);

	if (fd == -1 ) {
		printf("failed to open serial device %s\r\n", argv[1]);
		return(1);
	}
	else {
		printf("Success opening serial device %s\r\n", argv[1]);
   
    devArg = argv[1];
	}

sleep(1000); 		// Let bootloader timeout

	const char*message = argv[2];

	while (1) {
		
		pingRun=0;		// Only run ping once per cycle

//		sprintf( message ,  MESSAGE  , argv[1] , timestring() );		// Wasted, just to get the length 

		int width = drawString( 0 , message );

		for(int s = COLS ; s > (COLS-width) ; s--) {		// Start of rightmost copy of message
	
//			sprintf( message ,  MESSAGE  , argv[1] , timestring() );		// Wasted, just to get the length 

			int x = s;
						
			clear();
			
			// Fill the screen starting from the right side intil we run off the left

			while (x > (-width) ) {		// Draw copies to fill  whole string

				x -= drawString( x , message );

			}

			sendDots();

//			sleep( 100 );

//			printf("Sent %s\r\n", argv[1]);

//			sleep(1000);

		}

	}


	while (1) {

		demo();


		clear();
		for (int i = 0; i < ROWS; i++) {


			dots[i][i*2] = 1;
		}
		sendDots();
		sleep(3000);

		for (int c = 0; c < COLS; c++) {

			for (int r = 0; r < ROWS; r++) {

				dots[r][c] = 1;

			}

			sendDots();

			//printf("c=%d\r\n", c);

			//_sleep(100);

		}


		for (int c = 0; c < COLS; c++) {

			for (int r = 0; r < ROWS; r++) {

				dots[r][c] = 0;

			}

			sendDots();

		}

		for (int r = 0; r < ROWS; r++) {

			for (int c = 0; c < COLS; c++) {

				dots[r][c] = 1;
			}

			sendDots();

			sleep(100);


		}

		for (int r = 0; r < ROWS; r++) {

			for (int c = 0; c < COLS; c++) {

				dots[r][c] = 0;
			}

			sendDots();

			sleep(100);


		}
	}

	return 0;
}

