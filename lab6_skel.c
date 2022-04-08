// lab6_skel.c 
// Drew Gehrke 
// 11.12.21

//  HARDWARE SETUP:
//  PORTA is connected to the segments of the LED display and to the pushbuttons.
//  PORTA.0 corresponds to segment a, PORTA.1 corresponds to segement b, etc.
//  PORTB bits 0-3 go to the SPI corresponding to SS_n, SCK, MOSI, and MISO
//  PORTB bits 4-6 go to a,b,c inputs of the 74HC138.
//  PORTB bit 7 goes to the PWM transistor base and OE_n on the shifte register for thebar graph.
//  PORTC bit 0 goes to the audio rin and lin
//	PORTD bit 2 goes to the regclk on the bar graph
//	PORTE bit 6 goes to the SHIFT_LD_n on the encoders
//	PORTF bit 7 goes to the ADC value and shifts the 


#define F_CPU 16000000 // cpu speed in hertz 
#define TRISTATEOFF ~(1 << PB5) //Disable the tristate buffer on PORTB
#define F_ALARM 440

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdlib.h>
#include "uart_functions.h"
#include "hd44780.h"
#include "twi_master.h"
#include "lm73_functions.h"
#include "si4734.h"

uint8_t chk_button0();
uint8_t chk_button1();
uint8_t chk_button2();
uint8_t chk_button3();
uint8_t chk_button4();
uint8_t chk_button6();
uint8_t chk_button7();
void segsum(int8_t, int8_t);
void spi_init();
void tcnt0_init();
void tcnt1_init();
void tcnt2_init();
void tcnt3_init();
void adc_init();
void spi_write(uint16_t);
int8_t spi_read();
int8_t enc1_get();
int8_t enc2_get();
void enc_state();
void enc_state_alarm();
void enc_state_radio();
void enc_state_volume();
void inc_time();
void dec_time();
void inc_time_hours();
void dec_time_hours();
void inc_time_alarm();
void dec_time_alarm();
void inc_time_alarm_hours();
void dec_time_alarm_hours();
uint8_t alarm_check();
void brightness_adjust();
void uart_rxtx();
void temp_get();
void radio_reset();
void segsum_radio(uint16_t);
void segsum_volume(uint16_t);

//holds data to be sent to the segments. logic zero turns segment on
uint8_t segment_data[5]; 
//decimal to 7-segment LED display encodings, logic "0" turns on segment
uint8_t dec_to_7seg[12]; 

//decimal value holding number of hours to be sent to the 7-seg display
int8_t hours = 0;

//decimal value holding number of minutes to be sent to the 7-seg display
int8_t minutes = 0;

//decimal value holding number of seconds to be used for timing
uint8_t seconds = 0;

//decimal value holding number of hours to be sent to the 7-seg display
int8_t alarm_hours = 0;

//decimal value holding number of minutes to be sent to the 7-seg display
int8_t alarm_minutes = 0;

//binary value which indicates the mode of operation. 
//Modes are as follows:
//	0x00 - standard clock
//	0x01 - change time
//	0x02 - set alarm time
//	0x80 - alarm set
volatile uint8_t mode = 0x00;

volatile uint8_t enc_value = 0;			//value of the encoders read by the SPI to be used in the state machine.
volatile uint8_t colon_blink = 0x04;	//binary value acting as a boolean to indicate if the colon is on or off.
uint8_t alarm_enable = FALSE;			//boolean value indicating if the alarm is enabled
uint8_t snooze_enable = FALSE;			//boolean value indicating if the snooze is enabled
uint8_t snooze_timer = 0;				//decimal value holding number of seconds for snooze.
uint16_t adc_result = 0; 				//16 bit value of adc result
volatile uint8_t  rcv_rdy;
char rx_char; 
char lcd_str_array[16];    //string from remote temp sensor
uint8_t send_seq=0;        //transmit sequence number
char lcd_string[3];        //holds value of sequence number
char lcd_string_array_l1[16]; 	//holds string to send to LCD line1
char lcd_string_array_l2[16]; 	//holds string to send to LCD line2
uint8_t i;                     //general purpose index

extern uint8_t lm73_wr_buf[10]; 
extern uint8_t lm73_rd_buf[10]; 

uint8_t radio_enable = FALSE;
volatile uint8_t STC_interrupt;     //indicates tune or seek is done

