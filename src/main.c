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
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>

#include "main.h"
#include "interface.h"
#include "instructions.h"

GQuark
mcus_simulation_error_quark (void)
{
	static GQuark q = 0;

	if (q == 0)
		q = g_quark_from_static_string ("mcus-simulation-error-quark");

	return q;
}

void
mcus_initialise_simulation (gulong clock_speed)
{
	guint i;

	mcus->iteration = 0;
	mcus->program_counter = PROGRAM_START_ADDRESS;
	mcus->stack_pointer = 0;
	mcus->zero_flag = 0;
	mcus->input_port = 0;
	mcus->output_port = 0;
	mcus->analogue_input = 0.0;
	mcus->clock_speed = clock_speed;

	for (i = 0; i < REGISTER_COUNT; i++)
		mcus->registers[i] = 0;
	for (i = 0; i < STACK_SIZE; i++)
		mcus->stack[i] = 0;
}

gboolean
mcus_iterate_simulation (GError **error)
{
	guchar instruction, operand1, operand2;

	mcus->iteration++;

	/* Can't check it with >= as it does a check against guchar, which
	 * is always true due to the datatype's range. */
	if (mcus->program_counter + 1 > MEMORY_SIZE) {
		g_set_error (error, MCUS_SIMULATION_ERROR, MCUS_SIMULATION_ERROR_MEMORY_OVERFLOW,
			     _("The program counter overflowed available memory in simulation iteration %u."),
			     mcus->iteration);
		return FALSE;
	}

	instruction = mcus->memory[mcus->program_counter];
	operand1 = (mcus->program_counter + 1 < MEMORY_SIZE) ? mcus->memory[mcus->program_counter+1] : 0;
	operand2 = (mcus->program_counter + 2 < MEMORY_SIZE) ? mcus->memory[mcus->program_counter+2] : 0;

	switch (instruction) {
	case INSTRUCTION_MOVI:
		mcus->registers[operand1] = operand2;
		break;
	case INSTRUCTION_MOV:
		mcus->registers[operand1] = mcus->registers[operand2];
		break;
	case INSTRUCTION_ADD:
		mcus->registers[operand1] += mcus->registers[operand2];
		mcus->zero_flag = (mcus->registers[operand1] == 0) ? TRUE : FALSE;
		break;
	case INSTRUCTION_SUB:
		mcus->registers[operand1] -= mcus->registers[operand2];
		mcus->zero_flag = (mcus->registers[operand1] == 0) ? TRUE : FALSE;
		break;
	case INSTRUCTION_AND:
		mcus->registers[operand1] &= mcus->registers[operand2];
		mcus->zero_flag = (mcus->registers[operand1] == 0) ? TRUE : FALSE;
		break;
	case INSTRUCTION_EOR:
		mcus->registers[operand1] ^= mcus->registers[operand2];
		mcus->zero_flag = (mcus->registers[operand1] == 0) ? TRUE : FALSE;
		break;
	case INSTRUCTION_INC:
		mcus->registers[operand1] += 1;
		mcus->zero_flag = (mcus->registers[operand1] == 0) ? TRUE : FALSE;
		break;
	case INSTRUCTION_DEC:
		mcus->registers[operand1] -= 1;
		mcus->zero_flag = (mcus->registers[operand1] == 0) ? TRUE : FALSE;
		break;
	case INSTRUCTION_IN:
		mcus->registers[operand1] = mcus->input_port; /* only one operand is stored */
		break;
	case INSTRUCTION_OUT:
		mcus->output_port = mcus->registers[operand1]; /* only one operand is stored */
		break;
	case INSTRUCTION_JP:
		mcus->program_counter = operand1;
		return TRUE;
	case INSTRUCTION_JZ:
		if (mcus->zero_flag == TRUE) {
			mcus->program_counter = operand1;
			return TRUE;
		}
		break;
	case INSTRUCTION_JNZ:
		if (mcus->zero_flag == FALSE) {
			mcus->program_counter = operand1;
			return TRUE;
		}
		break;
	case INSTRUCTION_RCALL:
		/* Check for overflows */
		if (mcus->stack_pointer == STACK_SIZE) {
			g_set_error (error, MCUS_SIMULATION_ERROR, MCUS_SIMULATION_ERROR_STACK_OVERFLOW,
				     _("The stack pointer overflowed available stack space in simulation iteration %u."),
				     mcus->iteration);
			return FALSE;
		}

		mcus->stack[mcus->stack_pointer++] = mcus->program_counter;
		mcus->program_counter = operand1;
		return TRUE;
	case INSTRUCTION_RET:
		/* Check for underflows */
		if (mcus->stack_pointer == 0) {
			g_set_error (error, MCUS_SIMULATION_ERROR, MCUS_SIMULATION_ERROR_STACK_UNDERFLOW,
				     _("The stack pointer underflowed available stack space in simulation iteration %u."),
				     mcus->iteration);
			return FALSE;
		}

		mcus->program_counter = mcus->memory[--mcus->stack_pointer];
		return TRUE;
	case INSTRUCTION_SHL:
		mcus->registers[operand1] <<= 1;
		mcus->zero_flag = (mcus->registers[operand1] == 0) ? TRUE : FALSE;
		break;
	case INSTRUCTION_SHR:
		mcus->registers[operand1] >>= 1;
		mcus->zero_flag = (mcus->registers[operand1] == 0) ? TRUE : FALSE;
		break;
	default:
		/* We've encountered some data? */
		g_set_error (error, MCUS_SIMULATION_ERROR, MCUS_SIMULATION_ERROR_INVALID_INSTRUCTION,
			     _("An invalid instruction was encountered at address %02X in simulation iteration %u."),
			     (guint)mcus->program_counter,
			     mcus->iteration);
		return FALSE;
	}

	/* Don't forget to increment the PC */
	mcus->program_counter += mcus_instruction_data[instruction].size;

	return TRUE;
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
	g_printf ("Stack:");
	for (i = 0; i < STACK_SIZE; i++)
		g_printf (" %02X", (guint)mcus->stack[i]);
	g_printf ("\n");

	/* Ports */
	g_printf ("Input port: %02X\nOutput port: %02X\nADC: %f\n",
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
mcus_quit (void)
{
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

	/* Create and show the interface */
	mcus_create_interface ();
	gtk_widget_show_all (mcus->main_window);

	gtk_main ();
	return 0;
}
