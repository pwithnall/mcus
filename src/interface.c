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

#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <string.h>
#include <gtksourceview/gtksourceview.h>
#include <pango/pango.h>

#include "config.h"
#include "main.h"
#include "interface.h"

GtkWidget *
mcus_create_interface (void)
{
	GError *error = NULL;

	mcus->builder = gtk_builder_new ();

	if (gtk_builder_add_from_file (mcus->builder, PACKAGE_DATA_DIR"/mcus/mcus.ui", &error) == FALSE &&
	    gtk_builder_add_from_file (mcus->builder, "./data/mcus.ui", NULL) == FALSE) {
		/* Show an error */
		GtkWidget *dialog = gtk_message_dialog_new (NULL,
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_OK,
				_("UI file \"%s/mcus/mcus.ui\" could not be loaded. Error: %s"), PACKAGE_DATA_DIR, error->message);
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

		g_error_free (error);
		mcus_quit ();

		return NULL;
	}

	gtk_builder_set_translation_domain (mcus->builder, GETTEXT_PACKAGE);
	gtk_builder_connect_signals (mcus->builder, NULL);

	/* Set up the main window */
	/* TODO: This is horrible */
	mcus->main_window = GTK_WIDGET (gtk_builder_get_object (mcus->builder, "mcus_main_window"));
	mcus_update_ui ();

	/* Create the current line source mark */
	mcus->current_instruction_tag = gtk_text_buffer_create_tag (GTK_TEXT_BUFFER (gtk_builder_get_object (mcus->builder, "mw_code_buffer")),
								    "current-instruction",
								    "weight", PANGO_WEIGHT_BOLD,
								    NULL);

	return mcus->main_window;
}

/**
 * mcus_interface_error:
 * @message: Error message
 * @parent_window: The error dialog's parent window
 *
 * Display an error message and print the message to
 * the console.
 **/
void
mcus_interface_error (const gchar *message, GtkWidget *parent_window)
{
	GtkWidget *dialog;

	g_warning (message);

	dialog = gtk_message_dialog_new (GTK_WINDOW (parent_window),
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_OK,
				message);
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

void
mcus_print_debug_data (void)
{
	guint i;

	if (mcus->debug == FALSE)
		return;

	/* General data */
	g_printf ("Program counter: %02X\nStack pointer: %02X\nZero flag: %u\nClock speed: %lu\n",
		 (guint)mcus->program_counter,
		 (guint)mcus->stack_pointer,
		 (mcus->zero_flag == TRUE) ? 1 : 0,
		 mcus->clock_speed);

	/* Registers */
	g_printf ("Registers:");
	for (i = 0; i < REGISTER_COUNT; i++)
		g_printf (" %02X", (guint)mcus->registers[i]);
	g_printf ("\n");

	/* Stack */
	g_printf ("Stack:\n");
	for (i = 0; i < STACK_SIZE; i++) {
		g_printf (" %02X", (guint)mcus->stack[i]);

		if (i % 16 == 15)
			g_printf ("\n");
	}
	g_printf ("\n");

	/* Ports */
	g_printf ("Input port: %02X\nOutput port: %02X\nAnalogue input: %f\n",
		 (guint)mcus->input_port,
		 (guint)mcus->output_port,
		 mcus->analogue_input);

	/* Memory */
	g_printf ("Memory:\n");
	for (i = 0; i < MEMORY_SIZE; i++) {
		if (i == mcus->program_counter)
			g_printf ("\033[1m%02X\033[0m ", (guint)mcus->memory[i]);
		else
			g_printf ("%02X ", (guint)mcus->memory[i]);

		if (i % 16 == 15)
			g_printf ("\n");
	}
	g_printf ("\n");
}

void
mcus_update_ui (void)
{
	gboolean sensitive = mcus->simulation_state == SIMULATION_STOPPED ? TRUE : FALSE;

#define SET_SENSITIVITY2(W,S) \
	g_object_set (gtk_builder_get_object (mcus->builder, (W)), "sensitive", (S), NULL);
#define SET_SENSITIVITY(W) \
	SET_SENSITIVITY2(W, sensitive)

	SET_SENSITIVITY ("mw_code_view")
	SET_SENSITIVITY2 ("mw_input_port_entry", mcus->simulation_state != SIMULATION_RUNNING)
	SET_SENSITIVITY2 ("mw_analogue_input_spin_button", mcus->simulation_state != SIMULATION_RUNNING)
	SET_SENSITIVITY2 ("mw_clock_speed_spin_button", mcus->simulation_state != SIMULATION_RUNNING)

	SET_SENSITIVITY ("mcus_print_action")
	SET_SENSITIVITY ("mcus_cut_action")
	SET_SENSITIVITY ("mcus_copy_action")
	SET_SENSITIVITY ("mcus_paste_action")
	SET_SENSITIVITY ("mcus_delete_action")
	SET_SENSITIVITY ("mcus_compile_action")

	SET_SENSITIVITY2 ("mcus_run_action", mcus->simulation_state != SIMULATION_RUNNING)
	SET_SENSITIVITY2 ("mcus_pause_action", mcus->simulation_state == SIMULATION_RUNNING)
	SET_SENSITIVITY2 ("mcus_stop_action", mcus->simulation_state != SIMULATION_STOPPED)
}

void
mcus_read_input_port (void)
{
	gint digit_value;
	const gchar *entry_text;

	entry_text = gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (mcus->builder, "mw_input_port_entry")));
	if (strlen (entry_text) != 2) {
		g_error ("TODO: bad input port length");
		return;
	}

	/* Deal with the first digit */
	digit_value = g_ascii_xdigit_value (entry_text[0]);
	if (digit_value == -1) {
		g_error ("TODO: bad input port value");
		return;
	}
	mcus->input_port = digit_value * 16;

	/* Deal with the second digit */
	digit_value = g_ascii_xdigit_value (entry_text[1]);
	if (digit_value == -1) {
		g_error ("TODO: bad input port value");
		return;
	}
	mcus->input_port += digit_value;
}

void
mcus_read_analogue_input (void)
{
	mcus->analogue_input = gtk_spin_button_get_value (GTK_SPIN_BUTTON (gtk_builder_get_object (mcus->builder, "mw_analogue_input_spin_button")));
}

