// Drive BlueMan LED controller board


// COL DATA   - MOSI - PB3 - Chip pin 17 - Arduino Pin 11 - Board IC1 pin 32
// COL CLOCK  - SCK  - PB5 - Chip pin 19 - Arduino pin 13 - Board IC1 pin  7

// ROW SELECT - PB0-PB2 - Chip pins 14-16 - Arduino pins 8-10 - Board IC1 pins 1-3
// ROW SELECT HI (dormant, always 0)- PB4 - Chip pin PB4 - Arduino pin 12 - Board IC1 pin 3

// NOTE: code assumes it is ok to directly assign row to full 8 bits of PB even though only bottom 3 bits are used

// DATAb connected directly to ground (data lines are ORed on the board) - IC1 pin 17

#include <TimerOne.h>
#include <avr/power.h>        

#define FRAME_RATE 80

#define ROWS 7

#define CHARS 84                      // Number of char modules
#define CHAR_WIDTH 5                  // With of each module in LEDs

#define COLS  (CHARS*CHAR_WIDTH)

#define ROUNDUPN_TO_NEARESTM( n , m ) ( ( ( n + (m-1) ) / m ) * m)     // Round number to nearest multipule

#define PADDED_COLS ROUNDUPN_TO_NEARESTM( COLS, 8)    // Add padding at the end past the right edge of the display so we have full 8-bit bytes to pass down to SPI
          
uint8_t dots[PADDED_COLS];   // Packed bits
						// dot[0], bit 0 = upper leftmost LED
						// dot[COLS-1], bit (ROWS-1) = lower rightmost LED

            // Note that technically we do not need to allocate the extra space at the end for the padding since
            // whatever data ends up here will get sent to the dispaly, but will fall off the end and never actually end up on the LEDs.             

volatile uint8_t isr_row = 0;  // Row to display on next update

volatile uint8_t sync = 0;  // set to 0 after refresh so you can sync to it (vertical retrace sync)

#define SETROWBITS(ROW)	(PORTB = ROW)         // Set the seelct bits that drive a row of LEDs

void setupRowDDRbits() {

	DDRB |= 0b00010111;			// row bits

}

#define DDR_SPI DDRB
#define PORT_SPI PORTB
#define DD_MOSI 3     // PB3 = Pin 17 = Digital 11
#define DD_SCK  5     // PB5 = Pin 19 = Digital 13
#define DD_SS   2     // PB2 = Pin 16 = Digital 10

// "19.3.2: If SS is configured as an output, the pin is a general output pin which does not affect the SPI system." 
// So we must keep SS output, but can be generally used.

void SPI_MasterInit(void)
{
    /* Set MOSI,SCK,SS output */
    DDR_SPI |= (1<<DD_MOSI)|(1<<DD_SCK) | (1<<DD_SS);    

    SPSR |= _BV(SPI2X);         // Double time! Clock now at 8Mhz
  
    /* Enable SPI, Master, set clock rate 4Mhz */
    SPCR = (1<<SPE)|(1<<MSTR);

    SPDR = 0;     // Send a dummy byte to prime the pump so that the interrupt bit will be set when we go to send the first real byte. 
    
    //TODO: Probably change mode so setup is on falling, DORDER, 
}

inline void SPI_MasterTransmit(char cData)
{
  
  /* Wait for any previous transmission to complete */
  /* Start transmission */

  // TODO: Why don't these work? SPI hardware bug?

  //while(!(SPSR & (1<<SPIF)));  
  //do {
    SPDR = cData;
  //} while (SPSR & _BV(WCOL));
  while(!(SPSR & (1<<SPIF)));  
  
}


// Preset all bits into buffer so we can squirt them out fast

#define SPI_BUFFER_LEN ( PADDED_COLS/8 )     // SPI sends full bytes. We already padded PADDED_COLS up to the next 8-bit boundary above. 

#if (SPI_BUFFER_LEN>0xff)      // So we can use bytes for index variables. 8*256 should be big enough 

  #error display too long

