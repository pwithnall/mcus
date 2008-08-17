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
#include <glib/gprintf.h>
#include <glib/gi18n.h>
#include <string.h>

#include "input-port.h"
#include "main.h"

GQuark
mcus_io_error_quark (void)
{
	static GQuark q = 0;

	if (q == 0)
		q = g_quark_from_static_string ("mcus-io-error-quark");

	return q;
}

gboolean
mcus_input_port_read_entry (GError **error)
{
	gint digit_value;
	const gchar *entry_text;

	entry_text = gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (mcus->builder, "mw_input_port_entry")));
	if (strlen (entry_text) != 2) {
		g_set_error (error, MCUS_IO_ERROR, MCUS_IO_ERROR_INPUT,
			     _("The input port value was not two digits long."));
		return FALSE;
	}

	/* Deal with the first digit */
	digit_value = g_ascii_xdigit_value (entry_text[0]);
	if (digit_value == -1) {
		g_set_error (error, MCUS_IO_ERROR, MCUS_IO_ERROR_INPUT,
			     _("The input port contained a non-hexadecimal digit (\"%c\")."),
			     entry_text[0]);
		return FALSE;
	}
	mcus->input_port = digit_value * 16;

	/* Deal with the second digit */
	digit_value = g_ascii_xdigit_value (entry_text[1]);
	if (digit_value == -1) {
		g_set_error (error, MCUS_IO_ERROR, MCUS_IO_ERROR_INPUT,
			     _("The input port contained a non-hexadecimal digit (\"%c\")."),
			     entry_text[1]);
		return FALSE;
	}
	mcus->input_port += digit_value;

	return TRUE;
}

void
mcus_input_port_update_entry (void)
{
	gchar *output = g_strdup_printf ("%02X", mcus->input_port);
	gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (mcus->builder, "mw_input_port_entry")), output);
	g_free (output);
}

void
mcus_input_port_read_check_buttons (void)
{
	guint i;

	/* Clear the old value */
	mcus->input_port = 0;

	for (i = 0; i < 7; i++) {
		GtkToggleButton *button;
		gchar button_id[24];

		/* Grab the control */
		g_sprintf (button_id, "mw_input_check_button_%u", i);
		button = GTK_TOGGLE_BUTTON (gtk_builder_get_object (mcus->builder, button_id));

		/* Shift the new bit in as the LSB */
		mcus->input_port = gtk_toggle_button_get_active (button) | (mcus->input_port << 1);
	}
}

void
mcus_input_port_update_check_buttons (void)
{
	guint i;

	for (i = 0; i < 7; i++) {
		GtkToggleButton *button;
		gchar button_id[24];

		/* Grab the control */
		g_sprintf (button_id, "mw_input_check_button_%u", i);
		button = GTK_TOGGLE_BUTTON (gtk_builder_get_object (mcus->builder, button_id));

		/* Mask out everything except the interesting bit */
		gtk_toggle_button_set_active (button, mcus->input_port & (1 << i));
	}
}
