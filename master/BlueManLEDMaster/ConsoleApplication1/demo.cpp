#include "BlueManLEDMaster.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include <string.h>			// memset on linux



#ifdef WIN32

	#include <windows.h>

#endif


#define DELAY 400

#define SETDOT(c,r) (dots[r][c] = 1)
#define CLRDOT(c,r) (dots[r][c] = 0)

// Only set or clr if on screen
#define SETDOTP(c,r) if (c>=0 && c<COLS && r>=0 && r<ROWS) SETDOT(c,r)
#define CLRDOTP(c,r) if (c>=0 && c<COLS && r>=0 && r<ROWS) CLRDOT(c,r)

#define _BV(x) (1<<x)

#define random(x) (rand()%x)

void drawship(int x, unsigned char y, int j) {

	for (int q = -1; q <= 1; q++) {

		for (int p = -3 + abs(q); p <= 3 - abs(q); p++) {
			SETDOTP(x + p, y + q);
		}

	}

	// Blinking top light

	if (j & 0x10) {
		SETDOTP(x, y - 2);
	}

	// Make ship spin

	int shiplight = (j / 10 % 13);

	CLRDOTP(x + shiplight - 4, y);
}


#define DIGIT5WIDTH 5
#define DIGIT5HEIGHT 5

const unsigned char digits5[][DIGIT5WIDTH] = {

	{ 0x7c,0x4c,0x54,0x64,0x7c },
	{ 0x10,0x30,0x10,0x10,0x38 },
	{ 0x78,0x04,0x38,0x40,0x7c },
	{ 0x7c,0x04,0x38,0x04,0x7c },
	{ 0x40,0x40,0x50,0x7c,0x10 },
	{ 0x7c,0x40,0x78,0x04,0x78 },
	{ 0x7c,0x40,0x7c,0x44,0x7c },
	{ 0x7c,0x04,0x08,0x10,0x10 },
	{ 0x7c,0x44,0x7c,0x44,0x7c },
	{ 0x7c,0x44,0x7c,0x04,0x7c },

};


void drawdigit5(int center, int digit) {

	unsigned char w = DIGIT5WIDTH;

	int x = center - (w / 2);

	for (unsigned char c = 0; c < DIGIT5WIDTH; c++) {

		for (unsigned char r = 0; r < DIGIT5HEIGHT; r++) {

			if (digits5[digit][r] & _BV((DIGIT5WIDTH - c) + 1)) {
				SETDOTP(x, r + 2);
			}

		}

		x++;

	}

}
// IMages from...
/// http://cdn.instructables.com/FTR/8PSF/HCB8T24U/FTR8PSFHCB8T24U.LARGE.jpg


// All images are in top left -> bottomw right order. One row of data is once col of pixels


const unsigned char enemy1c[] = {
	0b00001110,
	0b00101111,
	0b01111111,
	0b01011011,
	0b00011011,
	0b00101111,
	0b00101111,
	0b00011011,
	0b01011011,
	0b01111111,
	0b00101111,
	0b00001110,
};

const unsigned char enemy1o[] = {
	0b01001110,
	0b01001111,
	0b00101111,
	0b00111011,
	0b00011011,
	0b00101111,
	0b00101111,
	0b00011011,
	0b00111011,
	0b00101111,
	0b01001111,
	0b01001110,
};



const unsigned char enemy2c[] = {
	0b00111000,
	0b00001100,
	0b00111110,
	0b01011011,
	0b01011110,
	0b00011110,
	0b01011110,
	0b01011011,
	0b00111110,
	0b00001100,
	0b00111000,
};

const unsigned char enemy2o[] = {
	0b00001111,
	0b01011100,
	0b00111110,
	0b00011011,
	0b00011110,
	0b00011110,
	0b00011110,
	0b00011011,
	0b00111110,
	0b01011100,
	0b00001111,
};

const unsigned char explode[] = {
	0b01001001,
	0b00101010,
	0b00001000,
	0b01000001,
	0b00100010,
	0b00000000,
	0b00000000,
	0b00100010,
	0b01000001,
	0b00001000,
	0b00101010,
	0b01001001,
};



const unsigned char rubble[] = {
	0b00010000,
	0b01000000,
	0b01100100,
	0b01100000,
	0b01110000,
	0b01100110,
	0b01111000,
	0b01111010,
	0b01100000,
	0b01111010,
	0b01001000,
	0b00110000,
	0b00110000,
	0b01000000,
	0b00000000,
	0b01000000,
};

const unsigned char graveo[] = {
	0b00000000,
	0b01000000,
	0b01100000,
	0b01100000,
	0b01100000,
	0b01100000,
	0b01100000,
	0b01100000,
	0b01100000,
	0b01100000,
	0b01100000,
	0b01000000,
	0b00000000,
	0b00000000,
	0b00000000,
	0b00000000,
};

