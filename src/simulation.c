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
#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <string.h>

#include "main.h"
#include "instructions.h"
#include "simulation.h"
#include "interface.h"
#include "analogue-input.h"
#include "widgets/led.h"
#include "widgets/seven-segment-display.h"

G_MODULE_EXPORT void mw_output_single_ssd_option_changed_cb (GtkToggleButton *self, gpointer user_data);
G_MODULE_EXPORT void mw_output_notebook_switch_page_cb (GtkNotebook *self, GtkNotebookPage *page, guint page_num, gpointer user_data);

GQuark
mcus_simulation_error_quark (void)
{
	static GQuark q = 0;

	if (q == 0)
		q = g_quark_from_static_string ("mcus-simulation-error-quark");

	return q;
}

void
mcus_simulation_init (void)
{
	guint i;

	mcus->iteration = 0;
	mcus->program_counter = PROGRAM_START_ADDRESS;
	mcus->zero_flag = 0;

	for (i = 0; i < REGISTER_COUNT; i++)
		mcus->registers[i] = 0;
	mcus->stack = NULL;

	/* Remove previous errors */
	mcus_remove_tag (mcus->error_tag);
}

void
mcus_simulation_finalise (void)
{
	MCUSStackFrame *stack_frame;

	/* Free up any remaining frames on the stack */
	stack_frame = mcus->stack;
	while (stack_frame != NULL) {
		MCUSStackFrame *prev_frame;
		prev_frame = stack_frame->prev;
		g_free (stack_frame);
		stack_frame = prev_frame;
	}
}

