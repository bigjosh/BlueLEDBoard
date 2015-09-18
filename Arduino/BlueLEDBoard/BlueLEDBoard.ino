// Drive BlueMan LED controller board

// ROW select bits on PB0-PB3

// COL CLOCK A on PD2
// COL CLOCK B on PD3
// Note that COL CLOCKs are ORed together, so only need one to high to drive

// COL DATA I7 on PD4. This bit will appear on DATA when ROW #7 is selected in ROW bits. 


#define ROWS 7
#define COLS (5*18)     // 5 rows per char, 100 chars

uint8_t dots[ COLS ];   // Packed bits
                        // dot[0], bit 0 = upper rightmost LED
                        // dot[COLS-1], bit 7 = lower leftmost LED


volatile uint8_t sync=0;  // if >0, then decremented after refresh so you can sync to it

volatile uint8_t  dutyBits[256];        // Duty cycle data. 

void setFullBrightnessDuty() {
  
  for(int i=0;i<256;i++) dutyBits[i]=0b01111111;
  
}

void setRainbowDuty() {
  
  
  for(int i=0;i<256;i++) dutyBits[i]= 0x00;
   
  for(int i=0;i<255;i+=1) dutyBits[i] |= _BV(0)  | _BV(6) ;
  
  for(int i=0;i<255;i+=2) dutyBits[i] |= _BV(1) | _BV(5);    // 50% for top and bottom rows
  for(int i=0;i<255;i+=4) dutyBits[i] |= _BV(2) | _BV(4);
  
  for(int i=0;i<255;i+=5) dutyBits[i] |= _BV(3);    // middle
  
}


// Interrupt is called once a millisecond to refresh the LED display
SIGNAL(TIMER0_COMPA_vect) 
{
  
  static uint8_t isr_row=0;    // Row to display on next update
  static uint8_t isr_count=0;  // counter for PWMing rows 
  static uint8_t dutyCount=0;          // counter cycles from 0-255. each bit in dutyBits[dutyCounter] is 1 if that row should be on at this point in the cycle. Must be 8 bits, rolls over at 255
  
  uint8_t row_mask = 1 << isr_row;    // For quick bit testing
  

  PORTB = 0x07;    // Select ROW 8- which is actually a dummy row for selecting the data bit  
                   // Would be nice to leave the LEDs on while shifting out the cols but
                   // unfortunately then we'd display arifacts durring the shift
                   
  if ( dutyBits[dutyCount] & row_mask ) {      // No need to check dutyCount for overflow since it is only 8 bits it will wrap.
  
    // This loop could be hyperoptimized in ASM by
    // *using a single byte loop counter
    // *preloading rhe high and low bitfields into registers and then just picking which one to send 
    // using boolean logic rather than a branch
    
    // Load up the col bits...

    uint16_t c=COLS;

    while (c--) {
      
      if ( dots[c] & row_mask) {      // Current row,col set?
           PORTD |= _BV( 4 ) ;  // Set Data high      
      } else {
           PORTD &= ~_BV( 4 );  // Set Data low       
      }
  
      // Pulse clock      
      PORTD |=   _BV(2);    // Clock high
      PORTD &= ~_BV(2);     // Clock low
          
    }
    
    // turn on the row...
    
    PORTB = isr_row;
    
  }
    
  if (isr_row++ >= ROWS ) {
    
    isr_row=0;
    dutyCount++;      // Just finished a full refresh, step to the next setting
    
  }
  
  
  // get ready for next time...
    
  if (sync) sync--;
  
}

void setupTimer() {
  // Timer0 is already used for millis() - we'll just interrupt somewhere
  // in the middle and call the "Compare A" function below
  OCR0A = 0xAF;
  TIMSK0 |= _BV(OCIE0A);  
}



/// set all row bits on
void fullrow() {
  
  PORTB = 0x07;    // Select ROW 8- which is actually a dummy row for selecting the data bit
  PORTD |= _BV( 4 );  // Set Data high
    
  for( int i=0; i<COLS; i++) {
    
      // Pulse clock
      
      PORTD |=   _BV(2);    // Clock high
    //  delay(1);
      PORTD &= ~_BV(2);     // Clock low
    //  delay(1);  
    
  }
  
}


void hwiped() {
  
  fullrow();
  for( uint8_t r=0; r< ROWS; r++ ) {
    PORTB=r;
    delay(60);    
  }
  
  PORTB=0x00;  
}

void hwipeu() {
  
  fullrow();
  for( int r=ROWS; r>=0;  r-- ) {
    PORTB=r;
    delay(60);    
  }
  
  PORTB=0x00;  
}


