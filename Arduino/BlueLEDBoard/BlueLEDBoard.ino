// Drive BlueMan LED controller board

// ROW select bits on PB0-PB3

// COL CLOCK A on PD2
// COL CLOCK B on PD3
// Note that COL CLOCKs are ORed together, so only need one to high to drive

// COL DATA I7 on PD4. This bit will appear on DATA when ROW #7 is selected in ROW bits. 


 #include <avr/power.h>        
 

#define ROWS 7
#define COLS (5*84)     // 5 rows per char, 83 chars

#if (COLS>=1000)

  #error Too Many Cols!

#endif

uint8_t dots[ COLS ];   // Packed bits
                        // dot[0], bit 0 = upper rightmost LED
                        // dot[COLS-1], bit 7 = lower leftmost LED

volatile uint8_t isr_row=0;  // Row to display on next update

volatile uint8_t sync=0;  // if >0, then decremented after refresh so you can sync to it


// Interrupt is called once a millisecond to refresh the LED display
SIGNAL(TIMER0_COMPA_vect) 
{
/*
    int i=10;
    while(i--) {
      PORTB=0x0;
      PORTB=0b00011000;
    }
    return;
      
*/

  uint8_t row_mask = 1 << isr_row;    // For quick bit testing
  
  uint16_t c=COLS;

  PORTD = 0x07;    // Select ROW 8- which is actually a dummy row for selecting the data bit  
                   // Would be nice to leave the LEDs on while shifting out the cols but
                   // unfortunately then we'd display arifacts durring the shift

                   // Also sets clock and data low

  // Load up the col bits...
  
  while (c--) {

    PORTB &= ~_BV(3);     // Clock low    
    
    if ( dots[c] & row_mask) {      // Current row,col set?
         PORTB |= _BV( 4 );  // Set Data high      
    } else {
         PORTB &= ~_BV( 4 );  // Set Data low       
    }
    
    
    //asm("nop");
    
    PORTB |=   _BV(3);    // Clock high
    
    //asm("nop");

    // Pulse clock      
    
    //asm("nop");
    
    
    //asm("nop");      // Streching the clock seems to help with long strings proabbly becuase of propigation delay thuogh the shift registers
            
  }
  
  // turn on the row... (also sets clock and data low)
  
  PORTD = isr_row;
  
  // get ready for next time...
  
  if (isr_row++ >= ROWS) {
    
    isr_row = 0;
    
  }
  
  if (sync) sync--;
  
}

void setupTimer() {
  // Timer0 is already used for millis() - we'll just interrupt somewhere
  // in the middle and call the "Compare A" function below
  OCR0A = 0xAF;
  TIMSK0 |= _BV(OCIE0A);  
}


void setup() {
  // put your setup code here, to run once:
  
  if (F_CPU == 16000000) clock_prescale_set(clock_div_1);  // For trinket - switch to full speed 16mhz prescaler
  // https://learn.adafruit.com/introducing-trinket/16mhz-vs-8mhz-clock
  
  PORTB = 0x00;
 
  DDRB = 0b00011000;    // , 3=clock, 4=data
  DDRD = 0b00000111;    // 0-2=row select


  setupTimer();
}

#define DELAY 400

#define SYNC(n) sync=n;while(sync)

#define SETDOT(c,r) dots[c] |= _BV(r)
#define CLRDOT(c,r) dots[c] &= ~_BV(r)

// Only set or clr if on screen
#define SETDOTP(c,r) if (c>=0 && c<COLS && r>=0 && r<ROWS) SETDOT(c,r)
#define CLRDOTP(c,r) if (c>=0 && c<COLS && r>=0 && r<ROWS) CLRDOT(c,r)


void drawship( int x, uint8_t y, int j ) {
      
      for (int q=-1;q<=1;q++) {

        for(int p=-3+abs(q);p<=3-abs(q);p++) {        
          SETDOTP( x+p  , y+q);
        }
      
      }
      
      // Blinking top light
      
      if ( j& 0x10 ) {
       SETDOTP( x, y - 2);
      }
                  
      // Make ship spin
            
      int shiplight = (j/10 % 13);
      
      CLRDOTP( x+shiplight-4 , y );  
}