/* Returns FALSE on error or if the simulation's ended */
gboolean
mcus_simulation_iterate (GError **error)
{
	guchar opcode, operand1, operand2;
	MCUSStackFrame *stack_frame;

	mcus_read_analogue_input ();

	mcus->iteration++;

	/* Can't check it with >= as it does a check against guchar, which
	 * is always true due to the datatype's range. */
	if (mcus->program_counter + 1 > MEMORY_SIZE) {
		g_set_error (error, MCUS_SIMULATION_ERROR, MCUS_SIMULATION_ERROR_MEMORY_OVERFLOW,
			     _("The program counter overflowed available memory in simulation iteration %u."),
			     mcus->iteration);
		mcus_simulation_finalise ();
		return FALSE;
	}

	opcode = mcus->memory[mcus->program_counter];
	operand1 = (mcus->program_counter + 1 < MEMORY_SIZE) ? mcus->memory[mcus->program_counter + 1] : 0;
	operand2 = (mcus->program_counter + 2 < MEMORY_SIZE) ? mcus->memory[mcus->program_counter + 2] : 0;

	switch (opcode) {
	case OPCODE_HALT:
		mcus_simulation_finalise ();
		return FALSE;
		break;
	case OPCODE_MOVI:
		mcus->registers[operand1] = operand2;
		break;
	case OPCODE_MOV:
		mcus->registers[operand1] = mcus->registers[operand2];
		break;
	case OPCODE_ADD:
		mcus->registers[operand1] += mcus->registers[operand2];
		mcus->zero_flag = (mcus->registers[operand1] == 0) ? TRUE : FALSE;
		break;
	case OPCODE_SUB:
		mcus->registers[operand1] -= mcus->registers[operand2];
		mcus->zero_flag = (mcus->registers[operand1] == 0) ? TRUE : FALSE;
		break;
	case OPCODE_AND:
		mcus->registers[operand1] &= mcus->registers[operand2];
		mcus->zero_flag = (mcus->registers[operand1] == 0) ? TRUE : FALSE;
		break;
	case OPCODE_EOR:
		mcus->registers[operand1] ^= mcus->registers[operand2];
		mcus->zero_flag = (mcus->registers[operand1] == 0) ? TRUE : FALSE;
		break;
	case OPCODE_INC:
		mcus->registers[operand1] += 1;
		mcus->zero_flag = (mcus->registers[operand1] == 0) ? TRUE : FALSE;
		break;
	case OPCODE_DEC:
		mcus->registers[operand1] -= 1;
		mcus->zero_flag = (mcus->registers[operand1] == 0) ? TRUE : FALSE;
		break;
	case OPCODE_IN:
		mcus->registers[operand1] = mcus->input_port; /* only one operand is stored */
		break;
	case OPCODE_OUT:
		mcus->output_port = mcus->registers[operand1]; /* only one operand is stored */
		break;
	case OPCODE_JP:
		mcus->program_counter = operand1;
		goto update_and_exit;
	case OPCODE_JZ:
		if (mcus->zero_flag == TRUE) {
			mcus->program_counter = operand1;
			goto update_and_exit;
		}
		break;
	case OPCODE_JNZ:
		if (mcus->zero_flag == FALSE) {
			mcus->program_counter = operand1;
			goto update_and_exit;
		}
		break;
	case OPCODE_RCALL:
		/* Check for calling the built-in subroutines */
		if (operand1 == mcus->program_counter) {
			/* readtable */
			/* TODO: Is this correct? */
			mcus->registers[0] = mcus->memory[mcus->registers[7]];
			break;
		} else if (operand1 == mcus->program_counter + 1) {
			/* wait1ms */
			g_usleep (1000);
			break;
		} else if (operand1 == mcus->program_counter + 2) {
			/* readadc */
			mcus->registers[0] = 255.0 * mcus->analogue_input / ANALOGUE_INPUT_MAX_VOLTAGE;
			break;
		}

		/* If we're just calling a normal subroutine, push the
		 * current state as a new frame onto the stack */
		stack_frame = g_new (MCUSStackFrame, 1);
		stack_frame->prev = mcus->stack;
		stack_frame->program_counter = mcus->program_counter + mcus_instruction_data[opcode].size;
		memcpy (stack_frame->registers, mcus->registers, sizeof (guchar) * REGISTER_COUNT);

		mcus->stack = stack_frame;

		/* Jump to the subroutine */
		mcus->program_counter = operand1;
		goto update_and_exit;
	case OPCODE_RET:
		/* Check for underflows */
		if (mcus->stack == NULL) {
			g_set_error (error, MCUS_SIMULATION_ERROR, MCUS_SIMULATION_ERROR_STACK_UNDERFLOW,
				     _("The stack pointer underflowed available stack space in simulation iteration %u."),
				     mcus->iteration);
			mcus_simulation_finalise ();
			return FALSE;
		}

		/* Pop the old state off the stack */
		stack_frame = mcus->stack;
		mcus->stack = stack_frame->prev;
		mcus->program_counter = stack_frame->program_counter;
		memcpy (mcus->registers, stack_frame->registers, sizeof (guchar) * REGISTER_COUNT);
		g_free (stack_frame);

		goto update_and_exit;
	case OPCODE_SHL:
		mcus->registers[operand1] <<= 1;
		mcus->zero_flag = (mcus->registers[operand1] == 0) ? TRUE : FALSE;
		break;
	case OPCODE_SHR:
		mcus->registers[operand1] >>= 1;
		mcus->zero_flag = (mcus->registers[operand1] == 0) ? TRUE : FALSE;
		break;
	default:
		/* We've encountered some data? */
		g_set_error (error, MCUS_SIMULATION_ERROR, MCUS_SIMULATION_ERROR_INVALID_OPCODE,
			     _("An invalid opcode \"%02X\" was encountered at address %02X in simulation iteration %u."),
			     (guint)opcode,
			     (guint)mcus->program_counter,
			     mcus->iteration);
		mcus_simulation_finalise ();
		return FALSE;
	}

	/* Don't forget to increment the PC */
	mcus->program_counter += mcus_instruction_data[opcode].size;

update_and_exit:
	mcus_simulation_update_ui ();
	return TRUE;
}

