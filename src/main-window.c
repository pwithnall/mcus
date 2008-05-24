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
#include <gtk/gtk.h>
#include <gtksourceview/gtksourceview.h>
#include <pango/pango.h>

#include "parser.h"
#include "main.h"
#include "simulation.h"
#include "interface.h"
#include "config.h"

void
notify_can_undo_cb (GObject *object, GParamSpec *param_spec, gpointer user_data)
{
	g_object_set (gtk_builder_get_object (mcus->builder, "mcus_undo_action"),
		      "sensitive", gtk_source_buffer_can_undo (GTK_SOURCE_BUFFER (object)),
		      NULL);
}

void
notify_can_redo_cb (GObject *object, GParamSpec *param_spec, gpointer user_data)
{
	g_object_set (gtk_builder_get_object (mcus->builder, "mcus_redo_action"),
		      "sensitive", gtk_source_buffer_can_redo (GTK_SOURCE_BUFFER (object)),
		      NULL);
}

void
notify_has_selection_cb (GObject *object, GParamSpec *param_spec, gpointer user_data)
{
	gboolean sensitive = gtk_text_buffer_get_has_selection (GTK_TEXT_BUFFER (object));
	g_object_set (gtk_builder_get_object (mcus->builder, "mcus_cut_action"),
		      "sensitive", sensitive,
		      NULL);
	g_object_set (gtk_builder_get_object (mcus->builder, "mcus_copy_action"),
		      "sensitive", sensitive,
		      NULL);
	g_object_set (gtk_builder_get_object (mcus->builder, "mcus_delete_action"),
		      "sensitive", sensitive,
		      NULL);
}

void
mcus_main_window_init (void)
{
	GtkTextBuffer *text_buffer;

	mcus_update_ui ();

	/* Create the highlighting tags */
	text_buffer = GTK_TEXT_BUFFER (gtk_builder_get_object (mcus->builder, "mw_code_buffer"));
	mcus->current_instruction_tag = gtk_text_buffer_create_tag (text_buffer,
								    "current-instruction",
								    "weight", PANGO_WEIGHT_BOLD,
								    NULL);
	mcus->error_tag = gtk_text_buffer_create_tag (text_buffer,
						      "error",
						      "background", "red",
						      NULL);

	/* Connect so we can update the undo/redo/cut/copy/delete actions */
	g_signal_connect (text_buffer, "notify::can-undo", G_CALLBACK (notify_can_undo_cb), NULL);
	g_signal_connect (text_buffer, "notify::can-redo", G_CALLBACK (notify_can_redo_cb), NULL);
	g_signal_connect (text_buffer, "notify::has-selection", G_CALLBACK (notify_has_selection_cb), NULL);
}

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
mw_undo_activate_cb (GtkAction *self, gpointer user_data)
{
	gtk_source_buffer_undo (GTK_SOURCE_BUFFER (gtk_builder_get_object (mcus->builder, "mw_code_buffer")));
}

