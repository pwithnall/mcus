/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * MCUS
 * Copyright (C) Philip Withnall 2008–2010 <philip@tecnocode.co.uk>
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
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <gtksourceview/gtksourceview.h>
#include <gtksourceview/gtksourceprintcompositor.h>
#include <gtksourceview/gtksourcelanguagemanager.h>
#include <pango/pango.h>
#include <math.h>

#ifdef G_OS_WIN32
#include <gio/gio.h>
#endif

#include "compiler.h"
#include "main.h"
#include "simulation.h"
#include "interface.h"
#include "config.h"
#include "input-port.h"
#include "main-window.h"
#include "widgets/seven-segment-display.h"
#include "widgets/led.h"

G_MODULE_EXPORT void mw_input_entry_changed (GtkEntry *self, gpointer user_data);
G_MODULE_EXPORT void mw_input_check_button_toggled (GtkToggleButton *self, gpointer user_data);
G_MODULE_EXPORT void mw_output_single_ssd_option_changed_cb (GtkToggleButton *self, gpointer user_data);
G_MODULE_EXPORT void mw_output_notebook_switch_page_cb (GtkNotebook *self, GtkNotebookPage *page, guint page_num, gpointer user_data);
G_MODULE_EXPORT void mw_adc_constant_option_toggled_cb (GtkToggleButton *self, gpointer user_data);
G_MODULE_EXPORT void notify_can_undo_cb (GObject *object, GParamSpec *param_spec, gpointer user_data);
G_MODULE_EXPORT void notify_can_redo_cb (GObject *object, GParamSpec *param_spec, gpointer user_data);
G_MODULE_EXPORT void notify_has_selection_cb (GObject *object, GParamSpec *param_spec, gpointer user_data);
static void notify_program_counter_cb (GObject *object, GParamSpec *param_spec, gpointer user_data);
static void notify_zero_flag_cb (GObject *object, GParamSpec *param_spec, gpointer user_data);
static void notify_output_port_cb (GObject *object, GParamSpec *param_spec, gpointer user_data);
G_MODULE_EXPORT void buffer_modified_changed_cb (GtkTextBuffer *self, gpointer user_data);
G_MODULE_EXPORT gboolean mw_delete_event_cb (GtkWidget *widget, GdkEvent *event, gpointer user_data);
G_MODULE_EXPORT gboolean mw_key_press_event_cb (GtkWidget *widget, GdkEventKey *event, gpointer user_data);
G_MODULE_EXPORT void mw_quit_activate_cb (GtkAction *self, gpointer user_data);
G_MODULE_EXPORT void mw_undo_activate_cb (GtkAction *self, gpointer user_data);
G_MODULE_EXPORT void mw_redo_activate_cb (GtkAction *self, gpointer user_data);
G_MODULE_EXPORT void mw_cut_activate_cb (GtkAction *self, gpointer user_data);
G_MODULE_EXPORT void mw_copy_activate_cb (GtkAction *self, gpointer user_data);
G_MODULE_EXPORT void mw_paste_activate_cb (GtkAction *self, gpointer user_data);
G_MODULE_EXPORT void mw_delete_activate_cb (GtkAction *self, gpointer user_data);
G_MODULE_EXPORT void mw_run_activate_cb (GtkAction *self, gpointer user_data);
G_MODULE_EXPORT void mw_pause_activate_cb (GtkAction *self, gpointer user_data);
G_MODULE_EXPORT void mw_stop_activate_cb (GtkAction *self, gpointer user_data);
G_MODULE_EXPORT void mw_step_forward_activate_cb (GtkAction *self, gpointer user_data);
G_MODULE_EXPORT void mw_clock_speed_spin_button_value_changed_cb (GtkSpinButton *self, gpointer user_data);
G_MODULE_EXPORT void mw_contents_activate_cb (GtkAction *self, gpointer user_data);
G_MODULE_EXPORT void mw_about_activate_cb (GtkAction *self, gpointer user_data);
G_MODULE_EXPORT void mw_new_activate_cb (GtkAction *self, gpointer user_data);
G_MODULE_EXPORT void mw_open_activate_cb (GtkAction *self, gpointer user_data);
G_MODULE_EXPORT void mw_save_activate_cb (GtkAction *self, gpointer user_data);
G_MODULE_EXPORT void mw_save_as_activate_cb (GtkAction *self, gpointer user_data);
G_MODULE_EXPORT void mw_print_activate_cb (GtkAction *self, gpointer user_data);