uint16_t current_fm_freq;
uint16_t  current_vol;

uint8_t  si4734_wr_buf[9];
uint8_t  si4734_rd_buf[9];
uint8_t  si4734_tune_status_buf[8];
char freq_string[8];
uint8_t freq_changed = FALSE;

//***********************************************************************************
//***********************************************************************************
uint8_t main() {
	uint8_t digit_out = 0x00;	// Keep track of the digit being output on PORTB
	uint8_t num_out = 0;

	DDRB = 0xF0; 					// Set PORTB bits 4-7 to outputs
	DDRC |= (1<<PC0) | (1<<PC2);	// Set PORTC bits 0 & 2 to output
	DDRD = (1<<PD2);				// Set PORTD bit 2 as output
	DDRE |= (1<<PE2) | (1<<PE3) | (1<<PE6);		// Set PORTE bits 2, 3 & 6 as output
	DDRF |= 0x08;					// LCD strobe bit

	spi_init();						// Initialize SPI
	tcnt0_init();					// Initialize Timer/Counter0
	tcnt1_init();					// Initialize Timer/Counter1
	tcnt2_init();					// Initialize Timer/Counter2
	tcnt3_init();					// Initialize Timer/Counter3
	adc_init();						// Initialize the ADC
	lcd_init();						// Initialize the LCD
	uart_init();					// Initialize the UART
	init_twi();						// Initialize the TWI (twi_master.h)
	sei();							// Enable global interrupts

	clear_display();
	cursor_home();

	PORTE |= 0x04;

	// Configure LCD screen for initial values
	strcpy(lcd_string_array_l1, "ALARM:N");
	strcat(lcd_string_array_l1, " S:");
	current_fm_freq = 9990;
	string2lcd(lcd_string_array_l1);
	lcd_int16(current_fm_freq, 2, 2, 0);

	current_vol = 0x0003;

	// Number masks for the LED Display. Logic "0" turns on the segment
	dec_to_7seg[0] = 0b11000000;
	dec_to_7seg[1] = 0b11111001;
	dec_to_7seg[2] = 0b10100100;
	dec_to_7seg[3] = 0b10110000;
	dec_to_7seg[4] = 0b10011001;
	dec_to_7seg[5] = 0b10010010;
	dec_to_7seg[6] = 0b10000010;
	dec_to_7seg[7] = 0b11111000;
	dec_to_7seg[8] = 0b10000000;
	dec_to_7seg[9] = 0b10010000;
	dec_to_7seg[10] = 0b10001000;

	//set LM73 mode for reading temperature by loading pointer register
	lm73_wr_buf[0] = LM73_PTR_TEMP;				//load lm73_wr_buf[0] with temperature pointer address
	twi_start_wr(LM73_ADDRESS, lm73_wr_buf, 2);	//start the TWI write process
	_delay_ms(2);    							//wait for the xfer to finish

	while(1){
		_delay_ms(2);	 	// Loop delay for debouncing
		DDRA = 0x00;	 	// Set PORTA to input
		PORTA = 0xFF;	 	// Enable PU Resistors on PORTA
		PORTB = 0xD0;		// Enable tristate buffer
		PORTE = (1<<PE0) | (1<<PE1);	// Enable PU Resistor for TWI CLK and DATA

		_delay_ms(1);

		brightness_adjust();	// Adjust the brightness based on the ambient light

		for(int i=0; i<4; i++) {	// Blank out all LED digits
			segment_data[i] = 0b11111111;
		}

		if (chk_button0()) {mode ^= (1 << 0);}		//Time set mode
		if (chk_button1()) {mode ^= (1 << 1);}		//Alarm time set mode
		if (chk_button2()) {						//Alarm set 
			mode ^= (1 << 7);
			alarm_enable = TRUE;
		}
		if (chk_button3()) {						//Radio enable
			mode ^= (1 << 3);
			radio_enable ^= (1<<0);
			if (radio_enable) {
				radio_reset();
				fm_pwr_up();
				while(twi_busy()){};
				STC_interrupt = FALSE;
				current_fm_freq = 9990;
				fm_tune_freq();
				while(twi_busy()){};
			}
			else if (radio_enable == 0) {radio_pwr_dwn();}
		}
		if (chk_button4()) {mode ^= (1<<4);}
		if (chk_button6()) {mode ^= (1<<6);}

		PORTB &= TRISTATEOFF;		// Disable tristate buffer

		spi_write(mode);			// Output the inc/dec mode to bargraph
		enc_value = spi_read();		// Read the value of the encoders from SPI

		if ((mode & 0x01) == 0x01) {
			enc_state();							// Check encoder state machine to change value for clock
			segsum(hours, minutes);					// Output clock time
		}
		else if ((mode & 0x02) == 0x02) {
			enc_state_alarm();						// Check encoder state machine to change value for alarm
			segsum(alarm_hours, alarm_minutes);		// Output alarm time
		}
		else if ((mode & 0x10) == 0x10) {
			enc_state_volume();
			segsum_volume(current_vol);
		}
		else if ((mode & 0x40) == 0x40) {
			enc_state_radio();						// Check encoder state machine to change value for radio
			segsum_radio(current_fm_freq);
		}
		else if ((mode & 0x80) == 0x80) {
			
			if (alarm_check() && alarm_enable) {
				snooze_enable = TRUE;
				TCCR1B |= (1<<CS10);
			}

			if(snooze_timer > 10) {
				TCCR1B |= (1<<CS10);
			}

			if (chk_button7()) {
				snooze_timer = 0;
				alarm_enable = FALSE;
				TCCR1B &= ~(1<<CS10);
			}

			segsum(hours, minutes);			// Set segment_data to proper numbers from dec_to_7seg
		}
		else {
			segsum(hours, minutes);
			alarm_enable = FALSE;
			snooze_enable = FALSE;
		}

		PORTB = digit_out;		// PORTB set to the digit for the decoder select
		DDRA = 0xFF;			// PORTA set to output

		PORTA = segment_data[num_out]; 	// Output number from segment_data to the display

		digit_out += 0x10;			// Increment which LED digit to display
		if (digit_out > 0x40)		// Only 5 digits, if more, reset
			digit_out = 0;

		num_out++;
		if (num_out > 4)
			num_out = 0;
  }//while
}//main
//******************************************************************************

