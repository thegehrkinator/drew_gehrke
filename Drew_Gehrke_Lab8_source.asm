;***********************************************************
;*
;*	Drew_Gehrke_Lab8_source
;*
;*	This program will allow the user to send a morse code 
;*	transmission via the LCD Display and utilizes the TCNT1
;*	for delaying the LED which is the actual transmission.
;*
;***********************************************************
;*
;*	 Author: Drew Gehrke
;*	   Date: 3/9/2021
;*
;***********************************************************

.include "m128def.inc"			; Include definition file

;***********************************************************
;*	Internal Register Definitions and Constants
;***********************************************************
.def	mpr = r16				; Multipurpose register is
								; required for LCD Driver
.def	morse_byte1 = r20		; First byte of morse code transmit
.def	morse_byte2 = r21		; Second byte of morse code transmit
.def	tcnt1_match = r22		; Variable to check when TCNT1A Match interrupt occurs
.def	n = r23					; Register for loop itereations
.def	ltr = r24				; Register for letter selection in morse code
.def	column = r25			; Register for column of line 2 of display counter
.def	waitcnt = r26			; Wait Loop Counter
.def	ilcnt = r27				; Inner Loop Counter
.def	olcnt = r28				; Outer Loop Counter


.equ	WTime = 50				; Time to wait in wait loop
.equ	pbutton0 = 0			; Pushbutton 0 offset
.equ	pbutton4 = 4			; Pushbutton 4 offset
.equ	pbutton6 = 6			; Pushbutton 6 offset
.equ	pbutton7 = 7			; Pushbutton 7 offset

;***********************************************************
;*	Start of Code Segment
;***********************************************************
.cseg							; Beginning of code segment

;***********************************************************
;*	Interrupt Vectors
;***********************************************************
.org	$0000					; Beginning of IVs
		rjmp INIT				; Reset interrupt
.org	$0018					; Interrupt of TCNT1A Match
		ldi		tcnt1_match, $01
		reti
.org	$0046					; End of Interrupt Vectors

;***********************************************************
;*	Program Initialization
;***********************************************************
INIT:							; The initialization routine
		; Initialize Stack Pointer
		ldi		mpr, low(RAMEND)
		out		SPL, mpr
		ldi		mpr, high(RAMEND)
		out		SPH, mpr

		; Initialize LCD Display
		call	LCDInit

		; Configure I/O ports

		; Initialize Port D for input
		ldi		mpr, $00		; Set Port D Data Direction Register
		out		DDRD, mpr		; for input
		ldi		mpr, $FF		; Initialize Port D Data Register
		out		PORTD, mpr		; so all Port D inputs are Tri-State

		; Initialize Port B for output
		ldi		mpr, $FF		; Set Port B Data Direction Register
		out		DDRB, mpr		; for output
		ldi		mpr, $00		; Initialize Port B Data Register
		out		PORTB, mpr		; so all Port B outputs are low		

		; Initialize the Timer/Counter 1
		ldi		mpr, 0b10101000		; Set TCCR1A to clear OCnA/B/C on compare match,
		out		TCCR1A, mpr			; and first two bits of WGM to 00 for CTC mode
		ldi		mpr, 0b00001101		; Set TCCR1B to set second two bits of WGM to 01
		out		TCCR1B, mpr			; for CTC mode and fully prescaled clock

		ldi		mpr, 0b00010000		; Set TIMSK to enable OCIE1A interrupt
		out		TIMSK, mpr

		ldi		mpr, $3D		; Initialize the TOP of TCNT1 to be 15625, 
		ldi		n, $09			; the same frequency of the clock after prescaler
		out		OCR1AH, mpr
		out		OCR1AL, n

		

		; Move strings from Program Memory to Data Memory
		call	string1l1_move		; Starting string for line 1 of display
		call	string1l2_move		; Starting string for line 2 of display
		call	LCDWrite			; Write the starting strings
		call	morse_move			; Move the letters for display into data memory
		call	morseCode_move		; Move the transmission combinations into data memory

		sei

