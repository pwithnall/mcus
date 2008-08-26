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
#include <gtk/gtk.h>
#include <gtksourceview/gtksourceview.h>
#include <gtksourceview/gtksourceprintcompositor.h>
#include <gtksourceview/gtksourcelanguagemanager.h>
#include <pango/pango.h>

#include "compiler.h"
#include "main.h"
#include "simulation.h"
#include "interface.h"
#include "config.h"
#include "input-port.h"

G_MODULE_EXPORT void mw_input_entry_changed (GtkEntry *self, gpointer user_data);
G_MODULE_EXPORT void mw_input_check_button_toggled (GtkToggleButton *self, gpointer user_data);

G_MODULE_EXPORT void
notify_can_undo_cb (GObject *object, GParamSpec *param_spec, gpointer user_data)
{
	g_object_set (gtk_builder_get_object (mcus->builder, "mcus_undo_action"),
		      "sensitive", gtk_source_buffer_can_undo (GTK_SOURCE_BUFFER (object)),
		      NULL);
}

G_MODULE_EXPORT void
notify_can_redo_cb (GObject *object, GParamSpec *param_spec, gpointer user_data)
{
	g_object_set (gtk_builder_get_object (mcus->builder, "mcus_redo_action"),
		      "sensitive", gtk_source_buffer_can_redo (GTK_SOURCE_BUFFER (object)),
		      NULL);
}

G_MODULE_EXPORT void
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

G_MODULE_EXPORT void
buffer_modified_changed_cb (GtkTextBuffer *self, gpointer user_data)
{
	g_object_set (gtk_builder_get_object (mcus->builder, "mcus_save_action"),
		      "sensitive", gtk_text_buffer_get_modified (self),
		      NULL);
	g_object_set (gtk_builder_get_object (mcus->builder, "mcus_save_as_action"),
		      "sensitive", TRUE,
		      NULL);
}

void
mcus_main_window_init (void)
{
	GtkTextBuffer *text_buffer;
	GtkSourceLanguage *language;
	const gchar * const language_dirs[] = {
		PACKAGE_DATA_DIR"/mcus/",
		"./data/",
		NULL
	};

	/* Grab various widgets */
	mcus->analogue_input_adjustment = GTK_ADJUSTMENT (gtk_builder_get_object (mcus->builder, "mw_analogue_input_adjustment"));
	mcus->analogue_input_frequency_adjustment = GTK_ADJUSTMENT (gtk_builder_get_object (mcus->builder, "mw_adc_frequency_adjustment"));
	mcus->analogue_input_amplitude_adjustment = GTK_ADJUSTMENT (gtk_builder_get_object (mcus->builder, "mw_adc_amplitude_adjustment"));
	mcus->analogue_input_offset_adjustment = GTK_ADJUSTMENT (gtk_builder_get_object (mcus->builder, "mw_adc_offset_adjustment"));
	mcus->analogue_input_phase_adjustment = GTK_ADJUSTMENT (gtk_builder_get_object (mcus->builder, "mw_adc_phase_adjustment"));

	mcus->simulation_state = SIMULATION_STOPPED;
	mcus_update_ui ();

	/* Create the highlighting tags */
	text_buffer = GTK_TEXT_BUFFER (gtk_builder_get_object (mcus->builder, "mw_code_buffer"));
	mcus->current_instruction_tag = gtk_text_buffer_create_tag (text_buffer,
								    "current-instruction",
								    "weight", PANGO_WEIGHT_BOLD,
								    NULL);
	mcus->error_tag = gtk_text_buffer_create_tag (text_buffer,
						      "error",
						      "background", "pink",
						      NULL);

	/* Connect so we can update the undo/redo/cut/copy/delete actions */
	g_signal_connect (text_buffer, "notify::can-undo", G_CALLBACK (notify_can_undo_cb), NULL);
	g_signal_connect (text_buffer, "notify::can-redo", G_CALLBACK (notify_can_redo_cb), NULL);
	g_signal_connect (text_buffer, "notify::has-selection", G_CALLBACK (notify_has_selection_cb), NULL);

	/* Watch for modification of the code buffer */
	g_signal_connect (text_buffer, "modified-changed", G_CALLBACK (buffer_modified_changed_cb), NULL);
	gtk_text_buffer_set_modified (text_buffer, FALSE);

	/* Set up the syntax highlighting */
	mcus->language_manager = gtk_source_language_manager_new ();
	gtk_source_language_manager_set_search_path (mcus->language_manager, (gchar**)language_dirs);
	language = gtk_source_language_manager_get_language (mcus->language_manager, "ocr-assembly");

	gtk_source_buffer_set_language (GTK_SOURCE_BUFFER (text_buffer), language);
}

