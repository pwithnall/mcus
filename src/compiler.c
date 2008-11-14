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

#include <glib.h>
#include <glib/gi18n.h>
#include <math.h>
#include <string.h>

#include "compiler.h"
#include "instructions.h"
#include "main.h"

typedef struct {
	gchar *label;
	guchar address;
} MCUSLabel;

typedef struct {
	MCUSOperandType type;
	union {
		guchar number; /* for constants, registers, inputs and output */
		gchar *label; /* for labels only */
	};
} MCUSOperand;

typedef struct {
	MCUSOpcode opcode;
	MCUSOperand operands[MAX_ARITY];
	guint offset;
	guint length;
} MCUSInstruction;

const MCUSInstructionData const mcus_instruction_data[] = {
	/* Opcode,	name,		arity,	size (bytes),		operand types */
	{ OPCODE_HALT,	"HALT",		0,	1,			{  },
		N_("HALT — halts simulation of the program.") },
	{ OPCODE_MOVI,	"MOVI",		2,	3,			{ OPERAND_REGISTER,	OPERAND_CONSTANT },
		N_("MOVI Sx, 00 — move the second operand into the register specified by the first.") },
	{ OPCODE_MOV,	"MOV",		2,	3,			{ OPERAND_REGISTER,	OPERAND_REGISTER },
		N_("MOV Sx, Sy — move the second register into the first.") },
	{ OPCODE_ADD,	"ADD",		2,	3,			{ OPERAND_REGISTER,	OPERAND_REGISTER },
		N_("ADD Sx, Sy — add the second register to the first and store the result in the first.") },
	{ OPCODE_SUB,	"SUB",		2,	3,			{ OPERAND_REGISTER,	OPERAND_REGISTER },
		N_("SUB Sx, Sy — subtract the second register from the first and store the result in the first.") },
	{ OPCODE_AND,	"AND",		2,	3,			{ OPERAND_REGISTER,	OPERAND_REGISTER },
		N_("AND Sx, Sy — logically AND the two registers, storing the result in the first.") },
	{ OPCODE_EOR,	"EOR",		2,	3,			{ OPERAND_REGISTER,	OPERAND_REGISTER },
		N_("EOR Sx, Sy — logically EOR the two registers, storing the result in the first.") },
	{ OPCODE_INC,	"INC",		1,	2,			{ OPERAND_REGISTER, },
		N_("INC Sx — increment the register by 1.") },
	{ OPCODE_DEC,	"DEC",		1,	2,			{ OPERAND_REGISTER, },
		N_("DEC Sx — decrement the register by 1.") },
	{ OPCODE_IN,	"IN",		2,	2, /* <-- special */	{ OPERAND_REGISTER,	OPERAND_INPUT },
		N_("IN Sx, I — move the value at the input port into the register.") },
	{ OPCODE_OUT,	"OUT",		2,	2, /* <-- special */	{ OPERAND_OUTPUT,	OPERAND_REGISTER },
		N_("OUT Q, Sx — move the value in the register to the output port.") },
	{ OPCODE_JP,	"JP",		1,	2,			{ OPERAND_LABEL, },
		N_("JP label — unconditionally jump to the instruction after the label.") },
	{ OPCODE_JZ,	"JZ",		1,	2,			{ OPERAND_LABEL, },
		N_("JZ label — jump to the instruction after the label if the result of the last operation was zero.") },
	{ OPCODE_JNZ,	"JNZ",		1,	2,			{ OPERAND_LABEL, },
		N_("JNZ label — jump to the instruction after the label if the result of the last operation was not zero.") },
	{ OPCODE_RCALL,	"RCALL",	1,	2,			{ OPERAND_LABEL, },
		N_("RCALL label — jump to the subroutine at label, storing the current program counter on the stack.") },
	{ OPCODE_RET,	"RET",		0,	1,			{  },
		N_("RET — return from the current subroutine call, popping the program counter off the stack.") },
	{ OPCODE_SHL,	"SHL",		1,	2,			{ OPERAND_REGISTER, },
		N_("SHL Sx — logically shift the bits in the register left one place.") },
	{ OPCODE_SHR,	"SHR",		1,	2,			{ OPERAND_REGISTER, },
		N_("SHR Sx — logically shift the bits in the register right one place.") }
};

const MCUSDirectiveData const mcus_directive_data[] = {
	/* Directive,		directive name */
	{ DIRECTIVE_SET,	"SET" }
};