void vwiper() {
  
  
  // step though the row to display...
    
  for( int s=COLS; s>0;  s-- ) {
    
    // Load up row bits...
    
    PORTB=0x07;  // Turn off rows to begin loading cols
    
    for( int c=0; c<COLS; c++) {

     PORTD &= ~_BV(2);     // Clock low
      
       if (s==c) {          // Light this Row?
         PORTD |= _BV( 4 );  // Set Data high
       } else {
         PORTD &= ~_BV( 4 );  // Set Data low       
       }
          
       PORTD |=   _BV(2);    // Clock high - latch bit
    
    }

    for( int l=0; l<2; l++ ) {    // Repeat the ROW scan this many times
    
      for( uint8_t r=0; r<ROWS; r++) {
        
        PORTB = r;      // Show the row
        delay(1);       // Let it persist
        
      }
    }  
  }
}

void vwipel() {
  
  
  // step though the row to display...
    
  for( int s=0; s<COLS;  s++ ) {
    
    // Load up row bits...
    
    PORTB=0x07;  // Turn off rows to begin loading cols
    
    for( int c=0; c<COLS; c++) {

     PORTD &= ~_BV(2);     // Clock low
      
       if (s==c) {          // Light this Row?
         PORTD |= _BV( 4 );  // Set Data high
       } else {
         PORTD &= ~_BV( 4 );  // Set Data low       
       }
          
       PORTD |=   _BV(2);    // Clock high - latch bit
    
    }

    for( int l=0; l<2; l++ ) {    // Repeat the ROW scan this many times
    
      for( uint8_t r=0; r<ROWS; r++) {
        
        PORTB = r;      // Show the row
        delay(1);       // Let it persist
        
      }
    }  
  }
}

void allon() {
  
  fullrow();
  
  
    for( int l=0; l<50; l++ ) {    // Repeat the ROW scan this many times
    
      for( uint8_t r=0; r<ROWS; r++) {
        
        PORTB = r;      // Show the row
        delay(1);       // Let it persist
        
      }
    }  
  
}

void setup() {
  // put your setup code here, to run once:
  
    
  
  PORTB = 0x00;
  PORTD = 0x00;

  DDRB = 0x0f;
  DDRD = _BV(2) | _BV(3) | _BV(PD4);
  
  setupTimer();
}

#define DELAY 400

#define SYNC(n) sync=n;while(sync)

#define SETDOT(c,r) dots[c] |= _BV(r)
#define CLRDOT(c,r) dots[c] &= ~_BV(r)

// Only set or clr if on screen
#define SETDOTP(c,r) if (c>=0 && c<COLS && r>=0 && r<ROWS) SETDOT(c,r)
#define CLRDOTP(c,r) if (c>=0 && c<COLS && r>=0 && r<ROWS) CLRDOT(c,r)


void drawship( int x, int y, int j ) {
      
      for (int q=-1;q<=1;q++) {

        for(int p=-3+abs(q);p<=3-abs(q);p++) {        
          SETDOTP( x+p  , y+q);
        }
      
      }
      
      // Blinking top light
      
      if ( j& 0x10 ) {
       SETDOTP( x, y - 2);
      }
      
      float angle = ((j/2000.0) * 2 * 3.1415 * 20 );
            
      // Make ship spin
            
      int shiplight = ( cos( angle ) * ( sin( angle ) < 0 ? -1 : 1 ) ) * 4  ;
      
      CLRDOTP( x+shiplight , y );  
}