static void update_simulation_ui (void);
static void read_analogue_input (void);

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

static void
notify_program_counter_cb (GObject *object, GParamSpec *param_spec, gpointer user_data)
{
	/* 3 characters for two hexadecimal characters and one \0 */
	gchar byte_text[3];
	guchar program_counter = mcus_simulation_get_program_counter (MCUS_SIMULATION (object));

	/* Update the program counter label */
	g_sprintf (byte_text, "%02X", program_counter);
	gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (mcus->builder, "mw_program_counter_label")), byte_text);
}

static void
notify_zero_flag_cb (GObject *object, GParamSpec *param_spec, gpointer user_data)
{
	
}

static void
notify_output_port_cb (GObject *object, GParamSpec *param_spec, gpointer user_data)
{
	/* 3 characters for two hexadecimal characters and one \0 */
	gchar byte_text[3];
	guchar output_port = mcus_simulation_get_output_port (MCUS_SIMULATION (object));

	/* Update the output port label */
	g_sprintf (byte_text, "%02X", output_port);
	gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (mcus->builder, "mw_output_port_label")), byte_text);
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
	const gchar * const *old_language_dirs;
	const gchar **language_dirs;
	guint i = 0;

	/* Grab various widgets */
	mcus->analogue_input_frequency_adjustment = GTK_ADJUSTMENT (gtk_builder_get_object (mcus->builder, "mw_adc_frequency_adjustment"));
	mcus->analogue_input_amplitude_adjustment = GTK_ADJUSTMENT (gtk_builder_get_object (mcus->builder, "mw_adc_amplitude_adjustment"));
	mcus->analogue_input_offset_adjustment = GTK_ADJUSTMENT (gtk_builder_get_object (mcus->builder, "mw_adc_offset_adjustment"));
	mcus->analogue_input_phase_adjustment = GTK_ADJUSTMENT (gtk_builder_get_object (mcus->builder, "mw_adc_phase_adjustment"));

	mcus->simulation_state = SIMULATION_STOPPED;
	mcus_update_ui ();

	/* Create the highlighting tags */
	text_buffer = GTK_TEXT_BUFFER (gtk_builder_get_object (mcus->builder, "mw_code_buffer"));
	mcus->current_instruction_tag = gtk_text_buffer_create_tag (text_buffer, "current-instruction",
								    "weight", PANGO_WEIGHT_BOLD,
								    NULL);
	mcus->error_tag = gtk_text_buffer_create_tag (text_buffer, "error",
						      "background", "pink",
						      NULL);

	/* Connect so we can update the undo/redo/cut/copy/delete actions */
	g_signal_connect (text_buffer, "notify::can-undo", G_CALLBACK (notify_can_undo_cb), NULL);
	g_signal_connect (text_buffer, "notify::can-redo", G_CALLBACK (notify_can_redo_cb), NULL);
	g_signal_connect (text_buffer, "notify::has-selection", G_CALLBACK (notify_has_selection_cb), NULL);

	/* Watch for changes in the simulation */
	g_signal_connect (mcus->simulation, "notify::program-counter", G_CALLBACK (notify_program_counter_cb), NULL);
	g_signal_connect (mcus->simulation, "notify::zero-flag", G_CALLBACK (notify_zero_flag_cb), NULL);
	g_signal_connect (mcus->simulation, "notify::output-port", G_CALLBACK (notify_output_port_cb), NULL);

	/* Watch for modification of the code buffer */
	g_signal_connect (text_buffer, "modified-changed", G_CALLBACK (buffer_modified_changed_cb), NULL);
	gtk_text_buffer_set_modified (text_buffer, FALSE);

	/* Set up the syntax highlighting */
	mcus->language_manager = gtk_source_language_manager_new ();

	/* Sort out the search paths so that our own are in there */
	old_language_dirs = gtk_source_language_manager_get_search_path (mcus->language_manager);

	while (old_language_dirs[i] != NULL)
		i++;

	language_dirs = g_malloc0 ((i + 3) * sizeof (gchar*));
	language_dirs[0] = mcus_get_data_directory ();
	language_dirs[1] = "data/";

	i = 0;
	while (old_language_dirs[i] != NULL) {
		language_dirs[i + 2] = old_language_dirs[i];
		i++;
	}
	language_dirs[i + 2] = NULL;

	if (mcus->debug) {
		g_debug ("Current language manager search path:");
		for (i = 0; language_dirs[i] != NULL; i++)
			g_debug ("\t%s", language_dirs[i]);
	}

	gtk_source_language_manager_set_search_path (mcus->language_manager, (gchar**)language_dirs);
	g_free (language_dirs);

	if (mcus->debug) {
		const gchar * const *language_ids = gtk_source_language_manager_get_language_ids (mcus->language_manager);

		g_debug ("Languages installed:");
		for (i = 0; language_ids[i] != NULL; i++)
			g_debug ("\t%s", language_ids[i]);
	}

	language = gtk_source_language_manager_get_language (mcus->language_manager, "ocr-assembly");
	if (language == NULL)
		g_warning ("Could not load syntax highlighting file.");

	gtk_source_buffer_set_language (GTK_SOURCE_BUFFER (text_buffer), language);
	if (gtk_source_buffer_get_style_scheme (GTK_SOURCE_BUFFER (text_buffer)) == NULL)
		g_debug ("NULL style scheme");
}

