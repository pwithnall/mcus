; LED chaser
; Lights up the output LEDs in sequence.

main:
	MOVI S0, 01 ; Initialise the output to 00000001 (i.e. just the first LED lit)

while_loop:
	OUT Q, S0 ; Output
	RCALL wait1ms
	SHL S0 ; Shift the bits in the S0 register left one place, so the high bit moves left, and lights the next LED in the sequence
	JZ main ; If the bit is shifted off the end, S0 will be 00, so we should go back to the beginning
	JP while_loop