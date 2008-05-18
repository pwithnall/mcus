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

#include "parser.h"
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
	MCUSInstructionType type;
	MCUSOperand operands[MAX_OPERAND_COUNT];
} MCUSInstruction;

static void mcus_parser_init (MCUSParser *self);
static void mcus_parser_finalize (GObject *object);
static void reset_state (MCUSParser *self);

#define LABEL_BLOCK_SIZE 5
#define INSTRUCTION_BLOCK_SIZE 10

struct _MCUSParserPrivate {
	MCUSLabel *labels;
	guint label_count;

	MCUSInstruction *instructions;
	guint instruction_count;

	gchar *code;
	gchar *i;

	guchar compiled_size;
	guint line_number;
};

G_DEFINE_TYPE (MCUSParser, mcus_parser, G_TYPE_OBJECT)
#define MCUS_PARSER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MCUS_TYPE_PARSER, MCUSParserPrivate))

static void
mcus_parser_class_init (MCUSParserClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	g_type_class_add_private (klass, sizeof (MCUSParserPrivate));
	gobject_class->finalize = mcus_parser_finalize;
}

static void
mcus_parser_init (MCUSParser *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, MCUS_TYPE_PARSER, MCUSParserPrivate);
}

static void
mcus_parser_finalize (GObject *object)
{
	reset_state (MCUS_PARSER (object));

	/* Chain up to the parent class */
	G_OBJECT_CLASS (mcus_parser_parent_class)->finalize (object);
}