static void
update_single_ssd_output (void)
{
	MCUSSevenSegmentDisplay *ssd = MCUS_SEVEN_SEGMENT_DISPLAY (gtk_builder_get_object (mcus->builder, "mw_output_single_ssd"));

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (mcus->builder, "mw_output_single_ssd_segment_option"))) == TRUE) {
		/* Each bit in the output corresponds to one segment */
		mcus_seven_segment_display_set_segment_mask (ssd, mcus->output_port);
	} else {
		/* The output is BCD-encoded, and we should display that number */
		guint digit = mcus->output_port & 0x0F;

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

	for (i = 0; i < 16; i++) {
		/* Work out which SSD we're setting */
		g_sprintf (object_id, "mw_output_multi_ssd%u", i);

		if (i == mcus->output_port >> 4) {
			guint digit;

			/* Get the new value */
			digit = mcus->output_port & 0x0F;
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

	/* Only update outputs if they're visible */
	switch (mcus->output_device) {
	case OUTPUT_LED_DEVICE:
		/* Update the LED outputs */
		for (i = 0; i < 8; i++) {
			gchar object_id[16];

			g_sprintf (object_id, "mw_output_led_%u", i);
			mcus_led_set_enabled (MCUS_LED (gtk_builder_get_object (mcus->builder, object_id)),
					      mcus->output_port & (1 << i));
		}
		break;
	case OUTPUT_SINGLE_SSD_DEVICE:
		/* Update the single SSD output */
		update_single_ssd_output ();
		break;
	case OUTPUT_DUAL_SSD_DEVICE:
		/* Update the dual-SSD output */
		i = mcus->output_port >> 4;
		if (i > 9)
			i = 0;
		mcus_seven_segment_display_set_digit (MCUS_SEVEN_SEGMENT_DISPLAY (gtk_builder_get_object (mcus->builder, "mw_output_dual_ssd1")), i);

		i = mcus->output_port & 0x0F;
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

void
mcus_simulation_update_ui (void)
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
	MCUSStackFrame *stack_frame;

	/* Update the memory label */
	for (i = 0; i < MEMORY_SIZE; i++) {
		g_sprintf (f, G_UNLIKELY (i == mcus->program_counter) ? "<b>%02X</b> " : "%02X ", mcus->memory[i]);
		f += 3;

		if (G_UNLIKELY (i == mcus->program_counter))
			f += 7;
		if (G_UNLIKELY (i % 16 == 15))
			*(f - 1) = '\n';
	}
	*(f - 1) = '\0';

	gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (mcus->builder, "mw_memory_label")), memory_markup);

	/* Update the register label */
	f = register_text;
	for (i = 0; i < REGISTER_COUNT; i++) {
		g_sprintf (f, "%02X ", mcus->registers[i]);
		f += 3;
	}
	*(f - 1) = '\0';

	gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (mcus->builder, "mw_registers_label")), register_text);

	/* Update the stack label */
	f = stack_text;
	i = 0;
	stack_frame = mcus->stack;
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

	/* Update the output port label */
	g_sprintf (byte_text, "%02X", mcus->output_port);
	gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (mcus->builder, "mw_output_port_label")), byte_text);

	/* Update the program counter label */
	g_sprintf (byte_text, "%02X", mcus->program_counter);
	gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (mcus->builder, "mw_program_counter_label")), byte_text);

	/* Update the stack pointer label */
	g_sprintf (byte_text, "%02X", (mcus->stack == NULL) ? 0 : mcus->stack->program_counter);
	gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (mcus->builder, "mw_stack_pointer_label")), byte_text);

	update_outputs ();

	/* Move the current line mark */
	if (mcus->offset_map != NULL) {
		mcus_tag_range (mcus->current_instruction_tag,
				mcus->offset_map[mcus->program_counter].offset,
				mcus->offset_map[mcus->program_counter].offset + mcus->offset_map[mcus->program_counter].length,
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