G_MODULE_EXPORT gboolean
mw_delete_event_cb (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	mcus_quit ();
	return TRUE;
}

G_MODULE_EXPORT void
mw_quit_activate_cb (GtkAction *self, gpointer user_data)
{
	mcus_quit ();
}

G_MODULE_EXPORT void
mw_undo_activate_cb (GtkAction *self, gpointer user_data)
{
	gtk_source_buffer_undo (GTK_SOURCE_BUFFER (gtk_builder_get_object (mcus->builder, "mw_code_buffer")));
}

G_MODULE_EXPORT void
mw_redo_activate_cb (GtkAction *self, gpointer user_data)
{
	gtk_source_buffer_redo (GTK_SOURCE_BUFFER (gtk_builder_get_object (mcus->builder, "mw_code_buffer")));
}

G_MODULE_EXPORT void
mw_cut_activate_cb (GtkAction *self, gpointer user_data)
{
	GtkClipboard *clipboard = gtk_clipboard_get_for_display (gtk_widget_get_display (GTK_WIDGET (mcus->main_window)), GDK_SELECTION_CLIPBOARD);
	gtk_text_buffer_cut_clipboard (GTK_TEXT_BUFFER (gtk_builder_get_object (mcus->builder, "mw_code_buffer")), clipboard, TRUE);
}

G_MODULE_EXPORT void
mw_copy_activate_cb (GtkAction *self, gpointer user_data)
{
	GtkClipboard *clipboard = gtk_clipboard_get_for_display (gtk_widget_get_display (GTK_WIDGET (mcus->main_window)), GDK_SELECTION_CLIPBOARD);
	gtk_text_buffer_copy_clipboard (GTK_TEXT_BUFFER (gtk_builder_get_object (mcus->builder, "mw_code_buffer")), clipboard);
}

G_MODULE_EXPORT void
mw_paste_activate_cb (GtkAction *self, gpointer user_data)
{
	GtkClipboard *clipboard = gtk_clipboard_get_for_display (gtk_widget_get_display (GTK_WIDGET (mcus->main_window)), GDK_SELECTION_CLIPBOARD);
	gtk_text_buffer_paste_clipboard (GTK_TEXT_BUFFER (gtk_builder_get_object (mcus->builder, "mw_code_buffer")), clipboard, NULL, TRUE);
}

G_MODULE_EXPORT void
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

	if (mcus_simulation_iterate (&error) == FALSE) {
		/* Get out of simulation UI mode */
		mcus->simulation_state = SIMULATION_STOPPED;
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

	return TRUE;
}

G_MODULE_EXPORT void
mw_run_activate_cb (GtkAction *self, gpointer user_data)
{
	MCUSCompiler *compiler;
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
	compiler = mcus_compiler_new ();
	mcus_compiler_parse (compiler, code, &error);
	g_free (code);

	if (error != NULL)
		goto compiler_error;
	mcus_print_debug_data ();

	/* Compile it */
	mcus_compiler_compile (compiler, &error);

	if (error != NULL)
		goto compiler_error;
	g_object_unref (compiler);

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

compiler_error:
	/* Highlight the offending line */
	mcus_tag_range (mcus->error_tag,
			mcus_compiler_get_offset (compiler),
			mcus_compiler_get_offset (compiler) + COMPILER_ERROR_CONTEXT_LENGTH,
			FALSE);

	/* Display an error message */
	mcus_interface_error (error->message);
	g_error_free (error);
	g_object_unref (compiler);
}

G_MODULE_EXPORT void
mw_pause_activate_cb (GtkAction *self, gpointer user_data)
{
	mcus->simulation_state = SIMULATION_PAUSED;
	mcus_update_ui ();
}

G_MODULE_EXPORT void
mw_stop_activate_cb (GtkAction *self, gpointer user_data)
{
	mcus->simulation_state = SIMULATION_STOPPED;
	mcus_update_ui ();
}

G_MODULE_EXPORT void
mw_step_forward_activate_cb (GtkAction *self, gpointer user_data)
{
	mcus->simulation_state = SIMULATION_RUNNING;
	if (simulation_iterate_cb (NULL) == TRUE)
		mcus->simulation_state = SIMULATION_PAUSED;
}