void undrawship( int x, int y ) {

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

void drawmap( int center , const uint8_t *map , int w) {

   int x=center-(w/2);
   // Left half
   int i=0;
   while (i<w) {    
    if (x>=0 && x<COLS) dots[x] |= map[i];    
    x++;
    i++;
  }
    
}


void undrawmap( int center , const uint8_t *map , int w) {

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

// all on
void fill() {
  for(int c=0; c<COLS;c++) dots[c]=0xff;  // Fill screen  
}

 #define STARS 10
  
  // all fixed point with Assumed 2 decimals
int starsX[STARS];
int starsY[STARS];
int starsDX[STARS];
int starsDY[STARS];


void drawalien( int x, int i, uint8_t open) {
  
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


void undrawalien( int x, int i, uint8_t open) {
  
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
  
  
  setRainbowDuty();
//  setFullBrightnessDuty();

  fill();
  
  delay(2000);
  
  setFullBrightnessDuty();

  
  // Set up alien locations
    
  for( int s=0;s<ALIENCOUNT;s++) {
    
    // Center with random velocity
    starsX[s]=( (COLS-(13*s)-8 + COLS ) *100 );      // Arange them with some space in between.
    starsDX[s]= -20;                       // not moving    
    
    starsDY[s]=s&1;                    // Coop this field to hold alien type
  }
  
  
  clear();

  for(int l=0; l<900; l++ ) {
    
    clear();
    
    for( int s=0; s<ALIENCOUNT; s++ ) {
      
      starsX[s] += starsDX[s];
      
      drawalien( starsX[s]/100 , starsDY[s] , l&32 );
      
    }
    
    SYNC(10);
    
  }
  
 
  
  // Running away aleins get fired upon and explode to rubble
  
  for(int s=0; s<ALIENCOUNT; s++) {
    
    starsX[s] = ((-20*s) )*100;
    
    starsDX[s] = +60 ;    // ;    // Run away!!!! Give later guys more speed to spread the pack
    
  }
  
  starsDX[ALIENCOUNT-1] = 28;    // Last guy is a slow poke
  
  int shipX= starsX[ALIENCOUNT-1] - (50*100);      // start with ship far off to left in hot persuit
  int shipDX = 30;                                 // Same speed as slow alien
  
  int laserX=0;  
    
  for(int l=0; l<1000; l++ ) {
    
    clear();
        
        
     if (!laserX) {     // If as laser is not currently in process
     
        if (shipDX) {
          if (shipX>=20*100) {    // Fire! when we get half way though the first panel.
            laserX=shipX;              
            shipDX=0;
            
          }
        }
        
      } else {
        
        laserX+=100;    // Advance laser beam. Very fast laser
        
     }
    
    
    if (laserX) {   
      
      // Draw laser starting at ship    
       
      for(int x=shipX/100; x<laserX/100 ; x++ ) {
        
        // draw beam      
        SETDOTP( x , ROWS/2 );
  
        // Collision detection...
        
          
        if (x>= (starsX[ALIENCOUNT-1]/100)) {    // Direct hit!
        
          starsDY[ALIENCOUNT-1] = 2;      // turn to explosion!
          starsDX[ALIENCOUNT-1] = 0;      // halt in its trax
          
          laserX = 0;   // Stop laser
          //while(1);

        }
          
        
      }

    }
    
    drawship( shipX/100 , ROWS/2 , l );    // Draw the ship after the laser so the beam doesn obscure the spinner

    
    for( int s=0; s<6; s++ ) {
      
      starsX[s] += starsDX[s];
      
      if ( starsDY[s] == 0)  {        
        drawalien( starsX[s]/100 , 0 , l&8 );    // Open
      } else if  ( starsDY[s] == 1)  {
        drawalien( starsX[s]/100 , 1 , l&8 );    // Close
      } else if  ( starsDY[s] < 100)  {
          if ( starsDY[s] & 8 ) {
            drawalien( starsX[s]/100 , 2 , l&8 );    // Explosion
          }
        starsDY[s]++;
      } else if (starsDY[s] <160) {
        drawalien( starsX[s]/100 , 3 , l&8 );    // rubble
        starsDY[s]++;        
      } else if (starsDY[s] <200) {
        drawalien( starsX[s]/100 , 4 , 1 );    // mound
        starsDY[s]++;        
      } else if (starsDY[s] <240) {
        drawalien( starsX[s]/100 , 4 , 0 );    // grave
        //starsDY[s]++;        
      } 
      
    }
           
    shipX+=shipDX;  //FAST SHIP!
    
    SYNC(10);
    
  }
  
  //return;
  
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
  
  delay(DELAY);
  
  for( int s=0;s<STARS;s++) {
    
    // Center with random velocity
    starsX[s]=(COLS*(100/2));
    starsY[s]=(ROWS*(100/2));
    starsDX[s]=(random(90)+10) * (random(2)==1?1:-1);
    starsDY[s]=random(50)-25;    // Ok to have zero y velocity
    
  }
 

  int centerX = COLS/2;  
  
  for(int j=0;j<400;j++) {    

      // Undraw ship
      
      for (int q=-1;q<=1;q++) {

        for(int p=-3;p<=3;p++) {
        
          dots[ (centerX /100)+p   ] &= ~_BV((ROWS/2)+q);
        }
      
      }
      
      CLRDOT( (centerX/100) , (ROWS/2) - 2);
      

    
    
    // Make the center move to a sin wave, cenetered, spaning 75% width, 3 cycles
    centerX = ( (((sin( (j/2000.0) * 2 * 3.1415 * 5 )+1.0)/2.0) * (3.0/4.0)) + (1.0/8.0)) * (COLS * 100);
    
    
    for(int s=0;s<STARS;s++) {
      
      int oldX = starsX[s];
      int oldY = starsY[s];
        
      starsX[s] += starsDX[s];
      starsY[s] += starsDY[s];
      
      
      // Off display hoazontally?
      if (starsY[s]<0 || starsY[s]>=ROWS*100 || starsX[s] < 0 || starsX[s]>=COLS*100 ) {
        starsX[s]=centerX;
        starsY[s]=(ROWS*(100/2));
        
        // Random velocity -200 to +200 parabola. No values between -20 and +20 to prevent stuck stars
        starsDX[s]=(random(90)+10) * (random(2)==1?1:-1);
        starsDY[s]=random(50)-25;    // Ok to have zero y velocity
        
      }
      
      // only draw if still on the display veritcally
      
//      if (starsY[s]>=0 && starsY[s]<(ROWS*100)) {       
//       dots[starsX[s]/100] |= ( 1<<(starsY[s]/100 ) );        
//      }

      // erase old pos if on the display vertically...
      
      if (oldY>=0 && oldY<(ROWS*100) ) {        
        dots[oldX/100] &= ~ ( 1<<(oldY/100 ) );
      }

      if (starsY[s]>=0 && starsY[s]<(ROWS*100)) {             
        dots[starsX[s]/100] |= ( 1<<(starsY[s]/100 ) );
      }
      
      
      for (int q=-1;q<=1;q++) {

        for(int p=-3+abs(q);p<=3-abs(q);p++) {
        
          dots[ (centerX /100)+p   ] |= _BV((ROWS/2)+q);
        }
      
      }
      
      // Blinking top light
      
      if ( j& 0x10 ) {
       SETDOT( (centerX/100) , (ROWS/2) - 2);
      }
      
      float angle = ((j/2000.0) * 2 * 3.1415 * 20 );
            
      // Make ship spin
            
      int shiplight = ( cos( angle ) * ( sin( angle ) < 0 ? -1 : 1 ) ) * 4  ;
      
      dots[ (centerX /100)+shiplight   ] &= ~_BV(ROWS/2);
            
   }
   
   SYNC(5);
             
  }
  
  
  // Now flying forward...
  
  clear();
  delay(DELAY);
  
  
  // init stars
  for( int s=0;s<STARS;s++) {
    
    // Center with random forward velocity
    starsX[s]=random(COLS*100);
    starsY[s]=random(ROWS*100);
    starsDX[s]=(random(90)+10) * -1;    
  }
  
  shipX = -400;      // Start ship to the left off the screen
  
  for(int j=0;j<2000;j++) {    

      undrawship( (shipX/100) , ROWS/2 );
    
      shipX += 7;
    
      for(int s=0;s<STARS;s++) {
        
        int oldX = starsX[s];
        int oldY = starsY[s];
          
        starsX[s] += starsDX[s];        
        
        // Off display hoazontally?
        if (starsX[s] < 0) {
          starsX[s]=(COLS-1)*100;
          starsY[s]=random(ROWS*100);
          starsDX[s]=(random(90)+10) * -1;                       
        }
         
        // Erase old star pos
        CLRDOT( oldX/100 , oldY/100 );    
        
        // Draw new
        SETDOT( starsX[s]/100 , starsY[s]/100 );
      }
            
      drawship( shipX/100 , ROWS/2 , j/5 );
                    
      SYNC(2);
             
  }
  
  clear();
  delay(DELAY);
  
  
  
  
    
}


void oldloop() {
  
  
   hwiped();
   hwipeu();
   
   vwiper();
   vwipel();
  
  
   allon();
  // put your main code here, to run repeatedly:
  
  PORTB = 0x07;    // Select ROW 8- which is actually a dummy row for selecting the data bit
    
  for( int i=0; i<100; i++) {
    
    
    PORTD |= _BV( 4 );  // Set Data high
    
    // Pulse clock
    
    PORTD |=   _BV(2);    // Clock high
  //  delay(1);
    PORTD &= ~_BV(2);     // Clock low
  //  delay(1);
    
    PORTD &= ~_BV( 4 );  // Set Data low
    
    // Pulse clock
    
    PORTD |=   _BV(2);    // Clock high
 //   delay(1);
    PORTD &= ~_BV(2);     // Clock low
 //   delay(1);
    
  }
  
  for( int p=0; p<100; p++ ) {
    
     // OK, we just pumped alternating rows of data bits. Light em up!
     
     for( int i=0; i<7; i++ ){
       
       PORTB = i;    // Select ROW. This will light an LED if its ROW is on
       delay(100);     
       
     }
     
  }
      
      

}
