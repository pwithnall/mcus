; SSD test program
; Cycles through the digits on a single SSD (set up to decode as segments), ensuring they all work

; Set up a lookup table with all the values to output for each different thing we want outputted on the SSD
$SET F0 FF ; All: 11111111
$SET F1 3F ; 0: 00111111
$SET F2 86 ; 1+DP: 10000110
$SET F3 5B ; 2: 01011011
$SET F4 CF ; 3+DP: 11001111
$SET F5 66 ; 4: 01100110
$SET F6 ED ; 5+DP: 11101101
$SET F7 7D ; 6: 01111101
$SET F8 87 ; 7+DP: 10000111
$SET F9 7F ; 8: 01111111
$SET FA EF ; 9+DP: 11101111
$SET FB 00 ; Terminator

main:
	MOVI S7 F0 ; Set S7 up to point at the first address in the lookup table

while_loop:
	RCALL readtable ; Read a value from the lookup table

	AND S0, S0 ; If S0 is 00, the zero flag in the status register will be taken high here, ready for the JZ below
	OUT Q, S0 ; Output the value
	RCALL wait1ms
	JZ main ; If S0 is zero, reset the loop

	INC S7 ; Increment the table index
	JP while_loop