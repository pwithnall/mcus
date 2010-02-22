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

#include <config.h>
#include <stdlib.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#ifdef G_OS_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "main.h"
#include "interface.h"

static GtkFileFilter *filter = NULL;

static GtkFileFilter *
get_file_filter (void)
{
	if (filter == NULL) {
		/* Set up the filter */
		filter = gtk_file_filter_new ();
		gtk_file_filter_add_pattern (filter, "*.asm");
		g_object_ref_sink (filter);
	}

	return filter;
}

/* Returns TRUE if changes were saved, or FALSE if the operation was cancelled */
gboolean
mcus_save_changes (void)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new (GTK_WINDOW (mcus->main_window), GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
					 _("Save changes to the program before closing?"));
	gtk_dialog_add_buttons (GTK_DIALOG (dialog),
				_("Close without Saving"), GTK_RESPONSE_CLOSE,
				"gtk-cancel", GTK_RESPONSE_CANCEL,
				(mcus->current_filename == NULL) ? "gtk-save-as" : "gtk-save", GTK_RESPONSE_OK,
				NULL);
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), _("If you don't save, your changes will be permanently lost."));

	switch (gtk_dialog_run (GTK_DIALOG (dialog))) {
	case GTK_RESPONSE_CLOSE:
		gtk_widget_destroy (dialog);
		return TRUE;
	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (dialog);
		return FALSE;
	default:
		gtk_widget_destroy (dialog);
		mcus_save_program ();
		return TRUE;
	}
}

void
mcus_new_program (void)
{
	GtkTextBuffer *text_buffer = GTK_TEXT_BUFFER (gtk_builder_get_object (mcus->builder, "mw_code_buffer"));

	/* Ask to save old files */
	if (gtk_text_buffer_get_modified (text_buffer) == FALSE ||
	    mcus_save_changes () == TRUE) {
		gtk_text_buffer_set_text (text_buffer, "", -1);
		gtk_text_buffer_set_modified (text_buffer, FALSE);
	}
}

void
mcus_open_program (void)
{
	GtkWidget *dialog;
	GtkTextBuffer *text_buffer = GTK_TEXT_BUFFER (gtk_builder_get_object (mcus->builder, "mw_code_buffer"));

	/* Ask to save old files */
	if (gtk_text_buffer_get_modified (text_buffer) == FALSE ||
	    mcus_save_changes () == TRUE) {
		/* Get a filename to open */
		dialog = gtk_file_chooser_dialog_new (_("Open File"), GTK_WINDOW (mcus->main_window), GTK_FILE_CHOOSER_ACTION_OPEN,
						      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
						      NULL);
		gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog), get_file_filter ());

		if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
		{
			gchar *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
			mcus_open_file (filename);
		}
		gtk_widget_destroy (dialog);
	}
}

void
mcus_save_program (void)
{
	GtkTextIter start_iter, end_iter;
	GtkTextBuffer *text_buffer;
	GIOChannel *channel;
	GtkWidget *dialog;
	gchar *file_contents = NULL;
	GError *error = NULL;

	if (mcus->current_filename == NULL)
		return mcus_save_program_as ();

	channel = g_io_channel_new_file (mcus->current_filename, "w", &error);
	if (error != NULL)
		goto file_error;

	text_buffer = GTK_TEXT_BUFFER (gtk_builder_get_object (mcus->builder, "mw_code_buffer"));

	gtk_text_buffer_get_bounds (text_buffer, &start_iter, &end_iter);
	file_contents = gtk_text_buffer_get_text (text_buffer, &start_iter, &end_iter, FALSE);

	g_io_channel_write_chars (channel, file_contents, -1, NULL, &error);
	if (error != NULL)
		goto file_error;

	gtk_text_buffer_set_modified (text_buffer, FALSE);
	g_free (file_contents);

	g_io_channel_shutdown (channel, TRUE, NULL);
	g_io_channel_unref (channel);

	return;

file_error:
	dialog = gtk_message_dialog_new (GTK_WINDOW (mcus->main_window),
					 GTK_DIALOG_MODAL,
					 GTK_MESSAGE_ERROR,
					 GTK_BUTTONS_OK,
					 _("Program could not be saved to \"%s\""), mcus->current_filename);
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", error->message);
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);

	g_error_free (error);
	g_io_channel_unref (channel);
	if (file_contents != NULL)
		g_free (file_contents);
}