;***********************************************************
;*	Main Program
;***********************************************************
MAIN:	
		in		mpr, PIND		; Take in PIND
		cpi		mpr, $FE		; Check if PD0 has been pushed
		breq	start_prog		; Start the program if PD0 has been hit
		rjmp	MAIN			; jump back to main and create an infinite
								; while loop.  Generally, every main program is an
								; infinite while loop, never let the main program
								; just run off

;***********************************************************
;*	Functions and Subroutines
;***********************************************************

;-----------------------------------------------------------
; Func: string1l1_move
; Desc: Move the string from program memory into data memory
;		for the first line of the display.
;-----------------------------------------------------------
string1l1_move:
		; Save variables by pushing them to the stack
		push	mpr

		; Execute the function here
		ldi		ZL, low(stringStart1_beg<<1)	; Set Z to location in program memory
		ldi		ZH, high(stringStart1_beg<<1)
		ldi		YL, $00		; Set Y to location in data memory
		ldi		YH, $01
		ldi		n, 8		; Set number of iterations required for moving string
		rcall	mem_move	; Move into data memory
		
		; Restore variables by popping them from the stack,
		; in reverse order
		pop		mpr
		ret

;-----------------------------------------------------------
; Func: string1l2_move
; Desc: Move the string from program memory into data memory
;		for the second line of the display.
;-----------------------------------------------------------
string1l2_move:
		; Save variables by pushing them to the stack
		push	mpr

		; Execute the function here
		ldi		ZL, low(stringStart2_beg<<1)	; Set Z to location in program memory
		ldi		ZH, high(stringStart2_beg<<1)	
		ldi		YL, $10		; Set Y to location in data memory
		ldi		YH, $01
		ldi		n, 16		; Set number of iterations required for moving string
		rcall	mem_move	; Move into data memory
		
		; Restore variables by popping them from the stack,
		; in reverse order
		pop		mpr
		ret						

;-----------------------------------------------------------
; Func: morse_move
; Desc: Move the string from program memory into data memory
;		for morse letters.
;-----------------------------------------------------------
morse_move:
		; Save variables by pushing them to the stack
		push	mpr

		; Execute the function here
		ldi		ZL, low(morse_start<<1)		; Set Z to location in program memory
		ldi		ZH, high(morse_start<<1)
		ldi		YL, $00		; Set Y to location in data memory
		ldi		YH, $02
		ldi		n, 26		; Set number of iterations to move strings
		rcall	mem_move	; Move into data memory
		
		; Restore variables by popping them from the stack,
		; in reverse order
		pop		mpr
		ret

;-----------------------------------------------------------
; Func: morseCode_move
; Desc: Move the string from program memory into data memory
;		for morse letters.
;-----------------------------------------------------------
morseCode_move:
		; Save variables by pushing them to the stack
		push	mpr

		; Execute the function here
		ldi		ZL, low(morseCode_start<<1)		; Set Z to location in program memory
		ldi		ZH, high(morseCode_start<<1)
		ldi		YL, $1B		; Set Y to location in data memory
		ldi		YH, $02
		ldi		n, 52		; Set number of iterations to move data
		rcall	mem_move	; Move into data memory
		
		; Restore variables by popping them from the stack,
		; in reverse order
		pop		mpr
		ret
;-----------------------------------------------------------
; Func: string2_move
; Desc: Move the string from program memory into data memory
;		for the first line of the display for the program.
;-----------------------------------------------------------
string2_move:
		; Save variables by pushing them to the stack
		push	mpr

		; Execute the function here
		ldi		ZL, low(stringPerm_beg<<1)		; Set Z to location in program memory
		ldi		ZH, high(stringPerm_beg<<1)	
		ldi		YL, $00		; Set Y to location in data memory
		ldi		YH, $01
		ldi		n, 12		; Set number of iterations to move the string
		rcall	mem_move	; Move into data memory
		
		; Restore variables by popping them from the stack,
		; in reverse order
		pop		mpr
		ret

