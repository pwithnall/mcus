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

#include "parser.h"

typedef struct {
	gchar *label;
	gchar location;
} MCUSLabel;

typedef struct {
	gboolean is_label;
	union {
		gchar number;
		gchar *label;
	};
} MCUSOperand;

static void mcus_parser_init (MCUSParser *self);
static void mcus_parser_dispose (GObject *object);
static void mcus_parser_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void mcus_parser_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

struct _MCUSParserPrivate {
	MCUSLabel *labels;
	gchar *code;
	gchar *i;
};

/*
TODO
enum {
	PROP_DEV_PATH = 1
};
*/

G_DEFINE_TYPE (MCUSParser, mcus_parser, G_TYPE_OBJECT)
#define MCUS_PARSER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MCUS_TYPE_PARSER, MCUSParserPrivate))

static void
mcus_parser_class_init (MCUSParserClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (MCUSParserPrivate));

	gobject_class->set_property = mcus_parser_set_property;
	gobject_class->get_property = mcus_parser_get_property;
	gobject_class->dispose = mcus_parser_dispose;

	/*
	TODO
	g_object_class_install_property (gobject_class, PROP_DEV_PATH,
				g_param_spec_string ("dev-path",
					"Device path", "The path to this connection's device node.",
					NULL,
					G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	*/
}

static void
mcus_parser_init (MCUSParser *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, MCUS_TYPE_PARSER, MCUSParserPrivate);
	self->priv->dispose_has_run = FALSE;
	/*
	TODO
	self->priv->dev_path = NULL;
	*/
}

static void
mcus_parser_dispose (GObject *object)
{
	MCUSParserPrivate *priv = MCUS_PARSER_GET_PRIVATE (object);

	/* Make sure we only run once */
	if (priv->dispose_has_run)
		return;
	priv->dispose_has_run = TRUE;

	/*
	TODO
	g_free (priv->dev_path);
	*/

	/* Chain up to the parent class */
	G_OBJECT_CLASS (mcus_parser_parent_class)->dispose (object);
}

static void
mcus_parser_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	MCUSParserPrivate *priv = MCUS_PARSER_GET_PRIVATE (object);

	switch (property_id) {
		/*
		TODO
		case PROP_DEV_PATH:
			g_value_set_string (value, g_strdup (priv->dev_path));
			break;
		*/
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
mcus_parser_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	MCUSParserPrivate *priv = MCUS_PARSER_GET_PRIVATE (object);

	switch (property_id) {
		/*
		TODO
		case PROP_DEV_PATH:
			g_free (priv->dev_path);
			priv->dev_path = g_strdup (g_value_get_string (value));
			break;
		*/
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

/* TODO: For the moment, I'm ignoring comments, but support for them will have to be included eventually */

static void
skip_whitespace (MCUSParser *self, gboolean skip_newlines)
{
	while (*(self->priv->i) == ' ' ||
	       *(self->priv->i) == '\t' ||
	       *(self->priv->i) == '\n' && skip_newlines == TRUE) {
		self->priv->i++;
	}
}

static gboolean
extract_instruction (MCUSParser *self, MCUSInstructionType *instruction_type)
{
	gchar instruction[MAX_INSTRUCTION_LENGTH];
	guint i = 0;

	while (g_ascii_isalpha (*(self->priv->i)) == TRUE && i < MAX_INSTRUCTION_LENGTH)
		instruction[i++] = self->priv->i++;

	/* If the instruction was zero-length or delimited by length, rather than a non-alphabetic
	 * character, check that the next character is acceptable whitespace; if it isn't, this
	 * instruction is invalid. */
	/* TODO: I'm sure this could be improved */
	if (i == 0 ||
	    (i == MAX_INSTRUCTION_LENGTH &&
	    *(self->priv->i + 1) != ' ' &&
	    *(self->priv->i + 1) != '\t')) {
		instruction_type = NULL;
		return FALSE;
		/* TODO: Return a GError? */
	}

	/* Convert the instruction string to a MCUSInstructionType */
	for (i = 0; i < G_N_ELEMENTS (mcus_instruction_data); i++) {
		if (stricmp (&instruction, mcus_instruction_data[i].instruction_name) == 0) {
			*instruction_type = mcus_instruction_data[i].type;
			return TRUE;
	}

	/* Invalid instruction! */
	instruction_type == NULL;
	return FALSE;
}

static gboolean
extract_operand (MCUSParser *self, MCUSOperand *operand)
{
	guint i = 0;
	gchar *operand_string;

	/* Find where the operand ends and copy it to a string that length */
	while (*(self->priv->i + i) != ' ' &&
	       *(self->priv->i + i) != '\t' &&
	       *(self->priv->i + i) != '\n' &&
	       *(self->priv->i + i) != '\0') {
		i++;
	}

	operand_string = g_memdup (self->priv->i, sizeof (gchar) * i);

	/* Check we actually have an operand to parse */
	if (operand_string == NULL)
		return FALSE;

	/* TODO: How do we differentiate between labels and hexadecimal values? e.g. Between the label "dad", and the value #dad? */
}

gboolean
mcus_parser_parse (MCUSParser *self, const gchar *code, GError **error)
{
	/* Set up parser variables */
	self->priv->code = code;
	self->priv->i = code;

	while (self->priv->i != NULL) {
		MCUSInstructionType instruction_type;

		if (extract_instruction (self, &instruction_type)) {
			MCUSInstruction *instruction;
			guint i;

			instruction = mcus_instruction_new (instruction_type);
			for (i = 0; i < mcus_instruction_data[instruction_type].operand_count; i++) {
				MCUSOperand operand;
				if (extract_operand (self, &operand)) {
					mcus_instruction_set_operand (instruction, i, &operand);
					skip_whitespace (self, FALSE);
				}
				/*else
					TODO: throw error */
			}

			skip_whitespace (self, TRUE);
		}
		/*else
			TODO: throw error */
	}

	return TRUE;
}