static void mcus_compiler_init (MCUSCompiler *self);
static void mcus_compiler_finalize (GObject *object);
static void reset_state (MCUSCompiler *self);

#define LABEL_BLOCK_SIZE 5
#define INSTRUCTION_BLOCK_SIZE 10
#define COMPILER_ERROR_CONTEXT_LENGTH 4

struct _MCUSCompilerPrivate {
	MCUSLabel *labels;
	guint label_count;

	MCUSInstruction *instructions;
	guint instruction_count;

	const gchar *code;
	const gchar *i;

	guchar compiled_size;
	guint line_number;
	guint error_length;
	gboolean dirty;
};

G_DEFINE_TYPE (MCUSCompiler, mcus_compiler, G_TYPE_OBJECT)
#define MCUS_COMPILER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MCUS_TYPE_COMPILER, MCUSCompilerPrivate))

MCUSCompiler *
mcus_compiler_new (void)
{
	return g_object_new (MCUS_TYPE_COMPILER, NULL);
}

static void
mcus_compiler_class_init (MCUSCompilerClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	g_type_class_add_private (klass, sizeof (MCUSCompilerPrivate));
	gobject_class->finalize = mcus_compiler_finalize;
}

static void
mcus_compiler_init (MCUSCompiler *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, MCUS_TYPE_COMPILER, MCUSCompilerPrivate);
	self->priv->line_number = 1;
	self->priv->compiled_size = PROGRAM_START_ADDRESS;
}

static void
mcus_compiler_finalize (GObject *object)
{
	reset_state (MCUS_COMPILER (object));

	/* Chain up to the parent class */
	G_OBJECT_CLASS (mcus_compiler_parent_class)->finalize (object);
}

GQuark
mcus_compiler_error_quark (void)
{
	static GQuark q = 0;

	if (q == 0)
		q = g_quark_from_static_string ("mcus-compiler-error-quark");

	return q;
}

static const gchar *
operand_type_to_string (MCUSOperandType operand_type)
{
	switch (operand_type) {
	case OPERAND_CONSTANT:
		return _("constant");
	case OPERAND_LABEL:
		return _("label");
	case OPERAND_REGISTER:
		return _("register");
	case OPERAND_INPUT:
		return _("input");
	case OPERAND_OUTPUT:
		return _("output");
	default:
		return _("unknown");
	}
}

static void
reset_state (MCUSCompiler *self)
{
	guint i;

	if (self->priv->dirty == FALSE)
		return;

	/* Free labels and instructions and reset state */
	for (i = 0; i < self->priv->label_count; i++)
		g_free (self->priv->labels[i].label);

	g_free (self->priv->labels);
	g_free (self->priv->instructions);

	self->priv->label_count = 0;
	self->priv->instruction_count = 0;
	self->priv->compiled_size = PROGRAM_START_ADDRESS;
	self->priv->line_number = 1;

	self->priv->i = NULL;
	self->priv->dirty = FALSE;
}

static void
store_label (MCUSCompiler *self, const MCUSLabel *label)
{
	/* If our label array is full, extend it */
	if (self->priv->label_count % LABEL_BLOCK_SIZE == 0) {
		self->priv->labels = g_realloc (self->priv->labels,
						sizeof (MCUSLabel) * (self->priv->label_count - self->priv->label_count % LABEL_BLOCK_SIZE + 1) * LABEL_BLOCK_SIZE);
		self->priv->label_count++;

		if (mcus->debug == TRUE) {
			g_debug ("Reallocating label memory block to %lu bytes due to having to store label %u (\"%s\").",
				 (gulong)(sizeof (MCUSLabel) * (self->priv->label_count - self->priv->label_count % LABEL_BLOCK_SIZE + 1) * LABEL_BLOCK_SIZE),
				 self->priv->label_count,
				 label->label);
		}
	} else {
		self->priv->label_count++;
	}

	/* Copy the new label into the array */
	g_memmove (&(self->priv->labels[self->priv->label_count-1]), label, sizeof (MCUSLabel));
}