void undrawship( int x, uint8_t y ) {

      for (int q=-1;q<=1;q++) {

        for(int p=-3+abs(q);p<=3-abs(q);p++) {        
          CLRDOTP( x+p  , y+q);
        }
      
      }
      
      // Blinking top light
      
      CLRDOTP( x, y - 2);
  
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


// x is left side, always drawn full height

void drawmap( uint16_t center , const uint8_t *map , uint8_t w) {

   int x=center-(w/2);
   // Left half
   int i=0;
  
   while (i<w) {    
    if (x>=0 && x<COLS) dots[x] |= map[i];    
    x++;
    i++;
  }
    
}


void undrawmap( uint16_t center , const uint8_t *map , uint8_t  w) {

   int x=center-(w/2);
   // Left half
   int i=0;
   while (i<w) {    
    if (x>=0 && x<COLS) dots[x] &= ~map[i];    
    x++;
    i++;
  }
    
}


// Clear the display

void clear() {
  for(int c=0; c<COLS;c++) dots[c]=0x00;  // Clear screen  
}


 #define STARS 10
  
  // all fixed point with Assumed 1 decimals
int starsX[STARS];
int starsY[STARS];
int starsDX[STARS];
int starsDY[STARS];


void drawalien( uint16_t x, uint8_t i, uint8_t open) {
  
  if (i==0) {  // alien 1
  
    if (open) {   // open
    
      drawmap( x , enemy1o , sizeof( enemy1o) );
    
    } else {    // close
    
      drawmap( x , enemy1c , sizeof( enemy1c) );
    
    }
  
  } else if (i==1) {    // enemyt 2

    if (open) {   // open

      drawmap( x , enemy2o , sizeof( enemy2o) );

    } else {    // close

      drawmap( x , enemy2c , sizeof( enemy2c) );
      
    }

  } else if (i==2) {   // elplosion
  
     drawmap( x , explode , sizeof( explode) );
      
  } else if (i==3) {            // rubble
  
     drawmap( x , rubble , sizeof( rubble ) );
    
  }  else if (i==4) {            // rubble
  
    if (open) {   // open
    
      drawmap( x , graveo , sizeof( graveo) );
    
    } else {    // close
    
      drawmap( x , gravec , sizeof( gravec) );
    
    }
    
  }    

    
}


void undrawalien( uint16_t x, uint8_t i, uint8_t open) {
  
  if (i==0) {  // alien 1
  
    if (open) {   // open
    
      undrawmap( x , enemy1o , sizeof( enemy1o) );
    
    } else {    // close
    
      undrawmap( x , enemy1c , sizeof( enemy1c) );
    
    }
  
  } else if (i==1)  {    // enemy 2

    if (open) {   // open

      undrawmap( x , enemy2o , sizeof( enemy2o) );

    
    } else {    // close

      undrawmap( x , enemy2c , sizeof( enemy2c) );

    }
  }    
    
}


#define ALIENCOUNT 6

void loop() {
  

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

   
  for( int s=0; s<(COLS+(13*6)); s++ ) {      // s=center of leftmost alien 
  
    clear();
    
    for( int a=0; a<6; a++) {        // Six aliens
    
      drawalien( s + (a*13) , a&1 , (s+8)&16);
    
    }
    
    SYNC(50);    
    
//    delay(10);
    
  }
  
  
  // Now flying forward...
  
  clear();
  delay(DELAY);
    
  // init stars
  for( int s=0;s<STARS;s++) {
    
    // Center with random forward velocity
    starsX[s]=random(COLS*10);
    starsY[s]=random(ROWS*10);
    starsDX[s]=(random(9)+1) * -1;    
  }
  
  
  int shipX = (-40)*10;      // Start ship to the left off the screen 40 cols
  
  while (shipX<COLS*10) { 
    
      clear();

      //undrawship( (shipX/100) , ROWS/2 );
    
      shipX += 2;

      drawship( shipX/10 , ROWS/2 , shipX/7 );
    
      for(int s=0;s<STARS;s++) {
        
//        uint16_t oldX = starsX[s];
  //      uint16_t oldY = starsY[s];
          
        starsX[s] += starsDX[s];        
        
        // Off display hoazontally?
        if (starsX[s] < 0) {
          starsX[s]=(COLS-1)*10;
          starsY[s]=random(ROWS*10);
          starsDX[s]=(random(9)+1) * -1;                       
        }
         
        // Erase old star pos
//        CLRDOT( oldX/100 , oldY/10 );    
        
        // Draw new
        SETDOT( starsX[s]/10 , starsY[s]/10 );
      }
            
                    
      SYNC(2);
             
  }
  
  
  
  
  int l=0;
    
  for(int i=0; i<COLS; i++ ){
    
    dots[i] = 0b10101010;
    
  }
  
  delay(DELAY);
  
  for(int i=0; i<COLS; i++ ){
    
    dots[i] = 0b01010101;
    
  }
  
  delay(DELAY);
  
  for(int i=0; i<COLS; i++ ){
    
    dots[i] = 0xff;
    
  }
  
  delay(DELAY);
  
  for(int i=0; i<COLS; i++ ){
    
    dots[i] = 0;
    
  }
  delay(DELAY);  
  
  
  for(int i=0; i<COLS; i++ ){
    
    if (i&1) {
     dots[i] = 0xff;
    } else {
      dots[i] = 0x00;
    }
    
  }
  delay(DELAY);  


  for(int i=0; i<COLS; i++ ){
    
    if (i&1) {
     dots[i] = 0x00;
    } else {
      dots[i] = 0xff;
    }
    
  }
  delay(DELAY);  

  for(int i=0; i<COLS; i++ ){
    
    dots[i] = 0xff;
    
  }
  
  delay(DELAY);
  
  for(int i=0; i<COLS; i++ ){
    
    dots[i] = 0;
    
  }
  delay(DELAY);  
  
  
  // Hello world cross hatch pattern  
  
  for(int i=0; i<COLS; i++ ){
    
    dots[i] = 0b10101010;
    
  }
  
  delay(DELAY);
  
  for(int i=0; i<COLS; i++ ){
    
    dots[i] = 0b01010101;
    
  }
  
  delay(DELAY);

  
  for(int i=0; i<COLS; i++ ){
    
    if (i&1) {
      dots[i] =0b01010101;      
    } else {
     dots[i] = 0b10101010;
      
    }
    
  }
  delay(DELAY);  


  for(int i=0; i<COLS; i++ ){
    
    if (i&1) {
     dots[i] = 0b10101010;
    } else {
      dots[i] = 0b01010101;
    }
    
  }
  delay(DELAY);  

  for(int i=0; i<COLS; i++ ){
    
    dots[i] = 0xff;
    
  }
  
  delay(DELAY);
  
  for(int i=0; i<COLS; i++ ){
    
    dots[i] = 0;
    
  }
  delay(DELAY);  


  for(int i=0; i<COLS; i++ ){
    
    dots[i] = 0xff;
    SYNC(6);
    
  }
  delay(DELAY);  

  for(int i=0; i<COLS; i++ ){
    
    dots[i] = 0x00;
    SYNC(6);
    
  }
  delay(DELAY);  

  for(int i=COLS; i>=0; i-- ){
    
    dots[i] = 0xff;
    SYNC(6);
    
  }
  delay(DELAY);  

  for(int i=COLS; i>=0; i-- ){
    
    dots[i] = 0x00;
    SYNC(6);
    
  }
  delay(DELAY);  



  for(int r=0; r<ROWS; r++ ){
    
    uint8_t m = ( 1<< (r+1)) -1;
    
    for( int c=0; c<COLS; c++ ) { 
      dots[c] = m;
    }
    
    SYNC(100);
    
  }
//  delay(DELAY);  


  for(int r=0; r<ROWS; r++ ){
    
    uint8_t m = ~ (( 1<< (r+1)) -1) ;
    
    for( int c=0; c<COLS; c++ ) { 
      dots[c] = m;
    }
    
    SYNC(100);
    
  }
  
  for(int r=0; r<ROWS; r++ ){
    
    uint8_t m = ~ ((1 << (ROWS-r) ) -1);
    
    for( int c=0; c<COLS; c++ ) { 
      dots[c] = m;
    }
    
    SYNC(100);
    
  }
//  delay(DELAY);  
  
  
//  delay(DELAY);  

  for(int r=0; r<ROWS; r++ ){
    
    uint8_t m = (1 << ((ROWS-1)-r) ) -1;
    
    for( int c=0; c<COLS; c++ ) { 
      dots[c] = m;
    }
    
    SYNC(100);
    
  }
//  delay(DELAY);  

  randomSeed(100);

  for( int l=0; l< 2000; l++ ) {
    
    int r = random( COLS *ROWS );
    int col = r/ROWS;
    int row = r%ROWS;
    
    dots[ col ] |= 1 << row;
    
    SYNC(1);
    
  }

  randomSeed(100);

  for( int l=0; l< 2000; l++ ) {
    
    int r = random( COLS *ROWS );
    int col = r/ROWS;
    int row = r%ROWS;
    
    dots[ col ] &= ~(1 << row);
    
    SYNC(1);
    
  }
    
  
  clear();
  delay(DELAY);         

}