const unsigned char gravec[] = {
	0b00000000,
	0b01000000,
	0b01100000,
	0b01100000,
	0b01100000,
	0b01100000,
	0b01100000,
	0b01100000,
	0b01100000,
	0b01100000,
	0b01100000,
	0b01000000,
	0b00001000,
	0b01111100,
	0b00001000,
	0b00000000,
};


#define ALIEN_WIDTH 12


// x is left side, always drawn full height

void drawmap(unsigned int center, const unsigned char *map, unsigned char w) {



	int x = center - (w / 2);

	int i = 0;

	while (i < w) {
		for (int r = 0; r < ROWS; r++) {
			if (map[i] & (1 << r)) {
				SETDOTP(x, r);
			}
		}

		x++;
		i++;
	}

}




#define STARS 10

// all fixed point with Assumed 1 decimals
int starsX[STARS];
int starsY[STARS];
int starsDX[STARS];
int starsDY[STARS];


void drawalien(unsigned int x, unsigned char i, unsigned char open) {

	if (i == 0) {  // alien 1

		if (open) {   // open

			drawmap(x, enemy1o, sizeof(enemy1o));

		}
		else {    // close

			drawmap(x, enemy1c, sizeof(enemy1c));

		}

	}
	else if (i == 1) {    // enemyt 2

		if (open) {   // open

			drawmap(x, enemy2o, sizeof(enemy2o));

		}
		else {    // close

			drawmap(x, enemy2c, sizeof(enemy2c));

		}

	}
	else if (i == 2) {   // elplosion

		drawmap(x, explode, sizeof(explode));

	}
	else if (i == 3) {            // rubble

		drawmap(x, rubble, sizeof(rubble));

	}
	else if (i == 4) {            // rubble

		if (open) {   // open

			drawmap(x, graveo, sizeof(graveo));

		}
		else {    // close

			drawmap(x, gravec, sizeof(gravec));

		}

	}


}


#define ALIENCOUNT 6

void demo() {

	printf("marching\r\n");

	const int alien_spacing = 2;    // Gap between adjecent aleins
	const int alien_train_width = (ALIEN_WIDTH * ALIENCOUNT) + (alien_spacing * (ALIENCOUNT - 1));

	for (int s = (-1 * alien_train_width) - (ALIEN_WIDTH / 2); s < COLS + (ALIEN_WIDTH / 2); s++) {      // s=center of leftmost alien 

		clear();

		for (int a = 0; a < ALIENCOUNT; a++) {

			drawalien(s + (a * 13), a & 1, (s + 8) & 16);

		}

		sendDots();		

	}


	// Now flying forward...

	printf("flying\r\n");

	clear();
	sendDots();

	sleep(DELAY);

	// init stars
	for (int s = 0; s < STARS; s++) {

		// Center with random forward velocity
		starsX[s] = random(COLS * 10);
		starsY[s] = random(ROWS * 10);
		starsDX[s] = (random(9) + 1) * -1;

		//printf("Star %d , %d, %d\r\n", starsX[s], starsY[s], starsDX[s]);
	}


	int shipX = (-40) * 10;      // Start ship to the left off the screen 40 cols

	while (shipX < COLS * 10) {

		clear();

		drawship(shipX / 10, ROWS / 2, shipX / 3);

		for (int s = 0; s < STARS; s++) {

			//        unsigned int oldX = starsX[s];
			//      unsigned int oldY = starsY[s];

			starsX[s] += starsDX[s];

			// Off display hoazontally?
			if (starsX[s] < 0) {
				starsX[s] = (COLS - 1) * 10;
				starsY[s] = random(ROWS * 10);
				starsDX[s] = (random(9) + 1) * -1;
			}

			SETDOT(starsX[s] / 10, starsY[s] / 10);

			//printf("%2.2d %5.5d,%5.5d,%5.5d ", s , starsX[s], starsY[s], starsDX[s]);

		}

		//printf("\r\n");

		sendDots();
		//delay(1);
		shipX += 10;

	}





	clear();
	sendDots();

	sleep(DELAY);


	// Draw ruler with ticks every 5 pixels and numbers every 50
	printf("ruler\r\n");

	clear();

	for (int s = 1; s < COLS; s++) {

		if (s % 5 == 0) {

			SETDOTP(s, 0);


			if (s % 10 == 0) {

				SETDOTP(s, 1);

				if (s % 50 == 0) {

					SETDOTP(s, 2);

					drawdigit5(s - (DIGIT5WIDTH / 2) - 2, s / 100);
					drawdigit5(s + (DIGIT5WIDTH / 2) + 2, s / 10 % 10);

				}


			}

		}
		sendDots();
		sleep(10);
	}

	sleep(1000);


}