static guchar
resolve_label (MCUSCompiler *self, guint instruction_number, const gchar *label_string, GError **error)
{
	guint i;

	if (mcus->debug == TRUE)
		g_debug ("Resolving label \"%s\".", label_string);

	/* Resolve labels for the built-in subroutines to some special (hacky) locations:
	 *  * readtable: Copies the byte in the lookup table pointed at by S7 into S0. The lookup table is
	 *		 labelled table: when S7=0 the first byte from the table is returned in S0.
	 *
	 *		 Location: the address where the RCALL opcode is stored.
	 *  * wait1ms:   Waits 1 ms before returning.
	 *
	 *		 Location: the address where the RCALL instruction's operand is stored.
	 *  * readadc:   Returns a byte in S0 proportional to the voltage at ADC.
	 *
	 *		 Location: the address of the opcode after the RCALL instruction. This is the most hacky,
	 *		 as there is legitimate code which would produce this in-memory representation. However,
	 *		 such code would be stupid and not work as part of a program regardless.
	 */
	if (strcmp (label_string, "readtable") == 0)
		return self->priv->compiled_size - 1;
	else if (strcmp (label_string, "wait1ms") == 0)
		return self->priv->compiled_size;
	else if (strcmp (label_string, "readadc") == 0)
		return self->priv->compiled_size + 1;

	/* Otherwise, just go through our list of labels */
	for (i = 0; i < self->priv->label_count; i++) {
		if (strcmp (label_string, self->priv->labels[i].label) == 0)
			return self->priv->labels[i].address;
	}

	self->priv->error_length = strlen (label_string);
	g_set_error (error, MCUS_COMPILER_ERROR, MCUS_COMPILER_ERROR_UNRESOLVABLE_LABEL,
		     _("A label (\"%s\") could not be resolved to an address for instruction %u."),
		     label_string,
		     instruction_number + 1);
	return 0;
}

static void
store_instruction (MCUSCompiler *self, const MCUSInstruction *instruction)
{
	/* If our instruction array is full, extend it */
	if (self->priv->instruction_count % INSTRUCTION_BLOCK_SIZE == 0) {
		self->priv->instructions = g_realloc (self->priv->instructions,
						      sizeof (MCUSInstruction) * (self->priv->instruction_count - self->priv->instruction_count % INSTRUCTION_BLOCK_SIZE + 1) * INSTRUCTION_BLOCK_SIZE);
		self->priv->instruction_count++;

		if (mcus->debug == TRUE) {
			g_debug ("Reallocating instruction memory block to %lu bytes due to having to store instruction %u (\"%s\").",
				 (gulong)(sizeof (MCUSInstruction) * (self->priv->instruction_count - self->priv->instruction_count % INSTRUCTION_BLOCK_SIZE + 1) * INSTRUCTION_BLOCK_SIZE),
				 self->priv->instruction_count,
				 mcus_instruction_data[instruction->opcode].mnemonic);
		}
	} else {
		self->priv->instruction_count++;
	}

	/* Copy the new instruction into the array */
	g_memmove (&(self->priv->instructions[self->priv->instruction_count-1]), instruction, sizeof (MCUSInstruction));

	/* Increase the compiled size */
	self->priv->compiled_size += mcus_instruction_data[instruction->opcode].size;
}

static void
skip_whitespace (MCUSCompiler *self, gboolean skip_newlines, gboolean skip_commas)
{
	gboolean in_comment = FALSE;

	while (*(self->priv->i) != '\0') {
		gchar current = *(self->priv->i);

		switch (current) {
		case ';':
			in_comment = TRUE;
		case ' ':
		case '\t':
			self->priv->i++;
			break;
		case '\n':
			in_comment = FALSE;
			if (skip_newlines == TRUE) {
				self->priv->i++;
				self->priv->line_number++;
				break;
			}
			return;
		case ',':
			if (skip_commas == TRUE || in_comment == TRUE) {
				self->priv->i++;
				break;
			}
			return;
		default:
			if (in_comment == TRUE) {
				self->priv->i++;
				break;
			}
			return;
		}
	}
}

static gboolean
lex_label (MCUSCompiler *self, MCUSLabel *label, GError **error)
{
	/* In EBNF:
	 * label-reference ::= ( alphanumeric-character | "_" ) , { alphanumeric-character | "_" }
	 * label ::= label-reference , ":" */

	guint length = 0;
	gchar *label_string;

	/* Find where the label ends and copy it to a string that length */
	while (*(self->priv->i + length) != ' ' &&
	       *(self->priv->i + length) != '\t' &&
	       *(self->priv->i + length) != '\n' &&
	       *(self->priv->i + length) != ';' &&
	       *(self->priv->i + length) != '\0') {
		length++;
	}

	/* Check we actually have a label to lex/parse */
	if (length == 0 || *(self->priv->i + length - 1) != ':') {
		gchar following_section[COMPILER_ERROR_CONTEXT_LENGTH+1] = { '\0', };
		g_memmove (following_section, self->priv->i, COMPILER_ERROR_CONTEXT_LENGTH);
		self->priv->error_length = length;

		g_set_error (error, MCUS_COMPILER_ERROR, MCUS_COMPILER_ERROR_INVALID_LABEL_DELIMITATION,
			     _("An expected label had no length, or was not delimited by a colon (\":\") around line %u before \"%s\"."),
			     self->priv->line_number,
			     following_section);
		return FALSE;
	}

	/* Make a copy of the label, excluding the colon and delimiter after it */
	label_string = g_memdup (self->priv->i, sizeof (gchar) * length);
	label_string[length - 1] = '\0';
	self->priv->i += length;

	/* Store it */
	label->label = label_string;
	label->address = self->priv->compiled_size;

	return TRUE;
}

