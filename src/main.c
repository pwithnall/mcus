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
#include "main-window.h"

void
mcus_quit (MCUSMainWindow *main_window)
{
	/* Try and save the file first */
	if (mcus_main_window_quit (main_window) == FALSE)
		return;

	if (main_window != NULL)
		gtk_widget_destroy (GTK_WIDGET (main_window));

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
	else
		g_debug ("Setting PATH as \"%s\".", path);

	g_free (path);
}

static void
set_icon_paths (void)
{
	GtkIconTheme *icon_theme;
	gchar *path, **paths;
	gint path_count;
	guint i;
	const gchar *top_directory;

	icon_theme = gtk_icon_theme_get_default ();
	get_installation_directories (&top_directory, NULL);
	path = g_build_filename (top_directory,
				 "icons",
				 NULL);

	g_debug ("Appending \"%s\" to icon theme search path.", path);
	gtk_icon_theme_append_search_path (icon_theme, path);

	g_free (path);

	/* Debug */
	gtk_icon_theme_get_search_path (icon_theme, &paths, &path_count);

	g_debug ("Current icon theme search path:");
	for (i = 0; i < path_count; i++)
		g_debug ("\t%s", paths[i]);

	g_strfreev (paths);
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
		path = PACKAGE_DATA_DIR"/mcus/";
#endif /* !G_OS_WIN32 */

		g_debug ("Setting data directory as \"%s\".", path);
	}

	return path;
}

/* Debug log message handler: discards debug messages unless MCUS is run with the --debug flag. */
static void
debug_handler (const char *log_domain, GLogLevelFlags log_level, const char *message, gpointer user_data)
{
	gboolean debug = GPOINTER_TO_UINT (user_data);

	if (debug == TRUE)
		g_log_default_handler (log_domain, log_level, message, NULL);
}

int
main (int argc, char *argv[])
{
	GOptionContext *context;
	GError *error = NULL;
	gboolean debug = FALSE;
	gchar **filenames = NULL;
	GtkWindow *main_window;

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

	/* Debug log handling */
	g_log_set_handler (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, (GLogFunc) debug_handler, GUINT_TO_POINTER (debug));

	/* Set up */
#ifdef G_OS_WIN32
	set_paths ();
	set_icon_paths ();
#endif
	gtk_window_set_default_icon_name ("mcus");

	/* Create and show the interface */
	main_window = mcus_main_window_new ();
	if (main_window == NULL)
		mcus_quit (MCUS_MAIN_WINDOW (main_window));
	gtk_widget_show_all (GTK_WIDGET (main_window));

	/* See if a file to load has been specified on the command line */
	if (filenames != NULL && filenames[0] != NULL)
		mcus_main_window_open_file (MCUS_MAIN_WINDOW (main_window), filenames[0]);

	gtk_main ();

	return 0;
}