G_MODULE_EXPORT gboolean
mw_delete_event_cb (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	mcus_quit ();
	return TRUE;
}

G_MODULE_EXPORT gboolean
mw_key_press_event_cb (GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	guint shift;

	/* The Ctrl key should be pressed */
	if (!(event->state & GDK_CONTROL_MASK))
		return FALSE;

	switch (event->keyval) {
	case GDK_1:
		shift = 0;
		break;
	case GDK_2:
		shift = 1;
		break;
	case GDK_3:
		shift = 2;
		break;
	case GDK_4:
		shift = 3;
		break;
	case GDK_5:
		shift = 4;
		break;
	case GDK_6:
		shift = 5;
		break;
	case GDK_7:
		shift = 6;
		break;
	case GDK_8:
		shift = 7;
		break;
	default:
		/* We don't want to handle this key */
		return FALSE;
	}

	/* Toggle the relevant bit in the input port */
	mcus_simulation_set_input_port (mcus->simulation, mcus_simulation_get_input_port (mcus->simulation) ^ (1 << shift));

	/* Update the UI (updating the check buttons updates the entry too) */
	mcus_input_port_update_check_buttons ();

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

	read_analogue_input ();

	if (mcus_simulation_iterate (mcus->simulation, &error) == FALSE) {
		/* Get out of simulation UI mode */
		mcus->simulation_state = SIMULATION_STOPPED;
		mcus_simulation_finish (mcus->simulation);
		mcus_update_ui ();

		if (error != NULL) {
			GtkWidget *dialog;
			guchar program_counter = mcus_simulation_get_program_counter (mcus->simulation);

			/* Highlight the offending line */
			mcus_tag_range (mcus->error_tag,
					mcus->offset_map[program_counter].offset,
					mcus->offset_map[program_counter].offset + mcus->offset_map[program_counter].length,
					FALSE);

			/* Display an error message */
			dialog = gtk_message_dialog_new (GTK_WINDOW (mcus->main_window),
							 GTK_DIALOG_MODAL,
							 GTK_MESSAGE_ERROR,
							 GTK_BUTTONS_OK,
							 _("Error iterating simulation"));
			gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", error->message);
			gtk_dialog_run (GTK_DIALOG (dialog));
			gtk_widget_destroy (dialog);
			g_error_free (error);
		} else {
			/* If we're finished, remove the current instruction tag */
			mcus_remove_tag (mcus->current_instruction_tag);
		}

		return FALSE;
	}

	update_simulation_ui ();

	return TRUE;
}

