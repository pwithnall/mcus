; Analogue-to-digital converter and divider
; Takes a reading from the ADC and well as a user-defined factor and returns the next integer above the quotient of the two.
; Note that the division method used here isn't particularly clever.

; Input a voltage from the ADC in S0 and a user-defined factor by which to divide it in S1
RCALL readadc
IN S1, I

; Inputs:
; * S0 is the numerator (ADC voltage)
; * S1 is the denominator (user-inputted factor)
; Outputs:
; * S4 is the remainder
; * S5 is the quotient
division_loop:
	INC S5 ; Increment the quotient
	MOV S4, S0 ; Set up for gte (numerator)
	SUB S0, S1 ; Subtract the denominator from the numerator --- this will be lower than the numerator if we haven't reached fractions yet, and higher if we have (the numerator will underflow)
	MOV S3, S0 ; Set up for gte (numerator - denominator)
	RCALL gte
	JZ division_loop

; Division's finished: output the quotient before stopping
OUT Q, S5
HALT

; Take in S3 and S4 and destructively return in S7:
; * 01 if S3 >= S4
; * 00 otherwise
gte:
	DEC S4
	JZ gte_true
	DEC S3
	JZ gte_false
	JP gte

gte_true:
	MOVI S7, 01
	AND S7, S7
	RET

gte_false:
	MOVI S7, 00
	AND S7, S7
	RET