;-----------------------------------------------------------
; Func: mem_move
; Desc: Move data located in program memory into data
;		memory.
;-----------------------------------------------------------
mem_move:	
		lpm		mpr, Z+
		st		Y+, mpr
		dec		n
		brne	mem_move	; Checks if n is zero
		ret

;-----------------------------------------------------------
; Func: LCDMem_clr
; Desc: Clear any data in the LCD display data memory. This
;		is addresses $0100-$011F
;-----------------------------------------------------------
LCDMem_clr:	
		st		Y+, mpr
		dec		n			
		brne	LCDMem_clr	; Checks if n is zero
		ret

;-----------------------------------------------------------
; Subroutine: start_prog
; Desc: After PD0 is hit, begin taking in values for morse
;		code transmission
;-----------------------------------------------------------
start_prog:	
		; Clear out any memory located in the display location lines
		ldi		mpr, $20
		ldi		YL, $00
		ldi		YH, $01
		ldi		n, $FF
		rcall	LCDMem_clr

		; Put the new string into data memory location for LCD Display
		rcall	string2_move
		ldi		XL, $00
		ldi		XH, $02
		ldi		YL, $10
		ldi		YH, $01

		clr		ltr		; Set ltr register to 0
		clr		column	; Set column register to 0

		; Put the letter A into second line display for LCD
		ld		mpr, X	
		st		Y, mpr
		call	LCDWrite
		
		; Wait to remove any button bouncing
		ldi		waitcnt, WTime
		rcall	kill_time

MAIN2:
		; Check for button pushes
		in		mpr, PIND	; Take in PIND
		andi	mpr, (1<<pbutton7|1<<pbutton6|1<<pbutton4|1<<pbutton0)	; Filter any buttons other than 0, 4, 6, and 7
		cpi		mpr, (1<<pbutton7|1<<pbutton6|1<<pbutton4)	; Check if button 0 was pushed
		brne	check_pb4	; Button 0 not pushed, move to next check
		rcall	move_column	; Button 0 was pushed
		rjmp	end_main2	; Repeat polling loop
check_pb4:
		cpi		mpr, (1<<pbutton7|1<<pbutton6|1<<pbutton0) ; Check if button 4 was pushed
		brne	check_pb6		; Button 4 not pushed, move to next check
		rcall	transmit_code	; Button 4 was pushed
		rjmp	end_main2		; Repeate polling loop
check_pb6:
		cpi		mpr, (1<<pbutton7|1<<pbutton4|1<<pbutton0) ; Check if button 6 was pushed
		brne	check_pb7	; Button 6 was not pushed, move to next check
		rcall	dec_letter	; Button 6 was pushed
		rjmp	end_main2	; Repeat polling loop
check_pb7:
		cpi		mpr, (1<<pbutton6|1<<pbutton4|1<<pbutton0)	; Check if button 7 was pushed
		brne	end_main2	; Button 7 was not pushed, repeat polling loop
		rcall	inc_letter	; Button 7 was pushed
end_main2:
		rjmp	MAIN2	; Repeat polling loop

;-----------------------------------------------------------
; Func: inc_letter
; Desc: This function will increment the letter shown on the
;		second line of the LCD Display.
;-----------------------------------------------------------
inc_letter:
		; Push registers to be saved onto the stack
		push	mpr
		push	XL
		push	XH

		; Execute the function here
		ldi		XL, $00		; Set X to start of Morse letters in data memory
		ldi		XH, $02
		cpi		ltr, 25		; Check if ltr is at the max, i.e. letter Z
		brge	to_start	; If at max, next letter will be A
		inc		ltr			; If not, then increment ltr
		add		XL, ltr		; Add ltr and low byte of X for the offset to select next letter
		ld		mpr, X		; Load the new letter into mpr
		st		Y, mpr		; Y is set to location for LCD Display line 2 and column displacement, store mpr
		rjmp	end_of_inc_letter	