void
mcus_save_program_as (void)
{
	GtkWidget *dialog;

	dialog = gtk_file_chooser_dialog_new (_("Save File"), GTK_WINDOW (mcus->main_window), GTK_FILE_CHOOSER_ACTION_SAVE,
					      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					      GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					      NULL);
	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
	gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog), get_file_filter ());

	if (mcus->current_filename != NULL)
		gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog), mcus->current_filename);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		g_free (mcus->current_filename);
		mcus->current_filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

		/* Ensure it ends with ".asm" */
		if (g_str_has_suffix (mcus->current_filename, ".asm") == FALSE) {
			gchar *temp_string = g_strconcat (mcus->current_filename, ".asm", NULL);
			g_free (mcus->current_filename);
			mcus->current_filename = temp_string;
		}

		mcus_save_program ();
	}
	gtk_widget_destroy (dialog);
}

/* Takes ownership of filename */
void
mcus_open_file (gchar *filename)
{
	GIOChannel *channel;
	GtkWidget *dialog;
	gchar *file_contents = NULL;
	GError *error = NULL;
	GtkTextBuffer *text_buffer = GTK_TEXT_BUFFER (gtk_builder_get_object (mcus->builder, "mw_code_buffer"));

	channel = g_io_channel_new_file (filename, "r", &error);
	if (error != NULL)
		goto file_error;

	g_io_channel_read_to_end (channel, &file_contents, NULL, &error);
	if (error != NULL)
		goto file_error;

	gtk_text_buffer_set_text (text_buffer, file_contents, -1);
	gtk_text_buffer_set_modified (text_buffer, FALSE);
	g_free (file_contents);

	g_io_channel_shutdown (channel, FALSE, NULL);
	g_io_channel_unref (channel);

	g_free (mcus->current_filename);
	mcus->current_filename = filename;

	return;

file_error:
	dialog = gtk_message_dialog_new (GTK_WINDOW (mcus->main_window),
					 GTK_DIALOG_MODAL,
					 GTK_MESSAGE_ERROR,
					 GTK_BUTTONS_OK,
					 _("Program could not be opened from \"%s\""), filename);
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", error->message);
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);

	g_error_free (error);
	g_io_channel_unref (channel);
	g_free (file_contents);
}

void
mcus_quit (void)
{
	GtkTextBuffer *text_buffer = GTK_TEXT_BUFFER (gtk_builder_get_object (mcus->builder, "mw_code_buffer"));

	/* Ask to save old files */
	if (gtk_text_buffer_get_modified (text_buffer) == TRUE &&
	    mcus_save_changes () == FALSE) {
		return;
	}

	if (filter != NULL)
		g_object_unref (filter);

	/*g_object_unref (mcus->current_instruction_tag);
	g_object_unref (mcus->error_tag);*/
	if (mcus->language_manager != NULL)
		g_object_unref (mcus->language_manager);

	if (mcus->builder != NULL)
		g_object_unref (mcus->builder);
	if (mcus->main_window != NULL)
		gtk_widget_destroy (mcus->main_window);

	g_free (mcus->offset_map);
	g_free (mcus->current_filename);

	g_free (mcus);

	if (gtk_main_level () > 0)
		gtk_main_quit ();

	exit (0);
}

#ifdef G_OS_WIN32
static void
get_installation_directories (const gchar **top_directory, const gchar **bin_directory)
{
	static gchar *exe_folder_utf8 = NULL;
	static gchar *top_folder_utf8 = NULL;

	if (exe_folder_utf8 == NULL) {
		/* Get the path of the dir above that containing mcus.exe */
		wchar_t exe_filename[MAX_PATH];
		wchar_t *p;

		GetModuleFileNameW (NULL, exe_filename, G_N_ELEMENTS (exe_filename));

		p = wcsrchr (exe_filename, L'\\');
		g_assert (p != NULL);

		*p = L'\0';
		exe_folder_utf8 = g_utf16_to_utf8 (exe_filename, -1, NULL, NULL, NULL);

		p = wcsrchr (exe_filename, L'\\');
		g_assert (p != NULL);

		*p = L'\0';
		top_folder_utf8 = g_utf16_to_utf8 (exe_filename, -1, NULL, NULL, NULL);
	}

	if (top_directory != NULL)
		*top_directory = top_folder_utf8;
	if (bin_directory != NULL)
		*bin_directory = exe_folder_utf8;
}