G_MODULE_EXPORT void
mw_clock_speed_spin_button_value_changed_cb (GtkSpinButton *self, gpointer user_data)
{
	mcus->clock_speed = 1000 / gtk_spin_button_get_value (self);
}

G_MODULE_EXPORT void
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

G_MODULE_EXPORT void
mw_new_activate_cb (GtkAction *self, gpointer user_data)
{
	mcus_new_program ();
}

G_MODULE_EXPORT void
mw_open_activate_cb (GtkAction *self, gpointer user_data)
{
	mcus_open_program ();
}

G_MODULE_EXPORT void
mw_save_activate_cb (GtkAction *self, gpointer user_data)
{
	mcus_save_program ();
}

G_MODULE_EXPORT void
mw_save_as_activate_cb (GtkAction *self, gpointer user_data)
{
	mcus_save_program_as ();
}

static gboolean
paginate_cb (GtkPrintOperation *operation, GtkPrintContext *context, GtkSourcePrintCompositor *source_compositor)
{
	if (gtk_source_print_compositor_paginate (source_compositor, context)) {
		gint n_pages;
        
		n_pages = gtk_source_print_compositor_get_n_pages (source_compositor);
		gtk_print_operation_set_n_pages (operation, n_pages);

		return TRUE;
	}

	return FALSE;
}

static void
draw_page_cb (GtkPrintOperation *operation, GtkPrintContext *context, gint page_number, GtkSourcePrintCompositor *source_compositor)
{
	gtk_source_print_compositor_draw_page (source_compositor, context, page_number);
}

G_MODULE_EXPORT void
mw_print_activate_cb (GtkAction *self, gpointer user_data)
{
	GtkPrintOperation *operation;
	GtkPrintOperationResult res;
	GtkSourcePrintCompositor *source_compositor;
	static GtkPrintSettings *settings;

	/* TODO: Header/Footer? */
	operation = gtk_print_operation_new ();
	source_compositor = gtk_source_print_compositor_new (GTK_SOURCE_BUFFER (gtk_builder_get_object (mcus->builder, "mw_code_buffer")));
	gtk_source_print_compositor_set_print_line_numbers (source_compositor, 1);
	gtk_source_print_compositor_set_highlight_syntax (source_compositor, TRUE);

	if (settings != NULL) 
		gtk_print_operation_set_print_settings (operation, settings);

	g_signal_connect (operation, "paginate", G_CALLBACK (paginate_cb), source_compositor);
	g_signal_connect (operation, "draw-page", G_CALLBACK (draw_page_cb), source_compositor);

	res = gtk_print_operation_run (operation, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
				       GTK_WINDOW (mcus->main_window), NULL);

	if (res == GTK_PRINT_OPERATION_RESULT_APPLY) {
		if (settings != NULL)
			g_object_unref (settings);
		settings = g_object_ref (gtk_print_operation_get_print_settings (operation));
	}

	g_object_unref (source_compositor);
	g_object_unref (operation);
}

G_MODULE_EXPORT void
mw_input_check_button_toggled (GtkToggleButton *self, gpointer user_data)
{
	/* Signal fluff prevents race condition between this signal handler and the one below */
	GObject *input_port_entry = gtk_builder_get_object (mcus->builder, "mw_input_port_entry");
	g_signal_handlers_block_by_func (input_port_entry, mw_input_entry_changed, NULL);

	mcus_input_port_read_check_buttons ();
	mcus_input_port_update_entry ();

	g_signal_handlers_unblock_by_func (input_port_entry, mw_input_entry_changed, NULL);
}

static void
set_input_check_button_signal_state (gboolean blocked)
{
	guint i;

	for (i = 0; i < 8; i++) {
		GObject *button;
		gchar button_id[24];

		/* Grab the control */
		g_sprintf (button_id, "mw_input_check_button_%u", i);
		button = gtk_builder_get_object (mcus->builder, button_id);

		/* Either block or unblock the signal as appropriate */
		if (blocked == TRUE)
			g_signal_handlers_block_by_func (button, mw_input_check_button_toggled, NULL);
		else
			g_signal_handlers_unblock_by_func (button, mw_input_check_button_toggled, NULL);
	}
}

G_MODULE_EXPORT void
mw_input_entry_changed (GtkEntry *self, gpointer user_data)
{
	set_input_check_button_signal_state (TRUE);

	/* TODO: Do something on error? */
	if (mcus_input_port_read_entry (NULL))
		mcus_input_port_update_check_buttons ();

	set_input_check_button_signal_state (FALSE);
}