to_start:
		clr		ltr			; Set ltr offset back to 0
		ld		mpr, X		; Store letter A into mpr
		st		Y, mpr		; Put mpr into LCD Display line 2 location

		; Restore variables by popping them from the stack,
		; in reverse order
end_of_inc_letter:
		call	LCDWrite	; Write the new contents to LCD Display
		ldi		waitcnt, WTime
		rcall	kill_time	; Wait to remove any button bouncing
		pop		XH			; Restore X
		pop		XL
		pop		mpr			; Restore mpr

		ret	

;-----------------------------------------------------------
; Func: dec_letter
; Desc: This function will increment the letter shown on the
;		second line of the LCD Display.
;-----------------------------------------------------------
dec_letter:
		push	mpr
		push	XL
		push	XH

		; Execute the function here
		ldi		XL, $00		; Set X to location of letters
		ldi		XH, $02
		cpi		ltr, 0		; Check if ltr is already a 0, i.e. at letter A
		breq	to_end		; If at max, next letter will be Z
		dec		ltr			; Move ltr down for the offset
		add		XL, ltr		; Offset low byte of X by ltr to get the next letter down
		ld		mpr, X		; Load in the letter
		st		Y, mpr		; Y is set to location for LCD Display line 2 and column displacement, store mpr
		rjmp	end_of_dec_letter
to_end:
		ldi		ltr, 25		; Set ltr to the end of the list of letters, to Z
		add		XL, ltr		; Add the offset ot X
		ld		mpr, X		; Load Z into mpr
		st		Y, mpr		; Y is set to location for LCD Display line 2 and column displacement, store mpr

		; Restore variables by popping them from the stack,
		; in reverse order
end_of_dec_letter:
		call	LCDWrite		; Write the new display data
		ldi		waitcnt, WTime	
		rcall	kill_time		; Wait to remove any button bouncing
		pop		XH				; Restore X
		pop		XL
		pop		mpr				; Restore mpr

		ret

;-----------------------------------------------------------
; Func: move_column
; Desc: Move the Y pointer to the next digit in the second
;		line of the LCD Display
;-----------------------------------------------------------
move_column:
		; Save variables by pushing them to the stack
		push	mpr

		; Execute the function here
		clr		ltr				; Set ltr offset back to 0
		cpi		column, 15		; Check if at the max number of columns
		breq	columns_full	; If at the max, transmit the morse code

		ldi		YL, $10			; Set Y to start of LCD Display line 2 in data memory
		ldi		YH, $01		
		inc		column			; Increment column offset
		add		YL, column		; Add the offset to low byte of Y to shift over that many columns

		ldi		XL, $00			; Set X to starting location of letters, to A
		ldi		XH, $02
		ld		mpr, X			; Load A into mpr
		st		Y, mpr			; Store into new column of data memory

		call	LCDWrite		; Write the new data onto LCD Display
		rjmp	end_of_column_move

columns_full:
		rcall	transmit_code ; Transmit the morse code


		; Restore variables by popping them from the stack,
		; in reverse order
end_of_column_move:
		ldi		waitcnt, WTime
		rcall	kill_time		; Wait to remove any button bouncing
		pop		mpr				; Restore mpr
		ret						

;-----------------------------------------------------------
; Func: transmit_code
; Desc: Trasnmit the morse code provided in the LCD Display
;-----------------------------------------------------------
transmit_code:
		
		ldi		XL, $10		; Set X to location of second line of LCD Display
		ldi		XH, $01
		inc		column		; Increment column. This will serve as the number of iterations to run
		ldi		mpr, $00	; Turn off all LEDs on PORTB
		out		PORTB, mpr

