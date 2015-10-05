#pragma once


#define ROWS 7


#define CHARS 84                      // Number of char modules
#define CHAR_WIDTH 5                  // With of each module in LEDs

#define COLS  (CHARS*CHAR_WIDTH)

#define ROUNDUPN_TO_NEARESTM( n , m ) ( ( ( n + (m-1) ) / m ) * m)     // Round number to nearest multipule

#define PADDED_COLS ROUNDUPN_TO_NEARESTM( COLS, 8)    // Add padding at the end past the right edge of the display so we have full 8-bit bytes to pass down to SPI

#define BUFFER_SIZE (PADDED_COLS/8)*ROWS


extern unsigned char dots[ROWS][PADDED_COLS];

void sendDots();

void demo();

void clear();