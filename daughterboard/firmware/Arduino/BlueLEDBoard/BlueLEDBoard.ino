
// Drive BlueMan LED controller board


// COL DATA   - MOSI - PB3 - Chip pin 17 - Arduino Pin 11 - Board IC1 pin 32
// COL CLOCK  - SCK  - PB5 - Chip pin 19 - Arduino pin 13 - Board IC1 pin  7

// ROW SELECT - PB0-PB2 - Chip pins 14-16 - Arduino pins 8-10 - Board IC1 pins 1-3

// NOTE: ROW SELECT #3 is hardwired to ground on the daughter board
// NOTE: code assumes it is ok to directly assign row to full 8 bits of PB even though only bottom 3 bits are used

// DATAb connected directly to ground (data lines are ORed on the board) - IC1 pin 17

#include <TimerOne.h>
#include <avr/power.h>        

#include "font5x7.h"

#define FRAME_RATE 80

#define ROWS 7

#define CHARS 150                      // Number of char modules
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

volatile uint8_t packetFlag=0;      // 1=new packet waiting in readBuffer

volatile uint8_t demoMode = 1;      // Start in demo mode. Automatically turns off when we recieve a good frame from serial port.

unsigned long demoCount = 0 ;

inline void pushSerialByte( unsigned char c) {   // Put a byte into the serial buffer


    if (!packetTimeout) {

            readBufferHead=0;
                  
    }

    readBuffer[readBufferHead++] = c;

    packetTimeout = 4;

    if ( readBufferHead == BUFFER_SIZE) {

      // TODO: Double or tripple buffer this so that we just update a flag to tell the refreh thread to use the new buffer

      packetFlag = 1 ;        //Signal to refresh process that we have a new packet ready

      readBufferHead=0;

      demoMode =0;            // Stop showing demo now that we have a good frame 
      
    }

}