void
mw_redo_activate_cb (GtkAction *self, gpointer user_data)
{
	gtk_source_buffer_redo (GTK_SOURCE_BUFFER (gtk_builder_get_object (mcus->builder, "mw_code_buffer")));
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

static gboolean
simulation_iterate_cb (gpointer user_data)
{
	GError *error = NULL;

	if (mcus->simulation_state != SIMULATION_RUNNING)
		return FALSE;

	mcus_read_analogue_input ();

	if (mcus_read_input_port (&error) == FALSE ||
	    mcus_simulation_iterate (&error) == FALSE) {
		/* Get out of simulation UI mode */
		mcus->simulation_state = SIMULATION_STOPPED;
		mcus_simulation_update_ui ();
		mcus_update_ui ();

		if (error != NULL) {
			/* Highlight the offending line */
			mcus_tag_range (mcus->error_tag,
					mcus->offset_map[mcus->program_counter].offset,
					mcus->offset_map[mcus->program_counter].offset + mcus->offset_map[mcus->program_counter].length,
					FALSE);

			/* Display an error message */
			mcus_interface_error (error->message);
			g_error_free (error);
		} else {
			/* If we're finished, remove the current instruction tag */
			mcus_remove_tag (mcus->current_instruction_tag);
		}

		return FALSE;
	}

	mcus_simulation_update_ui ();

	return TRUE;
}

void
mw_run_activate_cb (GtkAction *self, gpointer user_data)
{
	MCUSParser *parser;
	GtkTextBuffer *code_buffer;
	GtkTextIter start_iter, end_iter;
	gchar *code;
	GError *error = NULL;

	/* Remove previous errors */
	mcus_remove_tag (mcus->error_tag);

	/* Get the assembly code */
	code_buffer = GTK_TEXT_BUFFER (gtk_builder_get_object (mcus->builder, "mw_code_buffer"));
	gtk_text_buffer_get_bounds (code_buffer, &start_iter, &end_iter);
	code = gtk_text_buffer_get_text (code_buffer, &start_iter, &end_iter, FALSE);

	mcus_print_debug_data ();

	/* Parse it */
	parser = mcus_parser_new ();
	mcus_parser_parse (parser, code, &error);

	if (error != NULL)
		goto parser_error;
	g_free (code);
	mcus_print_debug_data ();

	/* Compile it */
	mcus_parser_compile (parser, &error);

	if (error != NULL)
		goto parser_error;
	g_object_unref (parser);

	/* Initialise the simulator */
	if (mcus->simulation_state == SIMULATION_STOPPED)
		mcus_simulation_init ();
	mcus_simulation_update_ui ();

	/* Enter simulation UI mode */
	mcus->simulation_state = SIMULATION_RUNNING;
	mcus_update_ui ();

	/* Add the timeout for the simulation iterations */
	g_timeout_add (mcus->clock_speed, (GSourceFunc)simulation_iterate_cb, NULL);

	return;

parser_error:
	/* Highlight the offending line */
	mcus_tag_range (mcus->error_tag,
			mcus_parser_get_offset (parser),
			mcus_parser_get_offset (parser) + PARSER_ERROR_CONTEXT_LENGTH,
			FALSE);

	/* Display an error message */
	mcus_interface_error (error->message);
	g_error_free (error);
	g_object_unref (parser);
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
mw_step_forward_activate_cb (GtkAction *self, gpointer user_data)
{
	mcus->simulation_state = SIMULATION_RUNNING;
	if (simulation_iterate_cb (NULL) == TRUE)
		mcus->simulation_state = SIMULATION_PAUSED;
}

void
mw_clock_speed_spin_button_value_changed_cb (GtkSpinButton *self, gpointer user_data)
{
	mcus->clock_speed = 1000 / gtk_spin_button_get_value (self);
}

void
mw_about_activate_cb (GtkAction *self, gpointer user_data)
{
	gchar *license;

	const gchar *authors[] = {
		"Philip Withnall <philip@tecnocode.co.uk>",
		NULL
	};
	const gchar *license_parts[] = {
		N_("MCUS is free software: you can redistribute it and/or modify "
		   "it under the terms of the GNU General Public License as published by "
		   "the Free Software Foundation, either version 3 of the License, or "
		   "(at your option) any later version."),
		N_("MCUS is distributed in the hope that it will be useful, "
		   "but WITHOUT ANY WARRANTY; without even the implied warranty of "
		   "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
		   "GNU General Public License for more details."),
		N_("You should have received a copy of the GNU General Public License "
		   "along with MCUS.  If not, see <http://www.gnu.org/licenses/>."),
	};
	const gchar *description = N_("A program to simulate the microcontroller specified in the 2008 OCR A-level electronics syllabus.");

	license = g_strjoin ("\n\n",
			  _(license_parts[0]),
			  _(license_parts[1]),
			  _(license_parts[2]),
			  NULL);

	gtk_show_about_dialog (GTK_WINDOW (mcus->main_window),
				"version", VERSION,
				"copyright", _("Copyright \xc2\xa9 2008 Philip Withnall"),
				"comments", _(description),
				"authors", authors,
				/* Translators: please include your names here to be credited for your hard work!
				 * Format:
				 * "Translator name 1 <translator@email.address>\n"
				 * "Translator name 2 <translator2@email.address>"
				 */
				"translator-credits", _("translator-credits"),
				"logo-icon-name", "mcus",
				"license", license,
				"wrap-license", TRUE,
				"website-label", _("MCUS Website"),
				"website", "http://tecnocode.co.uk/projects/mcus",
				NULL);

	g_free (license);
}