//******************************************************************************
// 							  TIMER0_OVF_vect
// When the TCNT0 overflow occurs, the count_7ms variable is incremented. 
// TCNT0 interrupts come at 7.8125ms intervals
//

ISR(TIMER0_OVF_vect){
	static uint8_t count_7ms = 0;	//Holds the 7ms tick count in binary

	count_7ms++;	//Increment count every 7.8125 ms
	if ((count_7ms % 128) == 0) {
		colon_blink ^= 0x03;
		seconds++;
		if (seconds == 60) {
			inc_time();
			seconds = 0;
		}

		if (snooze_enable) {snooze_timer++;}
		if (alarm_enable) {lcd_string_array_l1[6] = 'Y';}
		else {lcd_string_array_l1[6] = 'N';}
		if (freq_changed) {
			clear_display();
			freq_changed = FALSE;
		}

		temp_get();
		uart_rxtx();
		line1_col1();
		cursor_home();
		string2lcd(lcd_string_array_l1);
		lcd_int16(current_fm_freq, 2, 2, 0);
		line2_col1();
		cursor_home();
		string2lcd(lcd_string_array_l2);
	}
}//ISR(TIMER0_OVF_vect)

//******************************************************************************
// 							  TIMER1_CAPT_vect
// Alternates PORTC high and low to simulate a frequency generator for the audio
// output.
//

ISR(TIMER1_CAPT_vect) {	
	PORTC ^= (1 << PC2);
}

//******************************************************************************
// 							  USART0_RX_vect
// USART receiver ISR which will communicate the temperature at its position and
// send it via the USART bus.
//

ISR(USART0_RX_vect){
static  uint8_t  i;
  rx_char = UDR0;              //get character
  lcd_str_array[i++]=rx_char;  //store in array 
 //if entire string has arrived, set flag, reset index
  if(rx_char == '\0'){
    rcv_rdy=1; 
    lcd_str_array[--i]  = (' ');     //clear the count field
    lcd_str_array[i+1]  = (' ');
    lcd_str_array[i+2]  = (' ');
    i=0;  
  }
}

