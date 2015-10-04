// ConsoleApplication1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

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

	for(int r = 0; r < ROWS; r++) {

		int bit = 0;

		int out = 0;

		for (int c = 0; c < PADDED_COLS; c++) {

			if (dots[r][c]) {

				out |= 1 << bit;

			}

			bit++;

			if (bit >= 8) {

				fputc(out, f);
				dotCount++;

				out = 0;
				bit = 0;
			}

		}

	}

	fflush(f);

	_sleep(1);

	printf("padded=%d, rows=%d, buffersize=%d, dotCount=%d\r\n", PADDED_COLS, ROWS , BUFFER_SIZE ,  dotCount);

}


void clear() {

	memset(dots, 0x00, ROWS*PADDED_COLS);


}

int main()
{



	printf("BlueManBoard Serial Test\r\n");

	f = fopen("\\\\.\\COM12", "w+b");

	if (f == NULL) {
		printf("failed\r\n");
		return(1);
	}
	else {
		printf("Success!\r\n");
	}

	//demo();
	while (1) {

		clear();
		for (int r = 0; r < ROWS; r++) {

			for (int c = 0; c < PADDED_COLS; c++) {

				if ((r + c) & 1) {

					dots[r][c] = 1;

				}

			}

		}
		printf("hatch.");
		sendDots();
		_sleep(1000);

		clear();
		for (int r = 0; r < ROWS; r++) {

			for (int c = 0; c < PADDED_COLS; c++) {

				if ((c) & 1) {

					dots[r][c] = 1;

				}

			}

		}
		printf("stipe.");
		sendDots();
		_sleep(1000);




		clear();
		sendDots();
		_sleep(1000);

	}
	




	//demo();
	return(0);

	/*
	// Resycn reciever
	for (int i = 0; i < 255; i++) {

		fputc(0x00, f);

	}

	printf("synced.");


	fputc(7, f);

	for (int i = 0; i < 7; i++) {

		fputc(i, f);
	}

	//return(0);
	fputc(255, f);		// Sending 7 bytes
	*/
	while (1) {

		int i = 0;

		while (i < 128) {

			for (int l = 0; l < 84 * 5; l++) {

				fputc(i, f);

			}

			i++;

			fputc(0xff, f);			// Actual splat onto the display

			fflush(f);

			_sleep(10);

			printf("x");


		}


	}

	return 0;
}

