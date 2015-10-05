// Drive BlueMan LED controller board


// COL DATA   - MOSI - PB3 - Chip pin 17 - Arduino Pin 11 - Board IC1 pin 32
// COL CLOCK  - SCK  - PB5 - Chip pin 19 - Arduino pin 13 - Board IC1 pin  7

// ROW SELECT - PB0-PB2 - Chip pins 14-16 - Arduino pins 8-10 - Board IC1 pins 1-3

// NOTE: ROW SELECT #3 is hardwired to ground on the daughter board
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

#define ROW_BYTES (PADDED_COLS/8)       // Number of bytes in a row in the buffer

#define BUFFER_SIZE ( ROW_BYTES * ROWS )        // Size of a full screen buffer
          
uint8_t spiBuffer[ BUFFER_SIZE ];     

            // These bits are actually being displayed on the LEDs by the background refresh thread
            // Theses are packed a row at a time so the SPI can cycle though them quickly

volatile uint8_t sync = 0;  // used to syncronize frame buffer updates to avoid tearing. set to 0 after display refresh so you can sync to it (vertical retrace sync)

#define SYNC(); {sync=1;while(sync);}            // Block until diplay refresh complete. 

// Otherwise, the 1st byte of a message (0x01-FD) is the count of the number of bytes to follow

uint8_t readBuffer[BUFFER_SIZE];     // This buffer is reading in new bits from the Serial. It gets copied to the display buffer when we recieve a COMMAND_DISPLAY

uint16_t readBufferHead = 0;         // Where most recently recieved byte was written n the circular buffer

volatile uint8_t packetTimeout=0;     // If we dont' get a serial byte for a while, we reset to the begining of a packet
                                     // this is driven by the the refresh interval. 

// Read recieved serial byte and put into buffer


SIGNAL(USART_RX_vect) {

  while ( UCSR0A & _BV( RXC0 ) ) {    // While chars are available...

    PORTC |= _BV(4);

    unsigned char c = UDR0;
    
    if (!packetTimeout) {


            readBufferHead=0;
                  
    }


    readBuffer[readBufferHead++] = c;

    //UDR0= (  readBufferHead % 10 ) + '0';

    packetTimeout = 4;

    if ( readBufferHead >= BUFFER_SIZE) {

      PORTC |= _BV(5);

      // TODO: Double or tripple buffer this so that we just update a flag to tell the refreh thread to use the new buffer


      //SYNC();                                           // Wait for vertical refresh interval to avoid tearing from copying while display is updating

      memcpy( spiBuffer , readBuffer , BUFFER_SIZE );   // COpy the entire buffer even though it might not be full just to keep timing consistant

      readBufferHead=0;

      
    }

    PORTC =0;   

  }
  
}


void serialInit() {       // Initialize serial port to Recieve display data
  
//  uint16_t ubrr = F_CPU/16/BAUD-1;


  // TODO: Get faster baud rate working. Probably need zoer copy double and to enable ints durring SPI output

  /*Set baud rate 100K*/
  //UBRR0L = (unsigned char)9;
  //UBRR0H = (unsigned char)0;// 


  /*Set baud rate 1M*/
  UBRR0L = (unsigned char)0;
  UBRR0H = (unsigned char)0;// 


  /*Set baud rate 250K*/
  //UBRR0L = (unsigned char)3;
  //UBRR0H = (unsigned char)0;


  /*Set baud rate 38400*/
  //UBRR0L = (unsigned char)25;
  //UBRR0H = (unsigned char)0;

  
  /* Enable receiver and transmitter */
  UCSR0B = (1<<RXEN0)|(1<<TXEN0);

  // Enable recieve interrupt

  UCSR0B |= _BV( RXCIE0);

  // Leaved default settings for N-8-1
  
  //Serial.begin(100000);


}


#define SETROWBITS(ROW)	(PORTB = ROW)         // Set the seelct bits that drive a row of LEDs

void setupRowDDRbits() {

	DDRB |= 0b00000111;			// row bits

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
    
}

inline void SPI_MasterTransmit(char cData)
{
  
  /* Wait for any previous transmission to complete */
  /* Start transmission */

  // TODO: Why don't these work? SPI hardware bug?

  //while(!(SPSR & (1<<SPIF)));  
  //do {
//  SPSR;
    SPDR = cData;
    //SPSR;             
    
  //} while (SPSR & _BV(WCOL));
 while(!(SPSR & (1<<SPIF)));  

  // TODO: Blind send on the SPI to maximize output speed. Just need to count clock cycles to make sure we don't overrrun.
  
}

// Set a dot directly in the SPI buffer - displayed immedeately on next refresh pass

void setDot( int row, int col ) {

  spiBuffer[ ( row * ROW_BYTES ) + ( col / 8 ) ] |= 1 << ( col & 7 ) ;

}