static gboolean
lex_operand (MCUSCompiler *self, MCUSOperand *operand, GError **error)
{
	/* In EBNF:
	 * input ::= "I"
	 * output ::= "Q"
	 * register ::= "S" , ( "0" | "1" | ... | "6" | "7" )
	 * hex-digit ::= "0" | "1" | ... | "9" | "A" | ... | "F"
	 * constant ::= hex-digit , hex-digit
	 * operand ::= input | output | register | constant | label-reference
	*/

	guint length = 0;
	gchar *operand_string;

	/* Find where the operand ends and copy it to a string that length */
	while (*(self->priv->i + length) != ',' &&
	       *(self->priv->i + length) != ' ' &&
	       *(self->priv->i + length) != '\t' &&
	       *(self->priv->i + length) != '\n' &&
	       *(self->priv->i + length) != ';' &&
	       *(self->priv->i + length) != '\0') {
		length++;
	}

	/* Check we actually have an operand to lex */
	if (length == 0) {
		gchar following_section[COMPILER_ERROR_CONTEXT_LENGTH+1] = { '\0', };
		g_memmove (following_section, self->priv->i, COMPILER_ERROR_CONTEXT_LENGTH);
		self->priv->error_length = 0;

		g_set_error (error, MCUS_COMPILER_ERROR, MCUS_COMPILER_ERROR_INVALID_OPERAND,
			     _("A required operand had no length around line %u before \"%s\"."),
			     self->priv->line_number,
			     following_section);
		return FALSE;
	}

	operand_string = g_memdup (self->priv->i, sizeof (gchar) * (length + 1));
	operand_string[length] = '\0';
	self->priv->i += length;

	if (mcus->debug == TRUE)
		g_debug ("Lexing suspected operand \"%s\".", operand_string);

	/* There are several different types of operands we can lex/tokenise:
	 *  - Constant: in hexadecimal from 00..FF (e.g. "F7", "05" or "AB")
	 *  - Label: alphanumeric with underscores (e.g. "foobar", "shizzle05", "DAD" or "test_label")
	 *  - Register: "S" followed by 0..7 (e.g. "S0" or "S5")
	 *  - Input: "I" (there's only one)
	 *  - Output: "Q" (there's only one)
	 * All operands except labels are case-insensitive, just to make things harder.
	 */
	if (length == 1) {
		/* Could be anything, but hopefully it's an input or output */
		if (operand_string[0] == 'I' || operand_string[0] == 'i') {
			/* Input */
			operand->type = OPERAND_INPUT;
			operand->number = 0;
			g_free (operand_string);

			return TRUE;
		} else if (operand_string[0] == 'Q' || operand_string[0] == 'q') {
			/* Output */
			operand->type = OPERAND_OUTPUT;
			operand->number = 0;
			g_free (operand_string);

			return TRUE;
		}
	} else if (length == 2) {
		/* Could be anything, but hopefully it's a constant or register */
		if ((operand_string[0] == 'S' || operand_string[0] == 's') &&
		    g_ascii_isdigit (operand_string[1])) {
			/* Register? */
			operand->type = OPERAND_REGISTER;
			operand->number = g_ascii_digit_value (operand_string[1]);

			/* Check to see if it's valid */
			if (operand->number < REGISTER_COUNT) {
				g_free (operand_string);
				return TRUE;
			}
		}

		if (g_ascii_isxdigit (operand_string[0]) &&
		    g_ascii_isxdigit (operand_string[1])) {
			/* Constant */
			operand->type = OPERAND_CONSTANT;
			operand->number = g_ascii_xdigit_value (operand_string[0]) * 16 + g_ascii_xdigit_value (operand_string[1]);
			g_free (operand_string);

			return TRUE;
		}
	}

	/* If we're still here, it has to be a label. Resolution of the label
	 * can happen later, when we compile. */
	operand->type = OPERAND_LABEL;
	operand->label = operand_string;

	return TRUE;
}