//******************************************************************************
//                          External Interrupt 7 ISR                     
// Handles the interrupts from the radio that tells us when a command is done.
// The interrupt can come from either a "clear to send" (CTS) following most
// commands or a "seek tune complete" interrupt (STC) when a scan or tune command
// like fm_tune_freq is issued. The GPIO2/INT pin on the Si4734 emits a low
// pulse to indicate the interrupt. I have measured but the datasheet does not
// confirm a width of 3uS for CTS and 1.5uS for STC interrupts.
//
// I am presently using the Si4734 so that its only interrupting when the 
// scan_tune_complete is pulsing. Seems to work fine. (12.2014)
//
// External interrupt 7 is on Port E bit 7. The interrupt is triggered on the
// rising edge of Port E bit 7.  The i/o clock must be running to detect the
// edge (not asynchronouslly triggered)
//******************************************************************************
ISR(INT7_vect){STC_interrupt = TRUE;}
//******************************************************************************

//******************************************************************************
//                            chk_button 0-7                                      
//Checks the state of the button number which was called. It shifts in ones till   
//the button is pushed. Function returns a 1 only once per debounced button    
//push so a debounce and toggle function can be implemented at the same time.  
//Adapted from Ganssel's "Guide to Debouncing"            
//Expects active low pushbuttons on PINA port.  Debounce time is determined by 
//external loop delay times 12. 
//

uint8_t chk_button0() {
	static uint16_t state = 0; //holds present state
	state = (state << 1) | (! bit_is_clear(PINA, 0)) | 0xE000;
	if (state == 0xF000) return 1;
	return 0;
}

uint8_t chk_button1() {
	static uint16_t state = 0; //holds present state
	state = (state << 1) | (! bit_is_clear(PINA, 1)) | 0xE000;
	if (state == 0xF000) return 1;
	return 0;
}

uint8_t chk_button2() {
	static uint16_t state = 0; //holds present state
	state = (state << 1) | (! bit_is_clear(PINA, 2)) | 0xE000;
	if (state == 0xF000) return 1;
	return 0;
}

uint8_t chk_button3() {
	static uint16_t state = 0; //holds present state
	state = (state << 1) | (! bit_is_clear(PINA, 3)) | 0xE000;
	if (state == 0xF000) return 1;
	return 0;
}

uint8_t chk_button4() {
	static uint16_t state = 0; //holds present state
	state = (state << 1) | (! bit_is_clear(PINA, 4)) | 0xE000;
	if (state == 0xF000) return 1;
	return 0;
}

uint8_t chk_button6() {
	static uint16_t state = 0; //holds present state
	state = (state << 1) | (! bit_is_clear(PINA, 6)) | 0xE000;
	if (state == 0xF000) return 1;
	return 0;
}

uint8_t chk_button7() {
	static uint16_t state = 0; //holds present state
	state = (state << 1) | (! bit_is_clear(PINA, 7)) | 0xE000;
	if (state == 0xF000) return 1;
	return 0;
}

//******************************************************************************

//***********************************************************************************
//                                   segsum                                    
// Takes two 8-bit binary input values and places the appropriate equivalent 4 digit 
// BCD segment code in the array segment_data for display.                       
// array is loaded at exit as:  |digit3|digit2|colon|digit1|digit0|
//

void segsum(int8_t hour, int8_t mins) {
	uint8_t digit;
	uint8_t hours_temp = hour;
	uint8_t mins_temp = mins;

	// Adapted from https://www.log2base2.com/c-examples/loop/split-a-number-into-digits-in-c.html
	// Find the digit number and put the corresponding binary mask into segment_data
	
	for(int i = 0; i != 2; i++) {
		digit = mins_temp % 10;
		segment_data[i] = dec_to_7seg[digit];
		mins_temp /= 10;
	}
	
	for (int i = 3; i != 5; i++) {
		digit = hours_temp % 10;
		segment_data[i] = dec_to_7seg[digit];
		hours_temp /= 10;
	}

	// Blink the colon every second
	segment_data[2] = colon_blink;
	

}//segsum

//**********************************************************************************
//					spi_init
// Initializes the SPI port on the mega128. Does not do any further external device
// specific initializations.
//

void spi_init() {
	DDRB |= (1<<PB0) | (1<<PB1) | (1<<PB2);	// Turn on SS, MOSI, SCLK (SS is output)
	SPCR = (1<<SPE) | (1<<MSTR);		// SPI enabled, master mode enable
	SPSR = (1<<SPI2X);			// Run at i/o clock/2
}//spi_init