G_MODULE_EXPORT void
mw_run_activate_cb (GtkAction *self, gpointer user_data)
{
	MCUSCompiler *compiler;
	GtkTextBuffer *code_buffer;
	GtkTextIter start_iter, end_iter;
	gchar *code;
	guint error_start, error_end;
	GtkWidget *dialog;
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
	mcus_compiler_compile (compiler, mcus->simulation, &error);

	if (error != NULL)
		goto compiler_error;
	g_object_unref (compiler);

	/* Initialise the simulator */
	if (mcus->simulation_state == SIMULATION_STOPPED)
		mcus_simulation_start (mcus->simulation);
	update_simulation_ui ();

	/* Enter simulation UI mode */
	mcus->simulation_state = SIMULATION_RUNNING;
	mcus_update_ui ();

	/* Add the timeout for the simulation iterations */
	g_timeout_add (mcus->clock_speed, (GSourceFunc) simulation_iterate_cb, NULL);

	return;

compiler_error:
	/* Highlight the offending line */
	mcus_compiler_get_error_location (compiler, &error_start, &error_end);
	mcus_tag_range (mcus->error_tag, error_start, error_end, FALSE);

	/* Display an error message */
	dialog = gtk_message_dialog_new (GTK_WINDOW (mcus->main_window),
					 GTK_DIALOG_MODAL,
					 GTK_MESSAGE_ERROR,
					 GTK_BUTTONS_OK,
					 _("Error compiling program"));
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", error->message);
	gtk_dialog_run (GTK_DIALOG (dialog));

	gtk_widget_destroy (dialog);
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
	mcus_simulation_finish (mcus->simulation);
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
mw_contents_activate_cb (GtkAction *self, gpointer user_data)
{
	GError *error = NULL;
#ifdef G_OS_WIN32
	GFile *file;
	GAppInfo *app_info;
	GList list;
	gchar *path;

	path = g_build_filename (mcus_get_data_directory (), "help.pdf", NULL);
	file = g_file_new_for_path (path);
	g_free (path);

	list.data = file;
	list.next = list.prev = NULL;

	app_info = g_app_info_get_default_for_type (".pdf", FALSE);

	if (app_info == NULL || g_app_info_launch (app_info, &list, NULL, &error) == FALSE) {
#else /* !G_OS_WIN32 */
	if (gtk_show_uri (gtk_widget_get_screen (mcus->main_window), "ghelp:mcus", gtk_get_current_event_time (), &error) == FALSE) {
#endif /* !G_OS_WIN32 */
		GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW (mcus->main_window),
							    GTK_DIALOG_MODAL,
							    GTK_MESSAGE_ERROR,
							    GTK_BUTTONS_OK,
							    _("Error displaying help"));
		if (error == NULL)
			gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), _("Couldn't find a program to open .pdf files."));
		else
			gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", error->message);

		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

		if (error != NULL)
			g_error_free (error);
	}

#ifdef G_OS_WIN32
	g_object_unref (file);
	if (app_info != NULL)
		g_object_unref (app_info);
#endif
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

static void
update_single_ssd_output (void)
{
	MCUSSevenSegmentDisplay *ssd = MCUS_SEVEN_SEGMENT_DISPLAY (gtk_builder_get_object (mcus->builder, "mw_output_single_ssd"));

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (mcus->builder, "mw_output_single_ssd_segment_option"))) == TRUE) {
		/* Each bit in the output corresponds to one segment */
		mcus_seven_segment_display_set_segment_mask (ssd, mcus_simulation_get_output_port (mcus->simulation));
	} else {
		/* The output is BCD-encoded, and we should display that number */
		guint digit = mcus_simulation_get_output_port (mcus->simulation) & 0x0F;

		if (digit > 9)
			digit = 0;

		mcus_seven_segment_display_set_digit (ssd, digit);
	}
}

G_MODULE_EXPORT void
mw_output_single_ssd_option_changed_cb (GtkToggleButton *self, gpointer user_data)
{
	update_single_ssd_output ();
}