static gboolean
lex_instruction (MCUSCompiler *self, MCUSInstruction *instruction, GError **error)
{
	/* For the purposes of lexing the instruction, it consists of the following lexemes:
	 *  - mnemonic: the human-readable representation of an opcode
	 *  - (whitespace)
	 *  - optional operand: the mnemonic form of the operand
	 *  - (optional whitespace)
	 *
	 * In EBNF:
	 * mnemonic ::= "MOVI" | "MOV" | "ADD" | "SUB" | ...
	 * whitespace ::= " " | "\t"
	 * instruction ::= mnemonic , { "," , whitespace , { whitespace } , operand } */

	gchar mnemonic_string[MAX_MNEMONIC_LENGTH+1] = { '\0', };
	guint i, length = 0;
	gchar following_section[COMPILER_ERROR_CONTEXT_LENGTH+1] = { '\0', };
	const MCUSInstructionData *instruction_data = NULL;

	while (g_ascii_isalnum (*(self->priv->i + length)) == TRUE &&
	       length < MAX_MNEMONIC_LENGTH) {
		mnemonic_string[length] = *(self->priv->i + length);
		length++;
	}

	/* If the mnemonic was zero-length or delimited by length, rather than a non-alphabetic
	 * character, check that the next character is acceptable whitespace; if it isn't, this
	 * mnemonic is invalid. This catches things like labels, which will either:
	 *  - not have whitespace after them if they're shorter than the maximum mnemonic length, or
	 *  - be longer than the maximum mnemonic length. */
	if (length == 0 ||
	    (*(self->priv->i + length) != ' ' &&
	    *(self->priv->i + length) != '\t' &&
	    *(self->priv->i + length) != '\n' &&
	    *(self->priv->i + length) != ';' &&
	    *(self->priv->i + length) != '\0')) {
		g_memmove (following_section, self->priv->i + length, COMPILER_ERROR_CONTEXT_LENGTH);
		self->priv->error_length = length;

		g_set_error (error, MCUS_COMPILER_ERROR, MCUS_COMPILER_ERROR_INVALID_MNEMONIC,
			     _("An expected mnemonic had no length, or was not delimited by whitespace around line %u before \"%s\"."),
			     self->priv->line_number,
			     following_section);
		return FALSE;
	}

	if (mcus->debug == TRUE)
		g_debug ("Lexing suspected mnemonic \"%s\".", mnemonic_string);

	/* Tokenise the mnemonic string to produce an MCUSOpcode */
	for (i = 0; i < G_N_ELEMENTS (mcus_instruction_data); i++) {
		if (strcasecmp (mnemonic_string, mcus_instruction_data[i].mnemonic) == 0) {
			instruction->opcode = mcus_instruction_data[i].opcode;
			instruction->offset = self->priv->i - self->priv->code;
			self->priv->i += length;
			instruction_data = &(mcus_instruction_data[i]);
			break;
		}
	}

	if (instruction_data == NULL) {
		/* Invalid mnemonic! */
		g_memmove (following_section, self->priv->i + length, COMPILER_ERROR_CONTEXT_LENGTH);
		self->priv->error_length = length;

		g_set_error (error, MCUS_COMPILER_ERROR, MCUS_COMPILER_ERROR_INVALID_MNEMONIC,
			     _("A mnemonic (\"%s\") did not exist around line %u before \"%s\"."),
			     mnemonic_string,
			     self->priv->line_number,
			     following_section);

		return FALSE;
	}

	skip_whitespace (self, FALSE, FALSE);

	/* Lex the operands */
	for (i = 0; i < instruction_data->arity; i++) {
		MCUSOperand operand;
		const gchar *old_i = self->priv->i;
		GError *child_error = NULL;

		if (lex_operand (self, &operand, &child_error) == TRUE) {
			/* Check the operand's type is valid */
			if ((instruction_data->operand_types[i] == OPERAND_LABEL && operand.type != OPERAND_CONSTANT && operand.type != OPERAND_LABEL) ||
			    (instruction_data->operand_types[i] != OPERAND_LABEL && operand.type != instruction_data->operand_types[i])) {
				gchar following_section[COMPILER_ERROR_CONTEXT_LENGTH+1] = { '\0', };
				g_memmove (following_section, self->priv->i, COMPILER_ERROR_CONTEXT_LENGTH);

				switch (operand.type) {
				case OPERAND_CONSTANT:
				case OPERAND_REGISTER:
					self->priv->error_length = 2;
					break;
				case OPERAND_INPUT:
				case OPERAND_OUTPUT:
					self->priv->error_length = 1;
					break;
				default:
					self->priv->error_length = strlen (operand.label);
					break;
				}

				g_set_error (error, MCUS_COMPILER_ERROR, MCUS_COMPILER_ERROR_INVALID_OPERAND_TYPE,
					     _("An operand was of type \"%s\" when it should've been \"%s\" around line %u before \"%s\".\n\n%s"),
					     operand_type_to_string (operand.type),
					     operand_type_to_string (instruction_data->operand_types[i]),
					     self->priv->line_number,
					     following_section,
					     instruction_data->help);

				/* Restore i to before the operand so that error highlighting works correctly */
				self->priv->i = old_i;

				return FALSE;
			}

			/* Store the operand */
			instruction->operands[i] = operand;
			skip_whitespace (self, FALSE, TRUE);
		} else {
			/* Throw the error */
			g_propagate_error (error, child_error);
			return FALSE;
		}
	}

	/* Work out the instruction's length for future use */
	instruction->length = self->priv->i - self->priv->code - instruction->offset;

	return TRUE;
}

