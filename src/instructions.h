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

/* Maximum opcode mnemonic length in characters */
#define MAX_MNEMONIC_LENGTH 9

/* Maximum directive name length in characters, *not* including the dollar sign */
#define MAX_DIRECTIVE_NAME_LENGTH 3

/* Maximum number of operands for an instruction */
#define MAX_ARITY 2

/* Maximum instruction size in bytes (operand count plus one byte for the opcode) */
#define MAX_INSTRUCTION_SIZE MAX_ARITY + 1

/* These are convenient enums for the instructions, their decimal opcodes for compilation,
 * and keys for looking the relevant data up in mcus_instruction_data. */
typedef enum {
	OPCODE_HALT = 0,/* HALT		Extra instruction added by me to terminate simulation */
	OPCODE_MOVI,	/* MOVI Sd,n	Copy the byte n into register Sd */
	OPCODE_MOV,	/* MOV Sd,Ss	Copy the byte from Ss to Sd */
	OPCODE_ADD,	/* ADD Sd,Ss	Add the byte in Ss to the byte in Sd and store the result in Sd */
	OPCODE_SUB,	/* SUB Sd,Ss	Subtract the byte in Ss from the byte in Sd and store the result in Sd */
	OPCODE_AND,	/* AND Sd,Ss	Logical AND the byte in Ss with the byte in Sd and store the result in Sd */
	OPCODE_EOR,	/* EOR Sd,Ss	Logical EOR the byte in Ss with the byte in Sd and store the result in Sd */
	OPCODE_INC,	/* INC Sd	Add 1 to Sd */
	OPCODE_DEC,	/* DEC Sd	Subtract 1 from Sd */
	OPCODE_IN,	/* IN Sd,I	Copy the byte at the input port into Sd */
	OPCODE_OUT,	/* OUT Q,Ss	Copy the byte in Ss to the output port */
	OPCODE_JP,	/* JP e		Jump to label e */
	OPCODE_JZ,	/* JZ e		Jump to label e if the result of the last ADD, SUB, AND, EOR, INC, DEC, SHL or SHR was zero */
	OPCODE_JNZ,	/* JNZ e	Jump to label e if the result of the last ADD, SUB, AND, EOR, INC, DEC, SHL or SHR was not zero */
	OPCODE_RCALL,	/* RCALL s	Push the program counter onto the stack to store the return address and then jump to label s */
	OPCODE_RET,	/* RET		Pop the program counter from the stack to return to the place the subroutine was called from */
	OPCODE_SHL,	/* SHL Sd	Shift the byte in Sd one bit left putting a 0 into the lsb */
	OPCODE_SHR,	/* SHR Sd	Shift the byte in Sd one bit right putting a 0 into the msb */

	SUBROUTINE_READTABLE,	/* Copies the byte in the lookup table pointed at by S7 into S0. The lookup table is
				 * labelled table: when S7=0 the first byte from the table is returned in S0. */
	SUBROUTINE_WAIT1MS,	/* Waits 1 ms before returning. */
	SUBROUTINE_READADC	/* Returns a byte in S0 proportional to the voltage at ADC. */
} MCUSOpcode;

typedef enum {
	DIRECTIVE_SET = 0
} MCUSDirective;

typedef enum {
	OPERAND_CONSTANT,
	OPERAND_LABEL,
	OPERAND_REGISTER,
	OPERAND_INPUT,
	OPERAND_OUTPUT
} MCUSOperandType;

typedef struct {
	const MCUSOpcode opcode;
	const gchar *mnemonic;
	const guint arity;
	const guint size;
	const MCUSOperandType operand_types[MAX_ARITY];
	const gchar *help;
} MCUSInstructionData;

extern const MCUSInstructionData mcus_instruction_data[];

typedef struct {
	const MCUSDirective directive;
	const gchar *directive_name;
} MCUSDirectiveData;

extern const MCUSDirectiveData mcus_directive_data[];

G_END_DECLS

#endif /* !MCUS_INSTRUCTION_H */