#endif

uint8_t spiBuffer[ SPI_BUFFER_LEN ];

// Called from timer interrupt to refresh the next row of the LED display
void refreshRow()
{
 //   PINB|=0xff;


  // First load up the bitBuffer

	uint8_t row_mask = 1 << isr_row;    // For quick bit testing

  uint8_t spiBufferPtr = SPI_BUFFER_LEN;    // Fill in the bit buffer one byte at a time
  
	uint16_t c = PADDED_COLS;      //...by looking up bitmask in the dots array (Used PADDED_COLS so we capture the 8-aligned padding at the end automatically

  while (spiBufferPtr) {


    uint8_t t=0;      // Build output byte here

    uint8_t bit=8;

    while (bit) {

      bit--;

      t <<=1;                         // Shift previous value up one bit
      
      if (dots[--c] & row_mask) {      // Current row,col set? (pre-decrement indirect addressing faster in AVR) 
        t |=0b00000001;
      }

    }

    spiBuffer[--spiBufferPtr] = t;    // (pre-decrement indirect addressing faster in AVR) 

  }
 

  // Ok, bits from the current row in dots[] are now packed into bytes in spiBuffer[]. 1st byte is still leftmost on display. Rightmost bytes are sacraficial to give us a nice byte boundary

  spiBufferPtr = SPI_BUFFER_LEN;    
  
  // Everything is staged and ready to quickly squirt out the bytes over SPI
  // Once the set the row bits, the LEDs are off so the race is on! The longer the LEDs are off, the dimmer the display will look.

	SETROWBITS(0x07);		// Select ROW 8- which is actually a dummy row for selecting the data bit  
							  // Would be nice to leave the LEDs on while shifting out the cols but
							  // unfortunately then we'd display arifacts durring the shift

  

  while (spiBufferPtr) {

   //   PORTC |= _BV(5);
   //   asm("nop;nop;nop;nop;");
   //   PORTC &= ~_BV(5);

      SPI_MasterTransmit( spiBuffer[--spiBufferPtr] );      // (pre-decrement indirect addressing faster in AVR)

      __builtin_avr_delay_cycles(7);
      
  }
	
	// turn on the row... (also sets clock and data low)

	SETROWBITS(isr_row);

	// get ready for next time...

  isr_row++;

	if (isr_row >= ROWS) {

		isr_row = 0;
    sync=0;                 // Signal vertical retrace so forground knows when to start drawing to avoid tearing on the display

	}


}

void setupTimer() {


	Timer1.initialize(1000000UL / (FRAME_RATE*ROWS));			// uSeconds between row refreshes
	Timer1.attachInterrupt(refreshRow); 

}


#define COMMAND_NULL    0x00  // Do nothing. We can then send a bunch of these in a row to resync.  
#define COMMAND_DISPLAY 0xFF  // Put the last recieved data on the display
#define COMMAND_REBOOT  0xFE  // Reboot if the bytes in the buffer match the string "BOOT"

// Otherwise, the 1st byte of a message (0x01-FD) is the count of the number of bytes to follow

// Read an process any serial byte that might have come in since last time we checked. 

void readSerialByte() {

  uint8_t ss_read_bytes;      // Number of bytes left to read into the pixel buffer. Only valid durring SS_DATA state. 

  if (Serial.available()) {

    char c = Serial.read();

    if (ss_read_bytes) {    // Currently reading in a message?

      // TODO: pack 7 bits into 8bit bytes

      dots[ss_read_bytes--] = c;
      
    } else {

      if (c==COMMAND_DISPLAY) {

        // TODO: Make display double buffered
        
      } else if (c==COMMAND_REBOOT) {

        if (dots[0]=='B' && dots[1]=='O' && dots[2]== 'O' && dots[3] =='T') {   // Safety interlock

          // TODO: make this work. maybe jump to the reset vector?
          /*
          wdt_enable(WDTO_15MS);  
          while (1);              // Wait for the inveitable!
          */
          
        }


        
      }


      
    }

        

    
  }
}