void clearDot( int row, int col ) {

  spiBuffer[ ( row * ROW_BYTES ) + ( col / 8 ) ] &= ~ (1 << ( col & 7 )) ;

}


uint8_t isr_row = ROWS;  // Row to display on next update

uint16_t spiBufferPtr = PADDED_COLS * ROWS;      // Where are we in the buffer - we will stream one row of bytes on each cycle
                                           // We read tail to head becuase it is slightly faster to compare to 0 at the end of each pass

// Called from timer interrupt to refresh the next row of the LED display

void refreshRow()
{

  if (packetTimeout) packetTimeout--;     // Used to reset the Recieving serial packet pointer


  // The spiBufferPtr is always left pointing to the last byte of next row of bytes to go out
  
  // Everything is staged and ready to quickly squirt out the bytes over SPI
  // Once the set the row bits, the LEDs are off so the race is on! The longer the LEDs are off, the dimmer the display will look.

	SETROWBITS(0x07);		// Select ROW 8- which is actually a dummy row for selecting the data bit  
							  // Would be nice to leave the LEDs on while shifting out the cols but
							  // unfortunately then we'd display arifacts durring the shift

  delayMicroseconds(1);   // Give the darlingtons a chance to turn off so we don't get visual artifacts when we start shifting bits in
                          // Added this becuase I once saw some ghosting to the right on a single string - probably not needed on most controllers


  spiBufferPtr = isr_row * ROW_BYTES;   // Remeber isrrow is +1 here, so we are pointing one past the end of the row in the buffer 
  
  uint16_t spiCount = ROW_BYTES;      // Send one row per refresh - could also say spiCount = PADDED_COLS / 8 

  while (--spiCount) {

      SPI_MasterTransmit( spiBuffer[--spiBufferPtr] );      // (pre-decrement indirect addressing faster in AVR), also works becuase first bit sent gets shifted to rightmost dot on display

     // __builtin_avr_delay_cycles(2);
      
  }

  // note that spiBufferPtr will naturally point to the next row here, and keeps on decrementing though the full buffer acorss all rows
	
	// turn on the row of LEDs... (also sets clock and data low)

	SETROWBITS( --isr_row );

  if ( isr_row == 0 ) {     // Did we scan out all the rows?

		//spiBufferPtr = BUFFER_SIZE;     // Start over at the end of the buffer
    isr_row = ROWS;
    sync=0;                         // Signal vertical retrace so forground knows when to start drawing to avoid tearing on the display

	}

}

void setupTimer() {


	Timer1.initialize(1000000UL / (FRAME_RATE*ROWS));			// uSeconds between row refreshes
	Timer1.attachInterrupt(refreshRow); 

}



void setup() {

  
  // put your setup code here, to run once:

  //if (F_CPU == 16000000) clock_prescale_set(clock_div_1);  // For trinket - switch to full speed 16mhz prescaler
  // https://learn.adafruit.com/introducing-trinket/16mhz-vs-8mhz-clock

  setupRowDDRbits();

  DDRC |= _BV(5) | _BV(4);
  
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

/*
      for( int c = 0; c< COLS ; c++ ) {

        for( int r = 0 ; r < ROWS ; r++ ) {

          setDot( r , c ) ;
          
          SYNC();

          //clearDot( r , c ) ;
          

        }
      }
        

      for( int c = 0; c< COLS ; c++ ) {

        for( int r = 0 ; r < ROWS ; r++ ) {

          //setDot( r , c ) ;
          
          SYNC();

          clearDot( r , c ) ;
          

        }
  */      

    for( int c = 0; c< COLS ; c++ ) {

        
      SYNC();

      for( int r = 0 ; r < ROWS ; r++ ) {

        setDot( r , c ) ;

      }
   
    }

      
    for( int c = 0; c< COLS ; c++ ) {

      SYNC();

      for( int r = 0 ; r < ROWS ; r++ ) {

        clearDot( r , c ) ;

      }
        

    }

    for( int r=0; r<ROWS; r++ ) {
      SYNC();

      for( int c=0; c< COLS;c++ ) {

          setDot( r , c );
          
      }

      delay(50);

    }

    for( int r=0; r<ROWS; r++ ) {
      SYNC();

      for( int c=0; c< COLS;c++ ) {

          clearDot( r , c );
          
      }

      delay(50);


    }

    SYNC();

    for( int r=0; r<ROWS; r++ ) {

      for( int c=0; c< COLS;c++ ) {

          if ( (r+c) & 1 ) {
            setDot( r , c );
          }
          
      }

    }

    delay(750);

    SYNC();

    for( int r=0; r<ROWS; r++ ) {

      for( int c=0; c< COLS;c++ ) {

          if ( (r+c) & 1 ) {
            clearDot( r , c );
          } else {
            setDot( r , c );
            
          }
          
      }

    }

    delay(750);

    while (1);      // EVerything now happens over ISR, so We do nothing...
    
}


