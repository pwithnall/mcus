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

#include "instruction.h"

static void mcus_instruction_init (MCUSInstruction *self);
static void mcus_instruction_dispose (GObject *object);
static void mcus_instruction_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void mcus_instruction_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

struct _MCUSInstructionPrivate {
	MCUSInstructionType type;
	gchar operand1;
	gchar operand2;
};

/*
TODO
enum {
	PROP_DEV_PATH = 1
};
*/

G_DEFINE_TYPE (MCUSInstruction, mcus_instruction, G_TYPE_OBJECT)
#define MCUS_INSTRUCTION_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MCUS_TYPE_INSTRUCTION, MCUSInstructionPrivate))

static void
mcus_instruction_class_init (MCUSInstructionClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (MCUSInstructionPrivate));

	gobject_class->set_property = mcus_instruction_set_property;
	gobject_class->get_property = mcus_instruction_get_property;
	gobject_class->dispose = mcus_instruction_dispose;

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
mcus_instruction_init (MCUSInstruction *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, MCUS_TYPE_INSTRUCTION, MCUSInstructionPrivate);
	self->priv->dispose_has_run = FALSE;
	/*
	TODO
	self->priv->dev_path = NULL;
	*/
}

static void
mcus_instruction_dispose (GObject *object)
{
	MCUSInstructionPrivate *priv = MCUS_INSTRUCTION_GET_PRIVATE (object);

	/* Make sure we only run once */
	if (priv->dispose_has_run)
		return;
	priv->dispose_has_run = TRUE;

	/*
	TODO
	g_free (priv->dev_path);
	*/

	/* Chain up to the parent class */
	G_OBJECT_CLASS (mcus_instruction_parent_class)->dispose (object);
}

static void
mcus_instruction_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	MCUSInstructionPrivate *priv = MCUS_INSTRUCTION_GET_PRIVATE (object);

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
mcus_instruction_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	MCUSInstructionPrivate *priv = MCUS_INSTRUCTION_GET_PRIVATE (object);

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