#define DELAY 400

#define SYNC(); {sync=1;while(sync);}

#define SETDOT(c,r) dots[c] |= _BV(r)
#define CLRDOT(c,r) dots[c] &= ~_BV(r)

// Only set or clr if on screen
#define SETDOTP(c,r) if (c>=0 && c<COLS && r>=0 && r<ROWS) SETDOT(c,r)
#define CLRDOTP(c,r) if (c>=0 && c<COLS && r>=0 && r<ROWS) CLRDOT(c,r)


void drawship(int x, uint8_t y, int j) {

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

void undrawship(int x, uint8_t y) {

	for (int q = -1; q <= 1; q++) {

		for (int p = -3 + abs(q); p <= 3 - abs(q); p++) {
			CLRDOTP(x + p, y + q);
		}

	}

	// Blinking top light

	CLRDOTP(x, y - 2);

}

#define DIGIT5WIDTH 5
#define DIGIT5HEIGHT 5

const uint8_t digits5[][DIGIT5WIDTH] = {

 {0x7c,0x4c,0x54,0x64,0x7c},
 {0x10,0x30,0x10,0x10,0x38},
 {0x78,0x04,0x38,0x40,0x7c},
 {0x7c,0x04,0x38,0x04,0x7c},
 {0x40,0x40,0x50,0x7c,0x10},
 {0x7c,0x40,0x78,0x04,0x78},
 {0x7c,0x40,0x7c,0x44,0x7c},
 {0x7c,0x04,0x08,0x10,0x10},
 {0x7c,0x44,0x7c,0x44,0x7c},
 {0x7c,0x44,0x7c,0x04,0x7c},

};


void drawdigit5(int center, int digit) {

	uint8_t w = DIGIT5WIDTH;

	int x = center - (w / 2);

	for (uint8_t c = 0; c < DIGIT5WIDTH; c++) {

		for (uint8_t r = 0; r < DIGIT5HEIGHT; r++) {

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


const uint8_t enemy1c[] = {
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

const uint8_t enemy1o[] = {
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



const uint8_t enemy2c[] = {
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

const uint8_t enemy2o[] = {
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

const uint8_t explode[] = {
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



const uint8_t rubble[] = {
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

const uint8_t graveo[] = {
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

const uint8_t gravec[] = {
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

void drawmap(uint16_t center, const uint8_t *map, uint8_t w) {

	int x = center - (w / 2);
	// Left half
	int i = 0;

	while (i < w) {
		if (x >= 0 && x < COLS) dots[x] |= map[i];
		x++;
		i++;
	}

}


void undrawmap(uint16_t center, const uint8_t *map, uint8_t  w) {

	int x = center - (w / 2);
	// Left half
	int i = 0;
	while (i < w) {
		if (x >= 0 && x < COLS) dots[x] &= ~map[i];
		x++;
		i++;
	}

}


// Clear the display

void clear() {
  uint16_t c=COLS;
  while (c) dots[--c] = 0x00;  // Clear screen  - pre decrement inditect addressing 
}


#define STARS 10

// all fixed point with Assumed 1 decimals
int starsX[STARS];
int starsY[STARS];
int starsDX[STARS];
int starsDY[STARS];


void drawalien(uint16_t x, uint8_t i, uint8_t open) {

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


void undrawalien(uint16_t x, uint8_t i, uint8_t open) {

	if (i == 0) {  // alien 1

		if (open) {   // open

			undrawmap(x, enemy1o, sizeof(enemy1o));

		}
		else {    // close

			undrawmap(x, enemy1c, sizeof(enemy1c));

		}

	}
	else if (i == 1) {    // enemy 2

		if (open) {   // open

			undrawmap(x, enemy2o, sizeof(enemy2o));


		}
		else {    // close

			undrawmap(x, enemy2c, sizeof(enemy2c));

		}
	}

}


#define ALIENCOUNT 6

void demoloop() {

	/*
	// Testing code - flips dtata and clock as fast as possible.
	// Use to check board compatibility (Trinket is NOT compatible becuase of pull-ups)

	  TIMSK |= _BV(OCIE0A);  // Turn off timer int

	  while(1) {
		PORTB|=_BV(3);
		PORTB&=~_BV(4);
		PORTB&=~_BV(3);
		PORTB|=_BV(4);
	  }

	 */
	/*
	//TIMSK0 |= _BV(OCIE0A);  // Turn off timer int

	while (1) {

		PORTD = _BV(4);
		delay(100);
		PORTD &= ~_BV(4);
		delay(100);
	}
	return;

	*/

  // Draw ruler with ticks every 5 pixels and numbers every 50

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

		delay(10);
	}

 delay(1000);

  const int alien_spacing = 2;    // Gap between adjecent aleins
  const int alien_train_width = (ALIEN_WIDTH * ALIENCOUNT) + (alien_spacing * (ALIENCOUNT-1));

	for (int s = ( -1 * alien_train_width ) - (ALIEN_WIDTH/2) ; s < COLS + (ALIEN_WIDTH/2) ; s++ ) {      // s=center of leftmost alien 

    SYNC();
		clear();

		for (int a = 0; a < ALIENCOUNT; a++) {       

			drawalien(s + (a * 13), a & 1, (s + 8) & 16);

		}

    delay(40);

	}


	// Now flying forward...

	clear();
	delay(DELAY);

	// init stars
	for (int s = 0; s < STARS; s++) {

		// Center with random forward velocity
		starsX[s] = random(COLS * 10);
		starsY[s] = random(ROWS * 10);
		starsDX[s] = (random(9) + 1) * -1;
	}


	int shipX = (-40) * 10;      // Start ship to the left off the screen 40 cols

	while (shipX < COLS * 10) {

    SYNC();
		clear();

		drawship(shipX / 10, ROWS / 2, shipX / 3);

		for (int s = 0; s < STARS; s++) {

			//        uint16_t oldX = starsX[s];
			  //      uint16_t oldY = starsY[s];

			starsX[s] += starsDX[s];

			// Off display hoazontally?
			if (starsX[s] < 0) {
				starsX[s] = (COLS - 1) * 10;
				starsY[s] = random(ROWS * 10);
				starsDX[s] = (random(9) + 1) * -1;
			}

			SETDOT(starsX[s] / 10, starsY[s] / 10);
		}

		delay(1);
    shipX += 1;
   
	}




	int l = 0;

	for (int i = 0; i < COLS; i++) {

		dots[i] = 0b10101010;

	}

	delay(DELAY);

	for (int i = 0; i < COLS; i++) {

		dots[i] = 0b01010101;

	}

	delay(DELAY);

	for (int i = 0; i < COLS; i++) {

		dots[i] = 0xff;

	}

	delay(DELAY);

	for (int i = 0; i < COLS; i++) {

		dots[i] = 0;

	}
	delay(DELAY);


	for (int i = 0; i < COLS; i++) {

		if (i & 1) {
			dots[i] = 0xff;
		}
		else {
			dots[i] = 0x00;
		}

	}
	delay(DELAY);


	for (int i = 0; i < COLS; i++) {

		if (i & 1) {
			dots[i] = 0x00;
		}
		else {
			dots[i] = 0xff;
		}

	}
	delay(DELAY);

	for (int i = 0; i < COLS; i++) {

		dots[i] = 0xff;

	}

	delay(DELAY);

	for (int i = 0; i < COLS; i++) {

		dots[i] = 0;

	}
	delay(DELAY);


	// Hello world cross hatch pattern  

	for (int i = 0; i < COLS; i++) {

		dots[i] = 0b10101010;

	}

	delay(DELAY);

	for (int i = 0; i < COLS; i++) {

		dots[i] = 0b01010101;

	}

	delay(DELAY);


	for (int i = 0; i < COLS; i++) {

		if (i & 1) {
			dots[i] = 0b01010101;
		}
		else {
			dots[i] = 0b10101010;

		}

	}
	delay(DELAY);


	for (int i = 0; i < COLS; i++) {

		if (i & 1) {
			dots[i] = 0b10101010;
		}
		else {
			dots[i] = 0b01010101;
		}

	}
	delay(DELAY);

	for (int i = 0; i < COLS; i++) {

		dots[i] = 0xff;

	}

	delay(DELAY);

	for (int i = 0; i < COLS; i++) {

		dots[i] = 0;

	}
	delay(DELAY);


	for (int i = 0; i < COLS; i++) {

		dots[i] = 0xff;
		delay(6);

	}
	delay(DELAY);

	for (int i = 0; i < COLS; i++) {

		dots[i] = 0x00;
		delay(6);

	}
	delay(DELAY);

	for (int i = COLS-1; i >= 0; i--) {

		dots[i] = 0xff;
		delay(6);

	}
 
	delay(DELAY);

	for (int i = COLS-1; i >= 0; i--) {

		dots[i] = 0x00;
		delay(6);

	}
	delay(DELAY);


	for (int r = 0; r < ROWS; r++) {

		uint8_t m = (1 << (r + 1)) - 1;

    SYNC();

		for (int c = 0; c < COLS; c++) {
			dots[c] = m;
		}

		delay(100);

	}
	//  delay(DELAY);  


	for (int r = 0; r < ROWS; r++) {

		uint8_t m = ~((1 << (r + 1)) - 1);

    SYNC();

		for (int c = 0; c < COLS; c++) {
			dots[c] = m;
		}

		delay(100);

	}

	for (int r = 0; r < ROWS; r++) {

		uint8_t m = ~((1 << (ROWS - r)) - 1);

    SYNC();

		for (int c = 0; c < COLS; c++) {
			dots[c] = m;
		}

		delay(100);

	}
	//  delay(DELAY);  


	//  delay(DELAY);  

	for (int r = 0; r < ROWS; r++) {

		uint8_t m = (1 << ((ROWS - 1) - r)) - 1;

    SYNC();

		for (int c = 0; c < COLS; c++) {
			dots[c] = m;
		}

		delay(100);

	}
	//  delay(DELAY);  

	randomSeed(100);

	for (int l = 0; l < 2000; l++) {

		int r = random(COLS *ROWS);
		int col = r / ROWS;
		int row = r%ROWS;

		dots[col] |= 1 << row;

    delay(1);

	}

	randomSeed(100);

	for (int l = 0; l < 2000; l++) {

		int r = random(COLS *ROWS);
		int col = r / ROWS;
		int row = r%ROWS;

		dots[col] &= ~(1 << row);

		SYNC();
   delay(1);

	}


	clear();
	delay(DELAY);

}

void serialInit() {       // Initialize serial port to Recieve display data

  Serial.begin(115000);

}



void setup() {


  
  // put your setup code here, to run once:

  //if (F_CPU == 16000000) clock_prescale_set(clock_div_1);  // For trinket - switch to full speed 16mhz prescaler
  // https://learn.adafruit.com/introducing-trinket/16mhz-vs-8mhz-clock

  setupRowDDRbits();

  DDRC |= _BV(5);
  
//  PORTC |= _BV(5);
//  PORTC &= ~_BV(5);

  //delay(100);

  setupTimer();
  SPI_MasterInit();

  serialInit();
   
}



void loop() {

  
/*
  PORTC |= _BV(5);
  asm("nop");
  PORTC &= ~_BV(5);

  */
/*
  for(int i=0; i<ROWS; i++) {
    SETDOT( i,i);
  }

  while(1);
*/
  demoloop();

  while (1) {

    while (!Serial.available());
      
    char c = Serial.read();
  
    dots[0] = 0xff;
    dots[1] = c;

  }
}