//**********************************************************************************
//					tcnt0_init
// Initializes Timer/Counter0 on the mega128. Does not do any further external device
// specific initializations.
//

void tcnt0_init() {
	ASSR |= (1<<AS0);			// External OSC TOSC
	TIMSK |= (1<<TOIE0);		// Enable TCNT0 overflow interrupt
	TCCR0 |= (1<<CS00);			// Normal mode, no prescale
}//tcnt0_init

//**********************************************************************************
//					tcnt1_init
// Initializes Timer/Counter1 on the mega128. Does not do any further external device
// specific initializations.
//

void tcnt1_init() {
	//CTC mode, IRC1 holds value of TOP
	//no prescale

	TIMSK |= (1<<TICIE1);
	TCCR1B |= (1<<WGM12) | (1<<WGM13);
	TCCR1C = 0x00;
	ICR1 = 0x88FF;
}//tcnt1_init

//**********************************************************************************
//					tcnt2_init
// Initializes Timer/Counter2 on the mega128. Does not do any further external device
// specific initializations.
//

void tcnt2_init() {
	ASSR |= (1<<AS0);			// External OSC TOSC
	TCCR2 |= (1<<WGM21) | (1<<WGM20) | (1<<COM21) | (1<<CS20);	// Fast PWM, no prescale

}//tcnt2_init

//**********************************************************************************
//					tcnt3_init
// Initializes Timer/Counter3 on the mega128. Does not do any further external device
// specific initializations.
//

void tcnt3_init() {
	//fast PWM, set on match, clear at top, OCR3A holds value of TOP
	//no prescale

	TCCR3A |= (1<<COM3A1) | (1<<WGM31) | (1<<WGM30);
	TCCR3B |= (1<<CS30) | (0<<WGM33) | (1<<WGM32);
	TCCR3C = 0x00;
	OCR3A = 0x00;
}//tcnt3_init

//**********************************************************************************
//					adc_init
// Initializes the ADC on the mega128. Does not do any further external device
// specific initializations.
//

void adc_init() {
	DDRF |= (0<<PF7);
	PORTF |= (1<<PF7);
	ADMUX |= (1<<MUX2) | (1<<MUX1) | (1<<MUX0) | (1<<REFS0);
	ADCSRA |= (1<<ADEN) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);
}//adc_init

//***********************************************************************************
//					spi_write
// SPI output function. Displays the number being incremented by the encoders on the 
// bar graph.
//

void spi_write(uint16_t num) {
	SPDR = num;
	while(bit_is_clear(SPSR, SPIF)){}
	PORTD |= (1 << PD2);
	PORTD &= ~(1 << PD2);
}//spi_write

//***********************************************************************************
//					spi_read
// SPI input function. Read in the values from the encoder and determine whether to
// increment or decrement the count.
//

int8_t spi_read() {
	PORTE |= (1<<PE6);
	SPDR = 0xFF;
	while(bit_is_clear(SPSR, SPIF)){}
	PORTE &= ~(1<<PE6);
	return (SPDR);
}//spi_read

//***********************************************************************************
//					enc1_get
// Check to see if encoder 1 has been changed based on a state machine. Adapted
// from lecture slides provided from Vlad: encoder_use.pdf
//

int8_t enc1_get() {
	int8_t return_val = -1;
	static uint8_t old_a = 0;
	static uint8_t old_b = 0;
	static uint8_t new_a = 0;
	static uint8_t new_b = 0;
	static int8_t count = 0;

	new_a = (enc_value & 0x01);
	new_b = ((enc_value & 0x02) >> 1);

	//spi_write((new_a | new_b));
	if ((new_a != old_a) || (new_b != old_b)) {
		if ((new_a == 0 && new_b == 0)) {
			if (old_a == 1) {count++;}
			else {count--;}
		}
		else if ((new_a == 0) && (new_b == 1)) {
			if (old_a == 0) {count++;}
			else {count--;}
		}
		else if ((new_a == 1) && (new_b == 1)) {
			if (old_a == 0) {
				if (count == 3) {return_val = 1;}
			}
			else {
				if (count == (-3)) {return_val = 0;}
			}
			count = 0;
		}
		else if ((new_a == 1) && (new_b == 0)) {
			if (old_a == 1) {count++;}
			else {count--;}
		}
		old_a = new_a;
		old_b = new_b;
	}
	return (return_val);
 
}//enc1_get

