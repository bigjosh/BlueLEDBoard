// ConsoleApplication1.cpp : Defines the entry point for the console application.
//



#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>			// memset on linux
//#include <sys/time.h>		


#include "BlueManLEDMaster.h"
#include "font5x7.h"


unsigned char dots[ROWS][PADDED_COLS];


#define COMMAND_NULL    0x00  // Do nothing. We can then send a bunch of these in a row to resync.  
#define COMMAND_DISPLAY 0xFF  // Put the last recieved data on the display
#define COMMAND_REBOOT  0xFE  // Reboot if the bytes in the buffer match the string "BOOT"


FILE *f;

unsigned char buffer[BUFFER_SIZE];

struct timeval lastSend = { 0,0 };


#define MINIMUM_SEND_DELAY_MS 200			// Current boards run at 80 FPS, so this gives a little extra room between updates


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

	fwrite(  buffer , sizeof(*buffer) , BUFFER_SIZE , f );

	fflush(f);
/*
	struct timeval t;
	unsigned long elapsedTime;

	gettimeofday(&t2, NULL);

	// compute and print the elapsed time in millisec
	elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000;      // sec to ms
	elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms

*/
	sleep(10);

	//printf("padded=%d, rows=%d, buffersize=%d, dotCount=%d\r\n", PADDED_COLS, ROWS , BUFFER_SIZE ,  dotCount);

}


void clear() {

	memset(dots, 0x00, ROWS*PADDED_COLS);

}

#define CHAR_WIDTH 5
#define CHAR_PADDING 1

int stringWidth(const char *s) {

	return(strlen(s) * (CHAR_WIDTH + CHAR_PADDING));

}

void draw5x7(int x, char c) {

	unsigned char *rowbits = &Font5x7[ (c - 0x20) * 5 ];

	for (int col = 0; col < 5; col++) {

		int dotCol = x + col; 

		if (dotCol >= 0 && dotCol < COLS) {				// Check clipping rectangle

			for (int row = 0; row < 7 ; row++) {

				if (*rowbits & (1 << row)) {

					dots[row][dotCol] = 1;

				}
			}
		}

		rowbits++;

	}

}

void draw5x7String(int x, const char *s) {

	while (*s) {

		draw5x7(x, *s);

		x += CHAR_WIDTH + CHAR_PADDING;

		s++;
	}

}

int main(int argc, char **argv)
{



	printf("BlueManBoard Serial Test\r\n");

	f = fopen(argv[1], "w+b");

	if (f == NULL) {
		printf("failed to open serial device %s\r\n", argv[1]);
		return(1);
	}
	else {
		printf("Success!\r\n");
	}


	const char *message = "This is a very long text string!";
	int width = stringWidth(message);

	while (1) {

		for(int x = COLS; x > -width ; x--) {
//		for (int x = 0; x < COLS; x++ ) {

			clear();
			draw5x7String(x, message);
			sendDots();

			fgetc(f);			// Pause for vertical retrace


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