static void
update_multi_ssd_output (void)
{
	guint i;
	gchar object_id[21];
	guchar output_port = mcus_simulation_get_output_port (mcus->simulation);

	for (i = 0; i < 16; i++) {
		/* Work out which SSD we're setting */
		g_sprintf (object_id, "mw_output_multi_ssd%u", i);

		if (i == output_port >> 4) {
			guint digit;

			/* Get the new value */
			digit = output_port & 0x0F;
			if (digit > 9)
				digit = 0;

			mcus_seven_segment_display_set_digit (MCUS_SEVEN_SEGMENT_DISPLAY (gtk_builder_get_object (mcus->builder, object_id)), digit);
		} else {
			/* Blank the display */
			mcus_seven_segment_display_set_segment_mask (MCUS_SEVEN_SEGMENT_DISPLAY (gtk_builder_get_object (mcus->builder, object_id)), 0);
		}
	}
}

static void
update_outputs (void)
{
	guint i;
	guchar output_port = mcus_simulation_get_output_port (mcus->simulation);

	/* Only update outputs if they're visible */
	switch (mcus->output_device) {
	case OUTPUT_LED_DEVICE:
		/* Update the LED outputs */
		for (i = 0; i < 8; i++) {
			gchar object_id[16];

			g_sprintf (object_id, "mw_output_led_%u", i);
			mcus_led_set_enabled (MCUS_LED (gtk_builder_get_object (mcus->builder, object_id)),
			                      output_port & (1 << i));
		}
		break;
	case OUTPUT_SINGLE_SSD_DEVICE:
		/* Update the single SSD output */
		update_single_ssd_output ();
		break;
	case OUTPUT_DUAL_SSD_DEVICE:
		/* Update the dual-SSD output */
		i = output_port >> 4;
		if (i > 9)
			i = 0;
		mcus_seven_segment_display_set_digit (MCUS_SEVEN_SEGMENT_DISPLAY (gtk_builder_get_object (mcus->builder, "mw_output_dual_ssd1")), i);

		i = output_port & 0x0F;
		if (i > 9)
			i = 0;
		mcus_seven_segment_display_set_digit (MCUS_SEVEN_SEGMENT_DISPLAY (gtk_builder_get_object (mcus->builder, "mw_output_dual_ssd0")), i);
		break;
	case OUTPUT_MULTIPLEXED_SSD_DEVICE:
		/* Update the multi-SSD output */
		update_multi_ssd_output ();
		break;
	default:
		g_assert_not_reached ();
	}
}

static void
update_simulation_ui (void)
{
	guint i;
	/* 3 characters for each memory byte (two hexadecimal digits plus either a space, newline or \0)
	 * plus 7 characters for <b></b> around the byte pointed to by the program counter. */
	gchar memory_markup[3 * MEMORY_SIZE + 7];
	/* 3 characters for each register as above */
	gchar register_text[3 * REGISTER_COUNT];
	/* 3 characters for each stack byte as above */
	gchar stack_text[3 * STACK_PREVIEW_SIZE];
	/* 3 characters for one byte as above */
	gchar byte_text[3];
	gchar *f = memory_markup;
	MCUSStackFrame *stack_frame, *stack;
	guchar *memory, *registers, program_counter;

	memory = mcus_simulation_get_memory (mcus->simulation);
	registers = mcus_simulation_get_registers (mcus->simulation);
	stack = mcus_simulation_get_stack_head (mcus->simulation);
	program_counter = mcus_simulation_get_program_counter (mcus->simulation);

	/* Update the memory label */
	for (i = 0; i < MEMORY_SIZE; i++) {
		g_sprintf (f, G_UNLIKELY (i == program_counter) ? "<b>%02X</b> " : "%02X ", memory[i]);
		f += 3;

		if (G_UNLIKELY (i == program_counter))
			f += 7;
		if (G_UNLIKELY (i % 16 == 15))
			*(f - 1) = '\n';
	}
	*(f - 1) = '\0';

	gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (mcus->builder, "mw_memory_label")), memory_markup);

	/* Update the register label */
	f = register_text;
	for (i = 0; i < REGISTER_COUNT; i++) {
		g_sprintf (f, "%02X ", registers[i]);
		f += 3;
	}
	*(f - 1) = '\0';

	gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (mcus->builder, "mw_registers_label")), register_text);

	/* Update the stack label */
	f = stack_text;
	i = 0;
	stack_frame = stack;
	while (stack_frame != NULL && i < STACK_PREVIEW_SIZE) {
		g_sprintf (f, "%02X ", stack_frame->program_counter);
		f += 3;

		if (G_UNLIKELY (i % 16 == 15))
			*(f - 1) = '\n';

		i++;
		stack_frame = stack_frame->prev;
	}
	if (i == 0)
		g_sprintf (f, "(Empty)");
	*(f - 1) = '\0';

	gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (mcus->builder, "mw_stack_label")), stack_text);

	/* Update the stack pointer label */
	g_sprintf (byte_text, "%02X", (stack == NULL) ? 0 : stack->program_counter);
	gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (mcus->builder, "mw_stack_pointer_label")), byte_text);

	update_outputs ();

	/* Move the current line mark */
	if (mcus->offset_map != NULL) {
		mcus_tag_range (mcus->current_instruction_tag,
				mcus->offset_map[program_counter].offset,
				mcus->offset_map[program_counter].offset + mcus->offset_map[program_counter].length,
				TRUE);
	}

	mcus_print_debug_data ();
}