static void
set_paths (void)
{
	gchar *path;
	const gchar *bin_directory;

	get_installation_directories (NULL, &bin_directory);
	path = g_build_path (";",
			     bin_directory,
			     g_getenv ("PATH"),
			     NULL);

	if (g_setenv ("PATH", path, TRUE) == FALSE)
		g_warning ("Could not set PATH for MCUS.");
	else if (mcus->debug)
		g_debug ("Setting PATH as \"%s\".", path);

	g_free (path);
}

static void
set_icon_paths (void)
{
	GtkIconTheme *icon_theme;
	gchar *path;
	const gchar *top_directory;

	icon_theme = gtk_icon_theme_get_default ();
	get_installation_directories (&top_directory, NULL);
	path = g_build_filename (top_directory,
				 "icons",
				 NULL);

	if (mcus->debug)
		g_debug ("Appending \"%s\" to icon theme search path.", path);

	gtk_icon_theme_append_search_path (icon_theme, path);

	if (mcus->debug) {
		gchar **paths;
		gint path_count;
		guint i;

		gtk_icon_theme_get_search_path (icon_theme, &paths, &path_count);

		g_debug ("Current icon theme search path:");
		for (i = 0; i < path_count; i++)
			g_debug ("\t%s", paths[i]);

		g_strfreev (paths);
	}

	g_free (path);
}
#endif

const gchar *
mcus_get_data_directory (void)
{
	static const gchar *path = NULL;

	if (path == NULL) {
#ifdef G_OS_WIN32
		const gchar *top_directory;

		get_installation_directories (&top_directory, NULL);
		path = g_build_filename (top_directory,
					 "share",
					 "mcus",
					 NULL);
#else /* G_OS_WIN32 */
		/* Support loading while in the source tree, iff we've been called with --debug */
		if (mcus->debug == TRUE && g_file_test ("data/", G_FILE_TEST_IS_DIR) == TRUE)
			path = "data/";
		else
			path = PACKAGE_DATA_DIR"/mcus/";
#endif /* !G_OS_WIN32 */

		if (mcus->debug)
			g_debug ("Setting data directory as \"%s\".", path);
	}

	return path;
}

int
main (int argc, char *argv[])
{
	GOptionContext *context;
	GError *error = NULL;
	gboolean debug = FALSE;
	gchar **filenames = NULL;

	const GOptionEntry options[] = {
		{ "debug", 0, 0, G_OPTION_ARG_NONE, &debug, N_("Enable debug mode"), NULL },
		{ G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &filenames, NULL, N_("[FILE]") },
		{ NULL }
	};

#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	gtk_set_locale ();
	g_thread_init (NULL);
	gtk_init (&argc, &argv);
	g_set_application_name (_("Microcontroller Simulator"));

	/* Options */
	context = g_option_context_new (_("- Simulate the 2008 OCR A-level electronics microcontroller"));
	g_option_context_set_translation_domain (context, GETTEXT_PACKAGE);
	g_option_context_add_main_entries (context, options, GETTEXT_PACKAGE);

	if (g_option_context_parse (context, &argc, &argv, &error) == FALSE) {
		/* Show an error */
		GtkWidget *dialog = gtk_message_dialog_new (NULL,
							    GTK_DIALOG_MODAL,
							    GTK_MESSAGE_ERROR,
							    GTK_BUTTONS_OK,
							    _("Command-line options could not be parsed"));
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", error->message);
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

		g_error_free (error);
		exit (1);
	}

	g_option_context_free (context);

	/* Setup */
	mcus = g_new0 (MCUS, 1);
	mcus->debug = debug;
	mcus->clock_speed = 1000 / DEFAULT_CLOCK_SPEED; /* time between iterations in ms */

#ifdef G_OS_WIN32
	set_paths ();
	set_icon_paths ();
#endif
	gtk_window_set_default_icon_name ("mcus");

	/* Create and show the interface */
	mcus_create_interface ();
	gtk_widget_show_all (mcus->main_window);

	/* See if a file to load has been specified on the command line */
	if (filenames != NULL && filenames[0] != NULL)
		mcus_open_file (filenames[0]);

	gtk_main ();
	return 0;
}