static gboolean
lex_directive (MCUSCompiler *self, MCUSDirective *directive, GError **error)
{
	/* For the purposes of lexing the directive, it consists of the following lexemes:
	 *  - directive_name: the human-readable name
	 *  - (whitespace)
	 *  - optional operand: the mnemonic form of the operand
	 *  - (optional whitespace)
	 *
	 * In EBNF:
	 * directive_name ::= "SET"
	 * whitespace ::= " " | "\t"
	 * directive ::= directive_name , { "," , whitespace , { whitespace } , operand } */

	gchar name_string[MAX_DIRECTIVE_NAME_LENGTH+1] = { '\0', };
	guint i, length = 0;
	gchar following_section[COMPILER_ERROR_CONTEXT_LENGTH+1] = { '\0', };
	const MCUSDirectiveData *directive_data = NULL;
	guchar *memory_address = NULL;

	if (*(self->priv->i) != '$') {
		g_memmove (following_section, self->priv->i, COMPILER_ERROR_CONTEXT_LENGTH);

		self->priv->error_length = 0;
		while (g_ascii_isalnum (*(self->priv->i + self->priv->error_length)) == TRUE)
			self->priv->error_length++;

		g_set_error (error, MCUS_COMPILER_ERROR, MCUS_COMPILER_ERROR_INVALID_DIRECTIVE_DELIMITATION,
			     _("An expected directive name was not delimited by a dollar sign around line %u before \"%s\"."),
			     self->priv->line_number,
			     following_section);
		return FALSE;
	}

	/* Skip over the dollar sign */
	self->priv->i++;

	/* Grab the directive name */
	while (g_ascii_isalnum (*(self->priv->i + length)) == TRUE &&
	       length < MAX_DIRECTIVE_NAME_LENGTH) {
		name_string[length] = *(self->priv->i + length);
		length++;
	}

	/* If the name was zero-length or delimited by length, rather than a non-alphabetic
	 * character, check that the next character is acceptable whitespace; if it isn't, this
	 * name is invalid. This catches things like labels, which will either:
	 *  - not have whitespace after them if they're shorter than the maximum name length, or
	 *  - be longer than the maximum name length. */
	if (length == 0 ||
	    (*(self->priv->i + length) != ' ' &&
	    *(self->priv->i + length) != '\t' &&
	    *(self->priv->i + length) != '\n' &&
	    *(self->priv->i + length) != ';' &&
	    *(self->priv->i + length) != '\0')) {
		g_memmove (following_section, self->priv->i + length, COMPILER_ERROR_CONTEXT_LENGTH);
		self->priv->error_length = length;

		g_set_error (error, MCUS_COMPILER_ERROR, MCUS_COMPILER_ERROR_INVALID_DIRECTIVE_NAME,
			     _("An expected directive name had no length, or was not delimited by whitespace around line %u before \"%s\"."),
			     self->priv->line_number,
			     following_section);
		return FALSE;
	}

	if (mcus->debug == TRUE)
		g_debug ("Lexing suspected directive name \"%s\".", name_string);

	/* Tokenise the mnemonic string to produce an MCUSDirective */
	for (i = 0; i < G_N_ELEMENTS (mcus_directive_data); i++) {
		if (strcasecmp (name_string, mcus_directive_data[i].directive_name) == 0) {
			if (directive != NULL)
				*directive = mcus_directive_data[i].directive;
			directive_data = &(mcus_directive_data[i]);

			/* Make sure to blank out the directive name in the code so it doesn't confuse the other lexers */
			/* The -1 and +1 adjustments are so the dollar sign is included */
			memset ((gchar *)self->priv->i - 1, ' ', length + 1);
			self->priv->i += length;
			break;
		}
	}

	if (directive_data == NULL) {
		/* Invalid directive name! */
		g_memmove (following_section, self->priv->i + length, COMPILER_ERROR_CONTEXT_LENGTH);
		self->priv->error_length = length;

		g_set_error (error, MCUS_COMPILER_ERROR, MCUS_COMPILER_ERROR_INVALID_DIRECTIVE_NAME,
			     _("A directive name (\"%s\") did not exist around line %u before \"%s\"."),
			     name_string,
			     self->priv->line_number,
			     following_section);

		return FALSE;
	}

	skip_whitespace (self, FALSE, FALSE);

	/* Hard-code the operand code for the SET directive for now, since it's the only directive */
	for (i = 0; i < 2; i++) {
		MCUSOperand operand;
		const gchar *old_i = self->priv->i;
		GError *child_error = NULL;

		if (lex_operand (self, &operand, &child_error) == TRUE) {
			/* Check the operand's type is valid */
			if (operand.type != OPERAND_CONSTANT) {
				gchar following_section[COMPILER_ERROR_CONTEXT_LENGTH+1] = { '\0', };
				g_memmove (following_section, self->priv->i, COMPILER_ERROR_CONTEXT_LENGTH);

				switch (operand.type) {
				case OPERAND_CONSTANT:
				case OPERAND_REGISTER:
					self->priv->error_length = 2;
					break;
				case OPERAND_INPUT:
				case OPERAND_OUTPUT:
					self->priv->error_length = 1;
					break;
				default:
					self->priv->error_length = strlen (operand.label);
					break;
				}

				g_set_error (error, MCUS_COMPILER_ERROR, MCUS_COMPILER_ERROR_INVALID_OPERAND_TYPE,
					     _("An operand was of type \"%s\" when it should've been \"%s\" around line %u before \"%s\".\n\n%s"),
					     operand_type_to_string (operand.type),
					     operand_type_to_string (OPERAND_CONSTANT),
					     self->priv->line_number,
					     following_section,
					     _("$SET 00, 00 — set the memory address specified by the first operand to the value in the second."));

				/* Restore i to before the operand so that error highlighting works correctly */
				self->priv->i = old_i;

				return FALSE;
			}

			/* Either set the memory address or its value */
			if (i == 0)
				memory_address = mcus->memory + operand.number;
			else
				*memory_address = operand.number;

			/* Make sure to blank out the operands once done */
			memset ((gchar *)old_i, ' ', self->priv->i - old_i);

			skip_whitespace (self, FALSE, TRUE);
		} else {
			/* Throw the error */
			g_propagate_error (error, child_error);
			return FALSE;
		}
	}

	return TRUE;
}