G_MODULE_EXPORT void
mw_output_notebook_switch_page_cb (GtkNotebook *self, GtkNotebookPage *page, guint page_num, gpointer user_data)
{
	mcus->output_device = page_num;
	update_outputs ();
}

static void
read_analogue_input (void)
{
	gdouble amplitude, frequency, phase, offset, analogue_input;
	guint iteration = mcus_simulation_get_iteration (mcus->simulation);

	/* Update the analogue input from the function generator */
	amplitude = gtk_adjustment_get_value (mcus->analogue_input_amplitude_adjustment);
	frequency = gtk_adjustment_get_value (mcus->analogue_input_frequency_adjustment);
	phase = gtk_adjustment_get_value (mcus->analogue_input_phase_adjustment);
	offset = gtk_adjustment_get_value (mcus->analogue_input_offset_adjustment);

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (mcus->builder, "mw_adc_constant_option"))) == TRUE) {
		/* Constant signal */
		analogue_input = offset;
	} else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (mcus->builder, "mw_adc_sine_wave_option"))) == TRUE) {
		/* Sine wave */
		analogue_input = amplitude * sin (2.0 * M_PI * frequency * ((gdouble)(iteration) * mcus->clock_speed / 1000.0) + phase) + offset;
	} else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (mcus->builder, "mw_adc_square_wave_option"))) == TRUE) {
		/* Square wave */
		gdouble sine = sin (2.0 * M_PI * frequency * ((gdouble)(iteration) * mcus->clock_speed / 1000.0) + phase);
		analogue_input = (sine > 0) ? 1.0 : (sine == 0) ? 0.0 : -1.0;
		analogue_input = amplitude * analogue_input + offset;
	} else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (mcus->builder, "mw_adc_triangle_wave_option"))) == TRUE) {
		/* Triangle wave */
		analogue_input = amplitude * asin (sin (2.0 * M_PI * frequency * ((gdouble)(iteration) * mcus->clock_speed / 1000.0) + phase)) + offset;
	} else {
		/* Sawtooth wave */
		gdouble t = ((gdouble)(iteration) * mcus->clock_speed / 1000.0) * frequency;
		analogue_input = amplitude * 2.0 * (t - floor (t + 0.5)) + offset;
	}

	/* Clamp the value to 0--5V and set it */
	mcus_simulation_set_analogue_input (mcus->simulation, CLAMP (analogue_input, 0.0, 5.0));

	if (mcus->debug == TRUE)
		g_debug ("Analogue input: %f", analogue_input);
}

G_MODULE_EXPORT void
mw_adc_constant_option_toggled_cb (GtkToggleButton *self, gpointer user_data)
{
	gboolean constant_signal = gtk_toggle_button_get_active (self);

	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (mcus->builder, "mw_adc_frequency_spin_button")), !constant_signal);
	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (mcus->builder, "mw_adc_amplitude_spin_button")), !constant_signal);
	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (mcus->builder, "mw_adc_phase_spin_button")), !constant_signal);
}