//***********************************************************************************
//					enc2_get
// Check to see if encoder 2 has been changed based on a state machine. Adapted
// from lecture slides provided from Vlad: encoder_use.pdf
//

int8_t enc2_get() {
	int8_t return_val = -1;
	static uint8_t old_a;
	static uint8_t old_b;
	static uint8_t new_a = 0;
	static uint8_t new_b = 0;
	static int8_t count = 0;

	new_a = ((enc_value & 0x04) >> 2);
	new_b = ((enc_value & 0x08) >> 3);

	if ((new_a != old_a) || (new_b != old_b)) {
		if ((new_a == 0 && new_b == 0)) {
			if (old_a == 1) {count++;}
			else {count--;}
		}
		else if ((new_a == 0) && (new_b == 1)) {
			if (old_a == 0) {count++;}
			else {count--;}
		}
		else if ((new_a == 1) && (new_b == 1)) {
			if (old_a == 0) {
				if (count == 3) {return_val = 1;}
			}
			else {
				if (count == (-3)) {return_val = 0;}
			}
			count = 0;
		}
		else if ((new_a == 1) && (new_b == 0)) {
			if (old_a == 1) {count++;}
			else {count--;}
		}
		old_a = new_a;
		old_b = new_b;
	}
	return (return_val);
 
}//enc2_get

//***********************************************************************************
//					enc_state
// Check the state of the encoders to determine if it will increment or decrement. 
// Uses the enc#_get() state machines to determine if it was an increment or a 
// decrement. Changes the values for the clock.
//

void enc_state() {
	int8_t enc1 = enc1_get();
	int8_t enc2 = enc2_get();

	if (enc1 != -1) {
		if (enc1 == 0) {
			inc_time_hours();
		}
		else if (enc1 == 1) {
			dec_time_hours();
		}
	}
	
	if (enc2 != -1) {
		if (enc2 == 0) {
			inc_time();
		}
		else if (enc2 == 1) {
			dec_time();
		}
	}
}//enc_state

//***********************************************************************************
//					enc_state_alarm
// Check the state of the encoders to determine if it will increment or decrement. 
// Uses the enc#_get() state machines to determine if it was an increment or a 
// decrement. Changes the values for the alarm setting.
//

void enc_state_alarm() {
	int8_t enc1 = enc1_get();
	int8_t enc2 = enc2_get();

	if (enc1 != -1) {
		if (enc1 == 0) {
			inc_time_alarm_hours();
		}
		else if (enc1 == 1) {
			dec_time_alarm_hours();
		}
	}
	
	if (enc2 != -1) {
		if (enc2 == 0) {
			inc_time_alarm();
		}
		else if (enc2 == 1) {
			dec_time_alarm();
		}
	}
}//enc_state_alarm

//***********************************************************************************
//					enc_state_radio
// Check the state of the encoders to determine if it will increment or decrement. 
// Uses the enc#_get() state machines to determine if it was an increment or a 
// decrement. Changes the values for the radio frequency and tunes the radio.
//

void enc_state_radio() {
	int8_t enc1 = enc1_get();
	int8_t enc2 = enc2_get();

	if (enc1 != -1) {
		if (enc1 == 0) {
			current_fm_freq+= 20;
		}
		else if (enc1 == 1) {
			current_fm_freq-= 20;
		}
		freq_changed = TRUE;
		fm_tune_freq();
	}
	
	if (enc2 != -1) {
		if (enc2 == 0) {
			current_fm_freq+= 20;
		}
		else if (enc2 == 1) {
			current_fm_freq-= 20;
		}
		freq_changed = TRUE;
		fm_tune_freq();
	}

	if (current_fm_freq > 10790) {
		current_fm_freq = 8810;
		freq_changed = TRUE;
		fm_tune_freq();
	}
	else if (current_fm_freq < 8810) {
		current_fm_freq = 10790;
		freq_changed = TRUE;
		fm_tune_freq();
	}

}//enc_state_radio

//***********************************************************************************
//					enc_state_volume
// Check the state of the encoders to determine if it will increment or decrement. 
// Uses the enc#_get() state machines to determine if it was an increment or a 
// decrement. Changes the values for the current_volume variable.
//