SIGNAL(USART_RX_vect) {

  while ( UCSR0A & _BV( RXC0 ) ) {    // While chars are available...

    unsigned char c = UDR0;
    
    pushSerialByte( c ); 

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

void clearDots() {

  memset( spiBuffer ,  0 , BUFFER_SIZE );
  
}

volatile uint8_t spi=0;

uint8_t isr_row = ROWS;  // Row to display on next update

uint16_t spiBufferPtr = PADDED_COLS * ROWS;      // Where are we in the buffer - we will stream one row of bytes on each cycle
                                           // We read tail to head becuase it is slightly faster to compare to 0 at the end of each pass

// We need a little minibufer just to process any serial bytes that come in while we are in the middle of our SPI
// squirt. This squirt must complete as fast as possible (becuase all LEDs are off while it is running), so we can't
// let any interrupts happen while it is going on. Instead, we will poll the serial port and grab any bytes that come in
// Quickly store them into the tiny buffer. That buffer only has to be as big as the maximum number of bytes that can possibly
// come in at the current baud rate dirring the time it takes ot send one row via SPI. Since we know that the Serial port
// cant recieve bytes faster than the current baud rate, we do not need to check for buffer overruns when storing into the 
// mini buffer which saves time. 

// (1,000,000 bits/sec) / (10 bits/byte) = 100,000 bytes/sec serial = 10us/byte serial
// (8,000,000 bits/sec) /(8 bits/byte) = 1,000,000 bytes/sec SPI = 1us/byte SPI
// 84*5 ~= 500 bits/row
// (500 bits/row) * (1/(8,000,000 s/bit)) =  63us/row
// http://www.wolframalpha.com/input/?i=%28500+bits%2Frow%29+*+%28+%281%2F8%2C000%2C000%29+s%2Fbit%29+in+us
// (63us/row SPI) / (10us/byte serial) ~= 6 bytes serial/row SPI
// We will make the mini buffer much bigger since we have planety of room and the SPI is not 100% fast 

unsigned char miniBuffer[100];                                           

// Called from timer interrupt to refresh the next row of the LED display

void refreshRow()
{

  if (packetTimeout) packetTimeout--;     // Used to reset the Recieving serial packet pointer


  // The spiBufferPtr is always left pointing to the last byte of next row of bytes to go out
  
  // Everything is staged and ready to quickly squirt out the bytes over SPI
  // Once the set the row bits, the LEDs are off so the race is on! The longer the LEDs are off, the dimmer the display will look.



  spiBufferPtr = isr_row * ROW_BYTES;   // Remeber isrrow is +1 here, so we are pointing one past the end of the row in the buffer 
  
  uint16_t spiCount = ROW_BYTES;      // Send one row per refresh - could also say spiCount = PADDED_COLS / 8 

  uint8_t *miniBufferPtr=miniBuffer;

  SETROWBITS(0x07);   // Select ROW 8- which is actually a dummy row for selecting the data bit  
                // Would be nice to leave the LEDs on while shifting out the cols but
                // unfortunately then we'd display arifacts durring the shift

  delayMicroseconds(1);   // Give the darlingtons a chance to turn off so we don't get visual artifacts when we start shifting bits in
                          // Added this becuase I once saw some ghosting to the right on a single string - probably not needed on most controllers
  
  // Now the LEDs are off so we need to run full blast. The longer they are off, the dimmer they will appear. 

//spiCount = 3; // TODO: TEsting only

  while (spiCount--) {

      SPI_MasterTransmit( spiBuffer[--spiBufferPtr] );      // (pre-decrement indirect addressing faster in AVR), also works becuase first bit sent gets shifted to rightmost dot on display

      if ( UCSR0A & _BV( RXC0 ) ) {    // If a char is available, save it for later. We only need to check for one char becuase we knw that the serial port is running slower than the SPI port so never more than ! byte serial ready for each SPI pass

        unsigned char c = UDR0;

        *(miniBufferPtr++) = c;

      }
        
  }
  
  // note that spiBufferPtr will naturally point to the next row here, and keeps on decrementing though the full buffer acorss all rows

	// turn on the row of LEDs... (also sets clock and data low)

	SETROWBITS( --isr_row );

  // now that row of LEDs is back on, we have time to drain any chars that came into the mini buffer while we were squirting

  // Note that interrupts are still off here and we can't turn them back on becuase the new incoming bytes could scramble the 
  // ones we are dumping from the mini buffer  
  
  uint8_t *miniBufferReaderPtr = miniBuffer;

  while (miniBufferReaderPtr<miniBufferPtr) {

      // we still must keep manually checking the serial buffer becuase unloading the mini buffer tkaes longer than incoming serial data to fill the buffer

      if ( UCSR0A & _BV( RXC0 ) ) {    // If a char is available, save it for later. We only need to check for one char becuase we knw that the serial port is running slower than the SPI port so never more than ! byte serial ready for each SPI pass

        unsigned char c = UDR0;

        *(miniBufferPtr++) = c;

      }
    

      pushSerialByte( *(miniBufferReaderPtr++ ) );   

  }

  sei();      // Ok, let the serial interrupt take back over while we possibly do a time consuming buffer switch
  // TODO: Wont need this when the buffer switch is juts a pointer rather than a memcpy()
  

  // Note that this should natually just work in concert with the ISR since interrupts are off while we are refreshing the row
  // and if we clear the Serial buffer then there should not be an ISP when we reenable interrupts after the row is done.

  // A USART Receive Complete interrupt will be
  // generated only if the RXCIEn bit is written to one, the Global Interrupt Flag in SREG is written to one and the
  // RXCn bit in UCSRnA is set.  

 
  if ( isr_row == 0 ) {     // Did we scan out all the rows?


    // TODO: have free running spiBuffer thta only gets reset on vertical retrace
		//spiBufferPtr = BUFFER_SIZE;     // Start over at the end of the buffer
    isr_row = ROWS;
    sync=0;                         // Signal vertical retrace so forground knows when to start drawing to avoid tearing on the display


    if (packetFlag) {     // New packet available in serial buffer?

      // TODO: Just update a pointer rather than mempying the whole thing over

      memcpy( spiBuffer , readBuffer , BUFFER_SIZE );   // Copy the entire buffer even though it might not be full just to keep timing consistant
      packetFlag =0;

      UDR0 = 'V';    // Signal back to the controller that we just started displaying the pending packets so ok to send the next one. 

    } else {

      UDR0 = 'H';    // Signal back to the controller that we just did a repeat frame refresh
      
    }
	}

}

void setupTimer() {


	Timer1.initialize(1000000UL / (FRAME_RATE*ROWS));			// uSeconds between row refreshes
	Timer1.attachInterrupt(refreshRow); 

}

void drawChar( int x , unsigned char c ) {

  for (int col = 0; col < 5; col++) {

    unsigned char rowbits = pgm_read_byte_near( Font5x7 + ((c - 0x20)*5) + col );    

    for (int row = 0; row < 7; row++) {

      if (rowbits & (1 << row)) {

        setDot( row , x + col );

      }

    }

  }  
}


void showNumber( int x , unsigned long int n ) {

  while (n) {

    unsigned long int o = n/10;

    uint8_t digit = n - (o*10);

    drawChar( x , digit + '0' );

    n = o;
    x+= 7;        // Leave a 1 col space between consecutive digits
    
  }
  
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

  for( int c=0; c< COLS; c+=2 ) {

    for( int r=0; r<ROWS; r++ ) {

      setDot( r ,c );
      
    }
    
  }

  while (1) {

    spi=0;
    delay(500);
    spi=1;
    delay(500);

    
  }

  */

  

    for( int c = 0; demoMode && c< COLS ; c++ ) {
        
      SYNC();

      for( int r = 0 ; demoMode && r < ROWS ; r++ ) {

        setDot( r , c ) ;

      }
   
    }

      
    for( int c = 0; demoMode && c< COLS ; c++ ) {

      SYNC();

      for( int r = 0 ; demoMode && r < ROWS ; r++ ) {

        clearDot( r , c ) ;

      }
        

    }

    for( int r=0; demoMode && r<ROWS; r++ ) {
      SYNC();

      for( int c=0; demoMode && c< COLS;c++ ) {

          setDot( r , c );
          
      }

      delay(50);

    }

    for( int r=0; demoMode && r<ROWS; r++ ) {
      SYNC();

      for( int c=0; demoMode && c< COLS;c++ ) {

          clearDot( r , c );
          
      }

      delay(50);


    }

    // Cross hatch left

    SYNC();

    for( int r=0; demoMode && r<ROWS; r++ ) {

      for( int c=0; demoMode && c< COLS;c++ ) {

          if ( (r+c) & 1 ) {
            setDot( r , c );
          } else {
            clearDot( r , c );
          }
          
      }

    }

    delay(750);

    // Cross hatch right
    

    SYNC();

    for( int r=0; demoMode && r<ROWS; r++ ) {

      for( int c=0; demoMode && c< COLS;c++ ) {

          if ( (r+c) & 1 ) {
            clearDot( r , c );
          } else {
            setDot( r , c );
            
          }
          
      }

    }

    delay(750);

    // Up counter

    while (demoMode) {

        SYNC();
        SYNC();
        
        clearDots();
        showNumber( 0, demoCount++ );
        
    }

   
    while (1);      // EVerything now happens over ISR, so We do nothing...
    
}