gboolean
mcus_compiler_parse (MCUSCompiler *self, const gchar *code, GError **error)
{
	/* In EBNF:
	 * comment ::= ";" , ? any characters ? , "\n"
	 * terminating-whitespace ::= comment | whitespace | "\n"
	 * assembly ::= { terminating-whitespace , { terminating-whitespace } , ( instruction | label ) } , { terminating-whitespace } , "\0" */

	/* Set up parser variables */
	reset_state (self);
	self->priv->code = code;
	self->priv->i = code;
	self->priv->dirty = TRUE;

	skip_whitespace (self, TRUE, FALSE);

	while (*(self->priv->i) != '\0') {
		MCUSInstruction instruction;
		MCUSLabel label;
		GError *child_error = NULL;

		/* Are we finished? */
		if (*(self->priv->i) == '\0')
			break;

		if (lex_directive (self, NULL, &child_error) == TRUE) {
			/* Directive */
			skip_whitespace (self, TRUE, FALSE);
			continue;
		} else if (g_error_matches (child_error, MCUS_COMPILER_ERROR, MCUS_COMPILER_ERROR_INVALID_DIRECTIVE_DELIMITATION) == FALSE) {
			goto throw_error;
		} else {
			g_clear_error (&child_error);
		}

		if (lex_label (self, &label, &child_error) == TRUE) {
			/* Label */
			store_label (self, &label);
			skip_whitespace (self, TRUE, FALSE);
			continue;
		} else if (g_error_matches (child_error, MCUS_COMPILER_ERROR, MCUS_COMPILER_ERROR_INVALID_LABEL_DELIMITATION) == FALSE) {
			goto throw_error;
		} else {
			g_clear_error (&child_error);
		}

		if (lex_instruction (self, &instruction, &child_error) == TRUE) {
			/* Instruction */
			store_instruction (self, &instruction);
			skip_whitespace (self, TRUE, FALSE);
			continue;
		}

throw_error:
		/* Throw the error */
		g_propagate_error (error, child_error);
		return FALSE;
	}

	return TRUE;
}

