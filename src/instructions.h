/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * MCUS
 * Copyright (C) Philip Withnall 2008 <philip@tecnocode.co.uk>
 * 
 * MCUS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MCUS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MCUS.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MCUS_INSTRUCTION_H
#define MCUS_INSTRUCTION_H

#include <glib.h>

G_BEGIN_DECLS

/* Maximum instruction opcode length in characters */
#define MAX_INSTRUCTION_LENGTH 5

/* Maximum number of operands for an instruction */
#define MAX_OPERAND_COUNT 2

/* These are convenient enums for the instructions, their decimal opcodes for compilation,
 * and keys for looking the relevant data up in mcus_instruction_data. */
typedef enum {
	INSTRUCTION_MOVI = 0,	/* MOVI Sd,n	Copy the byte n into register Sd */
	INSTRUCTION_MOV,	/* MOV Sd,Ss	Copy the byte from As to Sd */
	INSTRUCTION_ADD,	/* ADD Sd,Ss	Add the byte in Ss to the byte in Sd and store the result in Sd */
	INSTRUCTION_SUB,	/* SUB Sd,Ss	Subtract the byte in Ss from the byte in Sd and store the result in Sd */
	INSTRUCTION_AND,	/* AND Sd,Ss	Logical AND the byte in Ss with the byte in Sd and store the result in Sd */
	INSTRUCTION_EOR,	/* EOR Sd,Ss	Logical EOR the byte in Ss with the byte in Sd and store the result in Sd */
	INSTRUCTION_INC,	/* INC Sd	Add 1 to Sd */
	INSTRUCTION_DEC,	/* DEC Sd	Subtract 1 from Sd */
	INSTRUCTION_IN,		/* IN Sd,I	Copy the byte at the input port into Sd */
	INSTRUCTION_OUT,	/* OUT Q,Ss	Copy the byte in Ss to the output port */
	INSTRUCTION_JP,		/* JP e		Jump to label e */
	INSTRUCTION_JZ,		/* JZ e		Jump to label e if the result of the last ADD, SUB, AND, EOR, INC, DEC, SHL or SHR was zero */
	INSTRUCTION_JNZ,	/* JNZ e	Jump to label e if the result of the last ADD, SUB, AND, EOR, INC, DEC SHL or SHR was not zero */
	INSTRUCTION_RCALL,	/* RCALL s	Push the program counter onto the stack to store the return address and then jump to label s */
	INSTRUCTION_RET,	/* RET		Pop the program counter from the stack to return to the place the subroutine was called from */
	INSTRUCTION_SHL,	/* SHL Sd	Shift the byte in Sd one bit left putting a 0 into the lsb */
	INSTRUCTION_SHR		/* SHR Sd	Shift the byte in Sd one bit right putting a 0 into the msb */
} MCUSInstructionType;

typedef struct {
	MCUSInstructionType opcode;
	gchar *instruction_name;
	guint operand_count;
} MCUSInstructionData;

const MCUSInstructionData mcus_instruction_data[] = {
	/* Instruction type,	name,		operand count */
	{ INSTRUCTION_MOVI,	"MOVI",		2 },
	{ INSTRUCTION_MOV,	"MOV",		2 },
	{ INSTRUCTION_ADD,	"ADD",		2 },
	{ INSTRUCTION_SUB,	"SUB",		2 },
	{ INSTRUCTION_AND,	"AND",		2 },
	{ INSTRUCTION_EOR,	"EOR",		2 },
	{ INSTRUCTION_INC,	"INC",		1 },
	{ INSTRUCTION_DEC,	"DEC",		1 },
	{ INSTRUCTION_IN,	"IN",		2 },
	{ INSTRUCTION_OUT,	"OUT",		2 },
	{ INSTRUCTION_JP,	"JP",		1 },
	{ INSTRUCTION_JZ,	"JZ",		1 },
	{ INSTRUCTION_JNZ,	"JNZ",		1 },
	{ INSTRUCTION_RCALL,	"RCALL",	1 },
	{ INSTRUCTION_RET,	"RET",		0 },
	{ INSTRUCTION_SHL,	"SHL",		1 },
	{ INSTRUCTION_SHR,	"SHR",		1 }
}

G_END_DECLS

#endif /* !MCUS_INSTRUCTION_H */
