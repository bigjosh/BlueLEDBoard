
// BitBand a single bit down the string at 10hz just for testing 

// COL DATA   - MOSI - PB3 - Chip pin 17 - Arduino Pin 11 - Board IC1 pin 32
// COL CLOCK  - SCK  - PB5 - Chip pin 19 - Arduino pin 13 - Board IC1 pin  7

// ROW SELECT - PB0-PB2 - Chip pins 14-16 - Arduino pins 8-10 - Board IC1 pins 1-3

// NOTE: ROW SELECT #3 is hardwired to ground on the daughter board
// NOTE: code assumes it is ok to directly assign row to full 8 bits of PB even though only bottom 3 bits are used

// DATAb connected directly to ground (data lines are ORed on the board) - IC1 pin 17

#define DATA_PIN 11
#define CLOCK_PIN 13

#define RS0_PIN 8
#define RS1_PIN 9
#define RS2_PIN 10

void setup() {

  pinMode( DATA_PIN , OUTPUT );
  pinMode( CLOCK_PIN , OUTPUT );
  pinMode( RS0_PIN , OUTPUT );
  pinMode( RS1_PIN , OUTPUT );
  pinMode( RS2_PIN , OUTPUT );

}

void setRow( int row ) {

    digitalWrite( RS0_PIN , row & (1<<0) );
    digitalWrite( RS1_PIN , row & (1<<1) );
    digitalWrite( RS2_PIN , row & (1<<2) );
    
}

#define DELAY 100000   //us


void loop() {

  int col = 0; 

  while (1) {

    if (col++==500) {        

      col=0; 

      digitalWrite( DATA_PIN , 1 );

    } else {

      digitalWrite( DATA_PIN , 0 );
      
    }

    setRow( 7 ) ;   // Clock line only connected on controller board when row #7 is selected
     
    digitalWrite( CLOCK_PIN, 1 ); 

    delayMicroseconds( DELAY );

    digitalWrite( CLOCK_PIN , 0 );

    for( int row =0; row<7; row++ ) {

      setRow( row );

      delayMicroseconds(DELAY);
    
    }
    
  }

}