void enc_state_volume() {
	int8_t enc1 = enc1_get();
	int8_t enc2 = enc2_get();

	if (enc1 != -1) {
		if (enc1 == 0) {
			current_vol+= 10;
		}
		else if (enc1 == 1) {
			current_vol-= 10;
		}
		if (current_vol > 670) {current_vol = 670;}
		if (current_vol < 0) {current_vol = 0;}

		OCR3A = current_vol;
	}
	
	if (enc2 != -1) {
		if (enc2 == 0) {
			current_vol+= 10;
		}
		else if (enc2 == 1) {
			current_vol-= 10;
		}
		if (current_vol > 670) {current_vol = 670;}
		if (current_vol < 0) {current_vol = 0;}

		OCR3A = current_vol;
	}
	
}//enc_state_volume

//***********************************************************************************
//					inc_time()
// Increment the minutes GV and check if the hours needs to be incremented as well
//

void inc_time() {
	minutes++;

	if (minutes == 60) {
		hours++;
		minutes = 0;

		if (hours == 24)
		hours = 0;
	}
}//inc_time

//***********************************************************************************
//					dec_time()
// Decrement the minutes GV and check if the hours needs to be decremented as well
//

void dec_time() {
	minutes--;
	if (minutes < 0) {
		minutes = 59;
		hours--;

		if (hours < 0)
			hours = 23;
	}
}//dec_time

//***********************************************************************************
//					inc_time_hours()
// Increment the hours GV only.
//

void inc_time_hours() {
	hours++;

	if (hours == 24)
		hours = 0;
}//inc_time_hours

//***********************************************************************************
//					dec_time_hours()
// Decrement the hours GV only.
//

void dec_time_hours() {
	hours--;

	if (hours < 0)
		hours = 23;
}//dec_time_hours

//***********************************************************************************
//					inc_time_alarm()
// Increment the minutes GV and check if the hours needs to be incremented as well
//

void inc_time_alarm() {
	alarm_minutes++;

	if (alarm_minutes == 60) {
		alarm_hours++;
		alarm_minutes = 0;

		if (alarm_hours == 24)
		alarm_hours = 0;
	}
}//inc_time_alarm

//***********************************************************************************
//					dec_time_alarm()
// Decrement the minutes GV and check if the hours needs to be decremented as well
//

void dec_time_alarm() {
	alarm_minutes--;
	if (alarm_minutes < 0) {
		alarm_minutes = 59;
		alarm_hours--;

		if (alarm_hours < 0)
			alarm_hours = 23;
	}
}//dec_time_alarm

//***********************************************************************************
//					inc_time_alarm_hours()
// Increment the alarm_hours GV only.
//

void inc_time_alarm_hours() {
	alarm_hours++;

	if (alarm_hours == 24)
		alarm_hours = 0;
}//inc_time_alarm_hours

//***********************************************************************************
//					dec_time_alarm_hours()
// Decrement the alarm_hours GV only.
//

void dec_time_alarm_hours() {
	alarm_hours--;

	if (alarm_hours < 0)
		alarm_hours = 23;
}//dec_time_alarm_hours

//***********************************************************************************
//					alarm_check()
// Check the time to see if the current time is the same as the alarm time. Returns 
// a 1 on a match and 0 if not a match.
//

uint8_t alarm_check() {
	uint8_t return_val = 0;
	
	if (hours == alarm_hours) {
		if (minutes == alarm_minutes) {
			return_val = 1;
		}
	}

	return (return_val);
}//alarm_check

//***********************************************************************************
//					brightness_adjust()
// Adjust the brightness of all the LEDs on the alarm clock based on ambient light. 
// Takes in the ADC value from a photoresistor and sets the TOP of the PWM signal to
// that value.
//

void brightness_adjust() {
	ADCSRA |= (1<<ADSC);				//Poke ADSC bit and start conversion
	while(bit_is_clear(ADCSRA, ADIF));	//Spin while interrupt flag not set
	ADCSRA |= (1<<ADIF);				//Clear ADIF flag
	
	adc_result = ADC;

	OCR2 = ~(adc_result);				//Sets the TOP value for PORTB bit 7 to adjust
}//brightness_adjust

//***********************************************************************************
//					uart_rxtx()
// Uses the UART protocol to receive and transmit from the other clock.
//