gboolean
mcus_compiler_compile (MCUSCompiler *self, GError **error)
{
	guint i;
	self->priv->dirty = TRUE;

	/* Allocate the line number map's memory */
	g_free (mcus->offset_map);
	mcus->offset_map = g_malloc (sizeof (*mcus->offset_map) * (self->priv->compiled_size + 1));

	if (mcus->debug == TRUE)
		g_debug ("Allocating line number map of %lu bytes.", (gulong)(sizeof (guint) * self->priv->compiled_size));

	/* Compile it to memory */
	self->priv->compiled_size = PROGRAM_START_ADDRESS;

	for (i = 0; i < self->priv->instruction_count; i++) {
		guint f, projected_size;
		const MCUSInstructionData *instruction_data;
		MCUSInstruction *instruction;

		instruction = &(self->priv->instructions[i]);
		instruction_data = &(mcus_instruction_data[instruction->opcode]);

		/* In case of error, ensure the correct part of the code will be highlighted */
		self->priv->i = self->priv->code + instruction->offset;

		/* Check we're not overflowing memory */
		projected_size = self->priv->compiled_size + instruction_data->size;

		if (projected_size >= MEMORY_SIZE) {
			self->priv->error_length = strlen (instruction_data->mnemonic);
			g_set_error (error, MCUS_COMPILER_ERROR, MCUS_COMPILER_ERROR_MEMORY_OVERFLOW,
				     _("Instruction %u overflows the microcontroller memory."),
				     i);
			return FALSE;
		}

		/* Store the line number mapping for the instruction */
		mcus->offset_map[self->priv->compiled_size].offset = instruction->offset;
		mcus->offset_map[self->priv->compiled_size].length = instruction->length;

		/* Store the opcode first, as that's easy */
		mcus->memory[self->priv->compiled_size++] = instruction->opcode;
		self->priv->i += strlen (instruction_data->mnemonic) + 1;

		/* Store the operands, although we have to special-case IN and OUT instructions */
		switch (instruction->opcode) {
		case OPCODE_IN:
			mcus->memory[self->priv->compiled_size++] = instruction->operands[0].number;
			break;
		case OPCODE_OUT:
			mcus->memory[self->priv->compiled_size++] = instruction->operands[1].number;
			break;
		default:
			for (f = 0; f < instruction_data->arity; f++) {
				if (instruction_data->operand_types[f] == OPERAND_LABEL &&
				    instruction->operands[f].type == OPERAND_LABEL) {
					GError *child_error = NULL;

					/* We need to resolve the label first */
					mcus->memory[self->priv->compiled_size++] = resolve_label (self, i, instruction->operands[f].label, &child_error);
					g_free (instruction->operands[f].label);

					if (child_error != NULL) {
						g_propagate_error (error, child_error);
						return FALSE;
					}
				} else {
					/* Just store the operand */
					mcus->memory[self->priv->compiled_size++] = instruction->operands[f].number;
				}
			}
		}
	}

	/* Set the last element in the line number map to -1 for safety */
	mcus->offset_map[self->priv->compiled_size].offset = -1;
	mcus->offset_map[self->priv->compiled_size].length = 0;

	reset_state (self);

	return TRUE;
}

guint
mcus_compiler_get_offset (MCUSCompiler *self)
{
	return self->priv->i - self->priv->code;
}

void
mcus_compiler_get_error_location (MCUSCompiler *self, guint *start, guint *end)
{
	if (start != NULL)
		*start = mcus_compiler_get_offset (self);
	if (end != NULL)
		*end = mcus_compiler_get_offset (self) + self->priv->error_length;
}
