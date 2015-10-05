// ConsoleApplication1.cpp : Defines the entry point for the console application.
//


#include <conio.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>



#include "BlueManLEDMaster.h"




unsigned char dots[ROWS][PADDED_COLS];


#define COMMAND_NULL    0x00  // Do nothing. We can then send a bunch of these in a row to resync.  
#define COMMAND_DISPLAY 0xFF  // Put the last recieved data on the display
#define COMMAND_REBOOT  0xFE  // Reboot if the bytes in the buffer match the string "BOOT"


FILE *f;


void sendDots() {

	int dotCount = 0;

	for (int r= 0; r < ROWS; r++ ) {

		int bit = 0;

		int out = 0;

		for (int c = 0; c < PADDED_COLS; c++ ) {
		
			if (dots[r][c]) {

				out |= 1 << bit;

			}

			bit++;

			if (bit == 8) {

				fputc(out, f);

				dotCount++;

				out = 0;
				bit = 0;
			}

		}

	}

	fflush(f);

	sleep(100);

	//printf("padded=%d, rows=%d, buffersize=%d, dotCount=%d\r\n", PADDED_COLS, ROWS , BUFFER_SIZE ,  dotCount);

}


void clear() {

	memset(dots, 0x00, ROWS*PADDED_COLS);

}

int main()
{



	printf("BlueManBoard Serial Test\r\n");

	f = fopen("\\\\.\\COM6", "w+b");

	if (f == NULL) {
		printf("failed\r\n");
		return(1);
	}
	else {
		printf("Success!\r\n");
	}

	demo();
	while (1) {

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