next_letter:
		ld		mpr, X+		; Load in a letter, set X to next in memory
		subi	mpr, 65		; Subtracting the ascii value in mpr by 65 will limit ascii from A-Z
		ldi		YL, $1B		; Set Y to location of morse code transmission values
		ldi		YH, $02
		add		YL, mpr		; Increment YL by mpr to shift to the letter transmission. Since these
		add		YL, mpr		; are 2 bytes, must be incremented twice to reach proper letter.

		ld		morse_byte1, Y+		; Load high byte of morse code transmission value
		ld		morse_byte2, Y		; Load low byte of morse code transmission value

		; Check if the first nibble is a dot or dash. 1 represents dash, 0 represents dot, F represents end of letter.
		; There can never be an F in the first nibble of the transmission values.
check_nibble1:
		mov		mpr, morse_byte2
		andi	mpr, $F0		; Filter out the low nibble

		cpi		mpr, $10		; Check if it's a dash
		breq	send_dash_nibble1
		cpi		mpr, $00		; Check if it's a dot
		breq	send_dot_nibble1

		; Check if the second nibble is a dot or dash. This can result in an F.
check_nibble2:
		mov		mpr, morse_byte2
		andi	mpr, $0F	; Filter out high nibble

		cpi		mpr, $0F	; Check if transmission of letter is over
		breq	next_iterate
		cpi		mpr, $01	; Check if it's a dash
		breq	send_dash_nibble2
		cpi		mpr, $00	; Check if it's a dot
		breq	send_dot_nibble2

		; Check if the third nibble is a dot or dash.
check_nibble3:
		mov		mpr, morse_byte1
		andi	mpr, $F0	; Filter out low nibble

		cpi		mpr, $F0	; Check for end of letter transmission
		breq	next_iterate
		cpi		mpr, $10	; Check for dash
		breq	send_dash_nibble3
		cpi		mpr, $00	; Check for dot
		breq	send_dot_nibble3

		; Check if the fourth nibble is a dot or dash.
check_nibble4:
		mov		mpr, morse_byte1
		andi	mpr, $0F	; Filter out high nibble
	
		cpi		mpr, $0F	; Check if end of letter transmission
		breq	next_iterate
		cpi		mpr, $01	; Check for dash
		breq	send_dash_nibble4
		cpi		mpr, $00	; Check for dot
		breq	send_dot_nibble4

send_dash_nibble1:
		call	dash	; Send a dash
		rjmp	check_nibble2	; Move back and check next nibble
send_dash_nibble2:
		call	dash
		rjmp	check_nibble3
send_dash_nibble3:
		call	dash
		rjmp	check_nibble4
send_dash_nibble4:
		call	dash
		rjmp	next_iterate	; Final nibble was a dash, move to next iteration
send_dot_nibble1:
		call	dot		; Send a dot
		rjmp	check_nibble2	; Move back and check next nibble
send_dot_nibble2:
		call	dot
		rjmp	check_nibble3
send_dot_nibble3:
		call	dot
		rjmp	check_nibble4
send_dot_nibble4:
		call	dot	; Final nibble was a dot, move to next iteration
next_iterate:
		dec		column		; Reduce column
		cpi		column, 0	; If no more columns, then transmission is over
		brne	next_letter	; If column still has a value, check the next transmission set

		; Set LED 1 to 0 for 3 seconds to represent end of transmission. Only need to call
		; one_second 2 more times since the end of each dot or dash has 1 second of off already.
end_of_transmission:
		ldi		mpr, $10
		out		PORTB, mpr
		rcall	one_second
		rcall	one_second
		ldi		mpr, $0
		out		PORTB, mpr	; Turn off all LEDs
		rjmp	start_prog	; Reset the morse code transmission program

;-----------------------------------------------------------
; Func: dot
; Desc: Transmit a "dot" in morse code. LED on for 1 second,
;		then off for one second.
;-----------------------------------------------------------
dot:
		push	mpr

		; Execute the function here
		ldi		mpr, $11
		out		PORTB, mpr
		rcall	one_second
		ldi		mpr, $10
		out		PORTB, mpr
		rcall	one_second

		; Restore variables by popping them from the stack,
		; in reverse order
		pop		mpr
		ret						; End a function with RET