GQuark
mcus_parser_error_quark (void)
{
	static GQuark q = 0;

	if (q == 0)
		q = g_quark_from_static_string ("mcus-parser-error-quark");

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
reset_state (MCUSParser *self)
{
	guint i;

	/* Free labels and instructions and reset state */
	for (i = 0; i < self->priv->label_count; i++)
		g_free (self->priv->labels[i].label);

	g_free (self->priv->labels);
	g_free (self->priv->instructions);

	self->priv->label_count = 0;
	self->priv->instruction_count = 0;
	self->priv->compiled_size = 0;
	self->priv->line_number = 0;

	self->priv->i = NULL;
}

static void
store_label (MCUSParser *self, const MCUSLabel *label)
{
	/* If our label array is full, extend it */
	if (self->priv->label_count % LABEL_BLOCK_SIZE == 0) {
		self->priv->labels = g_realloc (self->priv->labels,
						sizeof (MCUSLabel) * ceil (++(self->priv->label_count) / LABEL_BLOCK_SIZE) * LABEL_BLOCK_SIZE);
	}

	/* Copy the new label into the array */
	g_memmove (&(self->priv->labels[self->priv->label_count-1]), label, sizeof (MCUSLabel));
}

static guchar
resolve_label (MCUSParser *self, guint instruction_number, const gchar *label_string, GError **error)
{
	guint i;

	for (i = 0; i < self->priv->label_count; i++) {
		if (strcmp (label_string, self->priv->labels[i].label) == 0)
			return self->priv->labels[i].address;
	}

	g_set_error (error, MCUS_PARSER_ERROR, MCUS_PARSER_ERROR_UNRESOLVABLE_LABEL,
		     _("A label (\"%s\") could not be resolved to an address for instruction %u."),
		     label_string,
		     instruction_number + 1);
	return 0;
}

static void
store_instruction (MCUSParser *self, const MCUSInstruction *instruction)
{
	/* If our instruction array is full, extend it */
	if (self->priv->instruction_count % INSTRUCTION_BLOCK_SIZE == 0) {
		self->priv->instructions = g_realloc (self->priv->instructions,
						      sizeof (MCUSInstruction) * ceil (++(self->priv->instruction_count) / INSTRUCTION_BLOCK_SIZE) * INSTRUCTION_BLOCK_SIZE);
	}

	/* Copy the new instruction into the array */
	g_memmove (&(self->priv->instructions[self->priv->instruction_count-1]), instruction, sizeof (MCUSInstruction));
}

/* TODO: For the moment, I'm ignoring comments, but support for them will have to be included eventually */
/* TODO: Subroutine support */

static void
skip_whitespace (MCUSParser *self, gboolean skip_newlines)
{
	while (*(self->priv->i) == ' ' ||
	       *(self->priv->i) == '\t' ||
	       (*(self->priv->i) == '\n' && skip_newlines == TRUE && self->priv->line_number++)) {
		self->priv->i++;
	}
}

static gboolean
extract_label (MCUSParser *self, MCUSLabel *label, GError **error)
{
	guint length = 0;
	gchar *label_string;

	/* Find where the label ends and copy it to a string that length */
	while (*(self->priv->i + length) != ' ' &&
	       *(self->priv->i + length) != '\t' &&
	       *(self->priv->i + length) != '\n' &&
	       *(self->priv->i + length) != '\0') {
		length++;
	}

	/* Check we actually have a label to parse */
	if (length == 0 || *(self->priv->i + length - 1) != ':') {
		g_set_error (error, MCUS_PARSER_ERROR, MCUS_PARSER_ERROR_INVALID_LABEL,
			     _("An expected label had no length, or was not delimited by a colon (\":\") around line %u."),
			     self->priv->line_number);
		return FALSE;
	}

	/* Make a copy of the label, excluding the colon and delimiter after it */
	label_string = g_memdup (self->priv->i, sizeof (gchar) * (length - 2));
	self->priv->i += length;

	/* Store it */
	label->label = label_string;
	label->address = self->priv->compiled_size;

	return TRUE;
}

static gboolean
extract_instruction (MCUSParser *self, MCUSInstructionType *instruction_type, GError **error)
{
	gchar instruction[MAX_INSTRUCTION_LENGTH];
	guint i, length = 0;

	while (g_ascii_isalpha (*(self->priv->i + length)) == TRUE &&
	       length < MAX_INSTRUCTION_LENGTH) {
		instruction[length] = *(self->priv->i + length);
		length++;
	}

	/* If the instruction was zero-length or delimited by length, rather than a non-alphabetic
	 * character, check that the next character is acceptable whitespace; if it isn't, this
	 * instruction is invalid. This catches things like labels, which will either:
	 *  - not have whitespace after them if they're shorter than the maximum instruction length, or
	 *  - be longer than the maximum instruction length. */
	if (length == 0 ||
	    (*(self->priv->i + length) != ' ' &&
	    *(self->priv->i + length) != '\t' &&
	    *(self->priv->i + length) != '\n' &&
	    *(self->priv->i + length) != '\0')) {
		instruction_type = NULL;
		g_set_error (error, MCUS_PARSER_ERROR, MCUS_PARSER_ERROR_INVALID_INSTRUCTION,
			     _("An expected instruction had no length, or was not delimited by whitespace around line %u."),
			     self->priv->line_number);
		return FALSE;
	}

	/* Convert the instruction string to a MCUSInstructionType */
	for (i = 0; i < G_N_ELEMENTS (mcus_instruction_data); i++) {
		if (strcasecmp (instruction, mcus_instruction_data[i].instruction_name) == 0) {
			*instruction_type = mcus_instruction_data[i].opcode;
			self->priv->i += length;
			return TRUE;
		}
	}

	/* Invalid instruction! */
	instruction_type = NULL;
	g_set_error (error, MCUS_PARSER_ERROR, MCUS_PARSER_ERROR_INVALID_INSTRUCTION,
		     _("An instruction (\"%.*s\") did not exist around line %u."),
		     length, instruction,
		     self->priv->line_number);

	return FALSE;
}

static gboolean
extract_operand (MCUSParser *self, MCUSOperand *operand, GError **error)
{
	guint length = 0;
	gchar *operand_string;

	/* Find where the operand ends and copy it to a string that length */
	while (*(self->priv->i + length) != ' ' &&
	       *(self->priv->i + length) != '\t' &&
	       *(self->priv->i + length) != '\n' &&
	       *(self->priv->i + length) != '\0') {
		length++;
	}

	/* Check we actually have an operand to parse */
	if (length == 0) {
		g_set_error (error, MCUS_PARSER_ERROR, MCUS_PARSER_ERROR_INVALID_OPERAND,
			     _("A required operand had no length around line %u."),
			     self->priv->line_number);
		return FALSE;
	}

	operand_string = g_memdup (self->priv->i, sizeof (gchar) * length);
	self->priv->i += length;

	/* There are several different types of operands we can parse:
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

gboolean
mcus_parser_parse (MCUSParser *self, const gchar *code, GError **error)
{
	/* Set up parser variables */
	reset_state (self);
	self->priv->i = (gchar*)code;

	while (self->priv->i != NULL) {
		MCUSInstruction instruction;
		MCUSLabel label;
		GError *child_error = NULL;

		if (extract_instruction (self, &(instruction.type), NULL) == TRUE) {
			/* Instruction */
			guint i;
			const MCUSInstructionData *instruction_data = &(mcus_instruction_data[instruction.type]);

			for (i = 0; i < instruction_data->operand_count; i++) {
				MCUSOperand operand;

				if (extract_operand (self, &operand, &child_error) == TRUE) {
					/* Check the operand's type is valid */
					if ((instruction_data->operand_types[i] == OPERAND_LABEL &&
					    (operand.type != OPERAND_CONSTANT ||
					    operand.type != OPERAND_LABEL)) ||
					    (instruction_data->operand_types[i] != OPERAND_LABEL &&
					    operand.type != instruction_data->operand_types[i])) {
						g_set_error (error, MCUS_PARSER_ERROR, MCUS_PARSER_ERROR_INVALID_OPERAND_TYPE,
							     _("An operand was of type \"%s\" when it should've been \"%s\" around line %u."),
							     operand_type_to_string (operand.type),
							     operand_type_to_string (instruction_data->operand_types[i]),
							     self->priv->line_number);
						return FALSE;
					}

					/* Store the operand */
					instruction.operands[i] = operand;
					skip_whitespace (self, FALSE);
				} else {
					/* Throw the error */
					g_propagate_error (error, child_error);
					return FALSE;
				}
			}

			store_instruction (self, &instruction);
			skip_whitespace (self, TRUE);
		} else if (extract_label (self, &label, &child_error) == TRUE) {
			/* Label */
			store_label (self, &label);
			skip_whitespace (self, TRUE);
		} else {
			/* Throw the error */
			g_propagate_error (error, child_error);
			return FALSE;
		}
	}

	return TRUE;
}

/* TODO: Line numbers */

gboolean
mcus_parser_compile (MCUSParser *self, GError **error)
{
	guint i;
	self->priv->compiled_size = PROGRAM_START_ADDRESS;

	for (i = 0; i < self->priv->instruction_count; i++) {
		guint f, projected_size;
		const MCUSInstructionData *instruction_data;
		MCUSInstruction *instruction;

		instruction = &(self->priv->instructions[i]);
		instruction_data = &(mcus_instruction_data[instruction->type]);

		/* Check we're not overflowing memory */
		projected_size = self->priv->compiled_size + 1 + instruction_data->operand_count;
		if (instruction->type == INSTRUCTION_IN || instruction->type == INSTRUCTION_OUT)
			projected_size--;

		if (projected_size >= MEMORY_SIZE) {
			g_set_error (error, MCUS_PARSER_ERROR, MCUS_PARSER_ERROR_MEMORY_OVERFLOW,
				     _("Instruction %u overflows the microcontroller memory."),
				     i);
			return FALSE;
		}

		/* Store the opcode first, as that's easy */
		mcus->memory[self->priv->compiled_size++] = instruction->type;

		/* Store the operands, although we have to special-case IN and OUT instructions */
		switch (instruction->type) {
		case INSTRUCTION_IN:
			mcus->memory[self->priv->compiled_size++] = instruction->operands[0].number;
			break;
		case INSTRUCTION_OUT:
			mcus->memory[self->priv->compiled_size++] = instruction->operands[1].number;
			break;
		default:
			for (f = 0; f < instruction_data->operand_count; f++) {
				if (instruction_data->operand_types[f] == OPERAND_LABEL) {
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

	reset_state (self);

	return TRUE;
}

