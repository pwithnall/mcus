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
#include <gtk/gtk.h>

#include "parser.h"
#include "main.h"
#include "simulation.h"
#include "interface.h"

gboolean
mw_delete_event_cb (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	mcus_quit ();
	return TRUE;
}

void
mw_quit_activate_cb (GtkAction *self, gpointer user_data)
{
	mcus_quit ();
}

void
mw_cut_activate_cb (GtkAction *self, gpointer user_data)
{
	GtkClipboard *clipboard = gtk_clipboard_get_for_display (gtk_widget_get_display (GTK_WIDGET (mcus->main_window)), GDK_SELECTION_CLIPBOARD);
	gtk_text_buffer_cut_clipboard (GTK_TEXT_BUFFER (gtk_builder_get_object (mcus->builder, "mw_code_buffer")), clipboard, TRUE);
}

void
mw_copy_activate_cb (GtkAction *self, gpointer user_data)
{
	GtkClipboard *clipboard = gtk_clipboard_get_for_display (gtk_widget_get_display (GTK_WIDGET (mcus->main_window)), GDK_SELECTION_CLIPBOARD);
	gtk_text_buffer_copy_clipboard (GTK_TEXT_BUFFER (gtk_builder_get_object (mcus->builder, "mw_code_buffer")), clipboard);
}

void
mw_paste_activate_cb (GtkAction *self, gpointer user_data)
{
	GtkClipboard *clipboard = gtk_clipboard_get_for_display (gtk_widget_get_display (GTK_WIDGET (mcus->main_window)), GDK_SELECTION_CLIPBOARD);
	gtk_text_buffer_paste_clipboard (GTK_TEXT_BUFFER (gtk_builder_get_object (mcus->builder, "mw_code_buffer")), clipboard, NULL, TRUE);
}

void
mw_delete_activate_cb (GtkAction *self, gpointer user_data)
{
	gtk_text_buffer_delete_selection (GTK_TEXT_BUFFER (gtk_builder_get_object (mcus->builder, "mw_code_buffer")), TRUE, TRUE);
}

void
mw_compile_activate_cb (GtkAction *self, gpointer user_data)
{
	MCUSParser *parser;
	GtkTextBuffer *code_buffer;
	GtkTextIter start_iter, end_iter;
	gchar *code;
	GError *error = NULL;

	/* Get the assembly code */
	code_buffer = GTK_TEXT_BUFFER (gtk_builder_get_object (mcus->builder, "mw_code_buffer"));
	gtk_text_buffer_get_bounds (code_buffer, &start_iter, &end_iter);
	code = gtk_text_buffer_get_text (code_buffer, &start_iter, &end_iter, FALSE);

	mcus_print_debug_data ();

	/* Parse it */
	parser = mcus_parser_new ();
	mcus_parser_parse (parser, code, &error);

	if (error != NULL) {
		g_error (error->message); /* TODO */
		g_error_free (error);
		return;
	}

	g_free (code);
	mcus_print_debug_data ();

	/* Compile it */
	mcus_parser_compile (parser, &error);

	if (error != NULL) {
		g_error (error->message); /* TODO */
		g_error_free (error);
		return;
	}

	mcus_simulation_update_ui ();
}

static gboolean
simulation_iterate_cb (gpointer user_data)
{
	GError *error = NULL;

	if (mcus->simulation_state != SIMULATION_RUNNING)
		return FALSE;

	mcus_read_input_port ();
	mcus_read_analogue_input ();

	if (mcus_simulation_iterate (&error) == FALSE) {
		/* Get out of simulation UI mode */
		mcus->simulation_state = SIMULATION_STOPPED;
		mcus_simulation_update_ui ();
		mcus_update_ui ();

		if (error != NULL) {
			g_error (error->message); /* TODO */
			g_error_free (error);
		}

		return FALSE;
	}

	mcus_simulation_update_ui ();

	return TRUE;
}

void
mw_run_activate_cb (GtkAction *self, gpointer user_data)
{
	/* Initialise */
	if (mcus->simulation_state == SIMULATION_STOPPED)
		mcus_simulation_init ();
	mcus_simulation_update_ui ();

	/* Enter simulation UI mode */
	mcus->simulation_state = SIMULATION_RUNNING;
	mcus_update_ui ();

	/* Add the timeout for the simulation iterations */
	g_timeout_add (mcus->clock_speed, (GSourceFunc)simulation_iterate_cb, NULL);
}

void
mw_pause_activate_cb (GtkAction *self, gpointer user_data)
{
	mcus->simulation_state = SIMULATION_PAUSED;
	mcus_update_ui ();
}

void
mw_stop_activate_cb (GtkAction *self, gpointer user_data)
{
	mcus->simulation_state = SIMULATION_STOPPED;
	mcus_update_ui ();
}

void
mw_about_activate_cb (GtkAction *self, gpointer user_data)
{
	/*MCUSParser *parser;
	GError *error = NULL;

	mcus_print_debug_data ();

	parser = mcus_parser_new ();
	mcus_parser_parse (parser, "\n\
MOVI S1, 05 ; foobar comment\n\
MOVI S0, 00	; asd9;asd09ua9dj\n\
loop:;asdoijasi\n\
INC S0\n\
OUT Q, S0\n\
MOV S0, S2\n\
EOR S2, S1\n\
JNZ loop;asd809u", &error);
	if (error != NULL)
		g_error (error->message);

	mcus_parser_compile (parser, &error);
	if (error != NULL)
		g_error (error->message);

	mcus_print_debug_data ();

	while (mcus->zero_flag == FALSE && mcus_iterate_simulation (NULL) == TRUE) {
		mcus_print_debug_data ();
	}*/
}