;-----------------------------------------------------------
; Func: dash
; Desc: Transmit a "dash" in morse code. LED on for three
;		seconds, off for one second
;-----------------------------------------------------------
dash:
		push	mpr

		; Execute the function here
		ldi		mpr, $11		; Set LED 4 and 1 high, 
		out		PORTB, mpr		; 4 represents transmission in progress
		rcall	one_second		; Wait 3 seconds
		rcall	one_second
		rcall	one_second
		ldi		mpr, $10		; Set LED4 high, 1 low
		out		PORTB, mpr		
		rcall	one_second		; Wait 1 second

		; Restore variables by popping them from the stack,
		; in reverse order
		pop		mpr
		ret

;-----------------------------------------------------------
; Func: one_second
; Desc: Check for the interrupt of Timer/Counter 1 which will
;		add up to be roughly 1 second per interrupt.
;-----------------------------------------------------------
one_second:
		push	mpr
		push	tcnt1_match
		; Execute the function here
		ldi		mpr, $00		; Reset the timer/counter 1
		out		TCNT1H, mpr
		out		TCNT1L, mpr
		ldi		tcnt1_match, $0	; Set the check to 0

		; Loop until the TCNT1 match interrupt is triggered
compare_loop:
		cpi		tcnt1_match, $01	; When interrupt is triggered
		breq	end_second			; the variable will change to a 1
		rjmp	compare_loop

		; Restore variables by popping them from the stack,
		; in reverse order
end_second:
		pop		tcnt1_match		; Restore tcnt1_match
		pop		mpr				; Restore mpr
		ret						; End a function with RET

;----------------------------------------------------------------
; Sub:	kill_time
; Desc:	A wait loop that is 16 + 159975*waitcnt cycles or roughly 
;		waitcnt*10ms.  Just initialize wait for the specific amount 
;		of time in 10ms intervals. Here is the general eqaution
;		for the number of clock cycles in the wait loop:
;			((3 * ilcnt + 3) * olcnt + 3) * waitcnt + 13 + call
;----------------------------------------------------------------
kill_time:
		push	waitcnt			; Save wait register
		push	ilcnt			; Save ilcnt register
		push	olcnt			; Save olcnt register

Loop:	ldi		olcnt, 224		; load olcnt register
OLoop:	ldi		ilcnt, 237		; load ilcnt register
ILoop:	dec		ilcnt			; decrement ilcnt
		brne	ILoop			; Continue Inner Loop
		dec		olcnt		; decrement olcnt
		brne	OLoop			; Continue Outer Loop
		dec		waitcnt		; Decrement wait 
		brne	Loop			; Continue Wait loop	

		pop		olcnt		; Restore olcnt register
		pop		ilcnt		; Restore ilcnt register
		pop		waitcnt		; Restore wait register
		ret				; Return from subroutine

;***********************************************************
;*	Stored Program Data
;***********************************************************

;-----------------------------------------------------------
; An example of storing a string. Note the labels before and
; after the .DB directive; these can help to access the data
;-----------------------------------------------------------
stringStart1_beg:
.DB		"Welcome!"			; First line for initial statement

stringStart2_beg:
.DB		"Please press PD0"	; Second line for initial statement

stringPerm_beg:
.DB		"Enter word: "		; Line to stay while running program

morse_start:
.DB		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z' ; Letters for the LCD Display

morseCode_start:
.DW		0x01FF, 0x100, 0x1010, 0x100F, 0x0FFF, 0x0010, 0x110F, 0x0000, 0x00FF, 0x0111, 0x101F, 0x0100, 0x11FF, 0x10FF, 0x111F, 0x0110, 0x1101, 0x010F, 0x000F, 0x1FFF, 0x001F, 0x0001, 0x011F, 0x1001, 0x1011, 0x1100 ; Morse code values for transmission
;***********************************************************
;*	Additional Program Includes
;***********************************************************
.include "LCDDriver.asm"		; Include the LCD Driver

