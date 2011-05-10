// http://orpex.blogspot.com

// ATmega168
// PIN DEFINITIONS:
// PB1-4 ROW DRIVERS (0-3)
// PC0-5,PD2-3: COLUMN DRIVERS (0-7)
// PD7 BUTTON
#define F_CPU 14745600
#define ROWS 4
#define COLS 16
#define INVPOL

#include <stdio.h>
#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

volatile uint8_t la_row, real_row;
volatile uint8_t la_data[COLS];

inline uint8_t ledarray_get(uint8_t i, uint8_t j) 
{
  if(i < ROWS && j < COLS) 
  {
    if((la_data[j] & (1<<i)) != 0) 
	{
      return 1;
    } 
  } 
  return 0;
}


inline void ledarray_set(uint8_t i, uint8_t j, uint8_t onoff) 
{
  if(i < ROWS && j < COLS) 
  {
    if(onoff) 
	{
      la_data[j] |= (1<<i);
    } 
	else 
	{
      la_data[j] &= ~(1<<i);
    }
  }
}

//sense variable indicates direction of LED: sense == 1 indicates COL wire must be
//high for LED to turn on. sense == 0, COL wire must be low to turn LED on
inline void ledarray_set_columndriver(uint8_t j, uint8_t onoff, uint8_t sense) 
{
  // cols 0-5: PC0-5
  // cols 6-7: PD2-3
  if(j < 6) 
  {
    if(onoff)
	{ //led on
      DDRC |= (1 << (PC0 + j));
	  #ifndef INVPOL
      if(sense) 
	  #else
	  if(!sense)
	  #endif
	  {
        PORTC |= (1 << (PC0 + j));
      } 
	  else 
	  {
        PORTC &= ~(1<< (PC0 + j));
      }
    } 
	else 
	{ // led off, pins to high impedance
      DDRC &= ~(1 << (PC0 + j));
      //PORTC &= ~(1 << (PC0 + j));
    }
  } 
  else 
  {
    if(onoff)
	{ //led on
      DDRD |= (1 << (PD2 + (j-6)));
	  #ifndef INVPOL
      if(sense) 
	  #else
	  if(!sense)
	  #endif
	  {
        PORTD |= (1 << (PD2 + (j-6)));
      } 
	  else 
	  {
        PORTD &= ~(1 << (PD2 + (j-6)));
      }
    } 
	else 
	{ // led off, pins to high impedance
      DDRD &= ~(1 << (PD2 + j));
      //PORTD &= ~(1 << (PD2 + j));
    }
  }
}

SIGNAL(SIG_OUTPUT_COMPARE1A) 
{
  // turn off old row driver
  DDRB &= ~(1 << (PB1 + real_row));
  //PORTB &= ~(1 << (PB1 + real_row));
  //ledarray_all_off();

  // increment row number
  if (++la_row == 2*ROWS) la_row = 0;

  // set column drivers appropriately
  uint8_t j;
  if (la_row%2 == 0) 
  {
    // even la_row number: fill even columns
    real_row = la_row / 2;
    for(j=0; j<COLS/2; j++) 
	{
      ledarray_set_columndriver(j, ledarray_get(real_row, 2*j), 1);
    }
    // activate row driver SINK
	#ifndef INVPOL
    PORTB &= ~(1 << (PB1 + real_row));
	#else
    PORTB |= (1 << (PB1 + real_row));
	#endif
    DDRB |= (1 << (PB1 + real_row));
  } 
  else 
  {
    // odd la_row number: fill odd columns
    real_row = (la_row-1)/2;
    for(j=0; j<COLS/2; j++) 
	{
      ledarray_set_columndriver(j, ledarray_get(real_row, 2*j + 1), 0);
    }
    // activate row driver SOURCE
	#ifndef INVPOL
    PORTB |= (1 << (PB1 + real_row));
	#else
    PORTB &= ~(1 << (PB1 + real_row));
	#endif
    DDRB |= (1 << (PB1 + real_row));
  }  
}

void ledarray_init() 
{
  // http://www.et06.dk/atmega_timers
  TIMSK1 = _BV(OCIE1A); // compare1a interrupt
  TCCR1B = _BV(CS10) | _BV(WGM12); // ctc mode with CK/256 prescale from 8 MHz (default fuses)
  OCR1A = 33333; // compare1a value makes it 240 Hz per 2 rows or 30 fps!
  
  // outputs (set row drivers high for off)
  DDRC &= ~( (1<<PC0) | (1<<PC1) | (1<<PC2) | (1<<PC3) | (1<<PC4) | (1<<PC5) );
  DDRD &= ~( (1<<PD2) | (1<<PD3) );
  DDRB &= ~( (1<<PB1) | (1<<PB2) | (1<<PB3) | (1<<PB4) );
}

void powerup_sequence() 
{
  uint8_t j;
  
  for(j=0; j<COLS; j++) 
  {
    ledarray_set(0, j, 1);
    _delay_ms(262);
  }
}

void back_inner_c() 
{
  uint8_t j;
  
  for(j=0; j<COLS; j++) 
  {
    ledarray_set(1, j, 1);
  }
}

void ledarray_testpattern() 
{
  uint8_t i, j;
  for(i=0;i<ROWS;i++) 
  {
    for(j=0;j<COLS;j++) 
	{
      ledarray_set(i,j, 0);
    }
  }
  
  while (1)
  {
	for(i=0;i<ROWS;i++) 
	{
	  for(j=0;j<COLS;j++) 
	  {
		ledarray_set(i,j, 1 - ledarray_get(i,j));
		_delay_ms(60);
	  }
	}
  }
}

int main() 
{
  ledarray_init();
  
  // activate interrupts
  sei();

  DDRD &= ~(1<<PD7); // set PD7 as input
  PORTD |= (1<<PD7); // turn on internal pull up resistor for PD7

  powerup_sequence(); // front inner-C

  if (PIND & (1<<PD7))
  {
	_delay_ms(1049);
	back_inner_c();
  }
  else
  {
	ledarray_testpattern();
  }
  
  while (1)
  {
    while (PIND & (1<<PD7))
    {
      // loop waiting for button press
    }
	_delay_ms(150);
    while (!(PIND & (1<<PD7)))
    {
      // loop waiting for button release
    }
	
	// blade activation
    uint16_t j, r = 4900,cyc=0;
    for(j=0; j<COLS; j++) 
    {
	  // all on
      ledarray_set(2, j, 1);
      ledarray_set(3, j, 1);
    }
	_delay_ms(300);
    while ((PIND & (1<<PD7)))
    {
      // loop waiting for button press
      for(j=0; j<COLS; j++) 
      {
        ledarray_set(2, j, (j%3!=cyc));
        ledarray_set(3, j, (j%3!=cyc));
      }
	  _delay_us(r);
	  if (++r > 5300) r = 5300;
	  if (++cyc==3) cyc=0;
    }
	
	// blade deactivation
    for(j=0; j<COLS; j++) 
    {
	  // all off
      ledarray_set(2, j, 0);
      ledarray_set(3, j, 0);
    }
	_delay_ms(300);
    while (!(PIND & (1<<PD7)))
    {
      // loop waiting for button release
    }
	
  }
  
  return 0;
}

