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

#include <config.h>
#include <stdlib.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "main.h"
#include "interface.h"

void
mcus_quit (void)
{
	g_object_unref (mcus->builder);
	gtk_widget_destroy (mcus->main_window);
	g_free (mcus);

	if (gtk_main_level () > 0)
		gtk_main_quit ();

	exit (0);
}

int
main (int argc, char *argv[])
{
	GOptionContext *context;
	GError *error = NULL;
	gboolean debug = FALSE;

	const GOptionEntry options[] = {
		{ "debug", 0, 0, G_OPTION_ARG_NONE, &debug, N_("Enable debug mode"), NULL },
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
	gtk_window_set_default_icon_name ("mcus");

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
				_("Command-line options could not be parsed. Error: %s"), error->message);
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

		g_error_free (error);
		exit (1);
	}

	g_option_context_free (context);

	/* Setup */
	mcus = g_new (MCUS, 1);
	mcus->debug = debug;
	mcus->clock_speed = DEFAULT_CLOCK_SPEED; /* time between iterations in ms */

	/* Create and show the interface */
	mcus_create_interface ();
	gtk_widget_show_all (mcus->main_window);

	gtk_main ();
	return 0;
}