void uart_rxtx() {
	
//**************  start rcv portion ***************
    if(rcv_rdy==1){ 
		lcd_string_array_l2[4] = ' ';
		lcd_string_array_l2[5] = lcd_str_array[0];
		lcd_string_array_l2[6] = lcd_str_array[1];
        rcv_rdy=0;
    }//if 
	else {
		line2_col1();
		cursor_home();
		fill_spaces();
	}
//**************  end rcv portion ***************
}//uart_rxtx

//***********************************************************************************
//					temp_get()
// Get the current temperature from the LM73 temperature sensor and output to the
// LCD. This will save a value into the lcd_string_array global variable to be used
// elsewhere.
//

void temp_get() {
	int16_t lm73_temp = 0;
	
    twi_start_rd(LM73_ADDRESS, lm73_rd_buf, 2); //read temperature data from LM73 (2 bytes) 
    _delay_ms(2);		//wait for it to finish
    lm73_temp = lm73_rd_buf[0]; 	//save high temperature byte into lm73_temp
    lm73_temp = (lm73_temp << 8);  	//shift it into upper byte
    lm73_temp |= lm73_rd_buf[1];	//"OR" in the low temp byte to lm73_temp
    itoa((lm73_temp>>7), lcd_string_array_l2, 10); //convert to string in array with itoa() from avr-libc
	lcd_string_array_l2[2] = 'C';
}//temp_get

//***********************************************************************************
//					radio_reset()
// Does a hardware reset on the radio module.
//

void radio_reset() {
	PORTE &= ~(1<<PE7); //int2 initially low to sense TWI mode
	DDRE  |= 0x80;      //turn on Port E bit 7 to drive it low
	PORTE |=  (1<<PE2); //hardware reset Si4734 
	_delay_us(200);     //hold for 200us, 100us by spec         
	PORTE &= ~(1<<PE2); //release reset 
	_delay_us(30);      //5us required because of my slow I2C translators I suspect
                    	//Si code in "low" has 30us delay...no explaination given
	DDRE  &= ~(0x80);   //now Port E bit 7 becomes input from the radio interrupt
}//radio_reset

//***********************************************************************************
//					segsum_radio()
// Takes a 16-bit binary input value and places the appropriate equivalent 4 digit 
// BCD segment code in the array segment_data for display.                       
// array is loaded at exit as:  |digit3|digit2|colon|digit1|digit0|. Enables the 
// decimal point for the tenths place.
//

void segsum_radio(uint16_t sum) {
	uint8_t digit;

	// Adapted from https://www.log2base2.com/c-examples/loop/split-a-number-into-digits-in-c.html
	// Find the digit number and put the corresponding binary mask into segment_data
	
	int i=0;
	sum /= 10;
	do {
		digit = sum % 10;			// Extract the first digit from sum
		segment_data[i] = dec_to_7seg[digit];	// Store the number to be sent to the LED display
		sum /= 10;				// Divide by ten to remove the first digit
		i++;
	} while (sum != 0);

	if (segment_data[2] > 0) {
		segment_data[4] = segment_data[3];
		segment_data[3] = segment_data[2];
		segment_data[2] = 0b11111111;
	}
	segment_data[1] &= ~(1<<7);
}//segsum_radio

//***********************************************************************************
//					segsum_radio()
// Takes a 16-bit binary input value and places the appropriate equivalent 4 digit 
// BCD segment code in the array segment_data for display.                       
// array is loaded at exit as:  |digit3|digit2|colon|digit1|digit0|.
//

void segsum_volume(uint16_t sum) {
	uint8_t digit;

	// Adapted from https://www.log2base2.com/c-examples/loop/split-a-number-into-digits-in-c.html
	// Find the digit number and put the corresponding binary mask into segment_data
	
	int i=0;
	sum /= 10;
	do {
		digit = sum % 10;			// Extract the first digit from sum
		segment_data[i] = dec_to_7seg[digit];	// Store the number to be sent to the LED display
		sum /= 10;				// Divide by ten to remove the first digit
		i++;
	} while (sum != 0);

	if (segment_data[2] > 0) {
		segment_data[4] = segment_data[3];
		segment_data[3] = segment_data[2];
		segment_data[2] = 0b11111111;
	}
}//segsum_volume
//***********************************************************************************
