; Scrolling message
; Displays a simple scrolling message on the multiplexed SSDs.
; In real life, this would be executed fast enough for your eyes to perceive it as one continuous message being
; displayed on many SSDs at once, rather than just one.

; Set up the message
MOVI S3, 01 ; "1"
MOVI S4, 02 ; "2"
MOVI S5, 03 ; "3"
MOVI S6, 04 ; "4"
MOVI S7, 05 ; "5"

MOVI S1, 00 ; Position
MOVI S2, 10 ; Increment for S1
MOVI S0, 00 ; Working space

loop:
	; Combine the position with the value to display
	MOV S0, S1
	EOR S0, S3
	OUT Q, S0 ; Output the value
	ADD S1, S2 ; Increment the position

	; Repeat for the other values to display
	MOV S0, S1
	EOR S0, S4
	OUT Q, S0
	ADD S1, S2

	MOV S0, S1
	EOR S0, S5
	OUT Q, S0
	ADD S1, S2

	MOV S0, S1
	EOR S0, S6
	OUT Q, S0
	ADD S1, S2

	MOV S0, S1
	EOR S0, S7
	OUT Q, S0
	ADD S1, S2

	JP loop