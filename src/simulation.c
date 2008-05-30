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
#include <glib/gprintf.h>
#include <gtk/gtk.h>

#include "main.h"
#include "instructions.h"
#include "simulation.h"
#include "interface.h"

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
	mcus->stack_pointer = 0;
	mcus->zero_flag = 0;

	for (i = 0; i < REGISTER_COUNT; i++)
		mcus->registers[i] = 0;
	for (i = 0; i < STACK_SIZE; i++)
		mcus->stack[i] = 0;

	/* Remove previous errors */
	mcus_remove_tag (mcus->error_tag);
}

/* Returns FALSE on error or if the simulation's ended */
gboolean
mcus_simulation_iterate (GError **error)
{
	guchar opcode, operand1, operand2;

	mcus->iteration++;

	/* Can't check it with >= as it does a check against guchar, which
	 * is always true due to the datatype's range. */
	if (mcus->program_counter + 1 > MEMORY_SIZE) {
		g_set_error (error, MCUS_SIMULATION_ERROR, MCUS_SIMULATION_ERROR_MEMORY_OVERFLOW,
			     _("The program counter overflowed available memory in simulation iteration %u."),
			     mcus->iteration);
		return FALSE;
	}

	opcode = mcus->memory[mcus->program_counter];
	operand1 = (mcus->program_counter + 1 < MEMORY_SIZE) ? mcus->memory[mcus->program_counter+1] : 0;
	operand2 = (mcus->program_counter + 2 < MEMORY_SIZE) ? mcus->memory[mcus->program_counter+2] : 0;

	switch (opcode) {
	case OPCODE_END:
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
		return TRUE;
	case OPCODE_JZ:
		if (mcus->zero_flag == TRUE) {
			mcus->program_counter = operand1;
			return TRUE;
		}
		break;
	case OPCODE_JNZ:
		if (mcus->zero_flag == FALSE) {
			mcus->program_counter = operand1;
			return TRUE;
		}
		break;
	case OPCODE_RCALL:
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
	case OPCODE_RET:
		/* Check for underflows */
		if (mcus->stack_pointer == 0) {
			g_set_error (error, MCUS_SIMULATION_ERROR, MCUS_SIMULATION_ERROR_STACK_UNDERFLOW,
				     _("The stack pointer underflowed available stack space in simulation iteration %u."),
				     mcus->iteration);
			return FALSE;
		}

		mcus->program_counter = mcus->memory[--mcus->stack_pointer];
		return TRUE;
	case OPCODE_SHL:
		mcus->registers[operand1] <<= 1;
		mcus->zero_flag = (mcus->registers[operand1] == 0) ? TRUE : FALSE;
		break;
	case OPCODE_SHR:
		mcus->registers[operand1] >>= 1;
		mcus->zero_flag = (mcus->registers[operand1] == 0) ? TRUE : FALSE;
		break;
	case SUBROUTINE_READTABLE:
		/* TODO: Is this correct? */
		mcus->registers[0] = mcus->memory[mcus->registers[7]];
		break;
	case SUBROUTINE_WAIT1MS:
		g_usleep (1000);
		break;
	case SUBROUTINE_READADC:
		mcus->registers[0] = 255.0 * mcus->analogue_input / ANALOGUE_INPUT_MAX_VOLTAGE;
		break;
	default:
		/* We've encountered some data? */
		g_set_error (error, MCUS_SIMULATION_ERROR, MCUS_SIMULATION_ERROR_INVALID_OPCODE,
			     _("An invalid opcode \"%02X\" was encountered at address %02X in simulation iteration %u."),
			     (guint)opcode,
			     (guint)mcus->program_counter,
			     mcus->iteration);
		return FALSE;
	}

	/* Don't forget to increment the PC */
	mcus->program_counter += mcus_instruction_data[opcode].size;

	return TRUE;
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
	gchar stack_text[3 * STACK_SIZE];
	/* 3 characters for one byte as above */
	gchar byte_text[3];
	gchar *f = memory_markup;

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
	for (i = 0; i < STACK_SIZE; i++) {
		g_sprintf (f, "%02X ", mcus->stack[i]);
		f += 3;

		if (G_UNLIKELY (i % 16 == 15))
			*(f - 1) = '\n';
	}
	*(f - 1) = '\0';

	gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (mcus->builder, "mw_stack_label")), stack_text);

	/* Update the output port label */
	g_sprintf (byte_text, "%02X", mcus->output_port);
	gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (mcus->builder, "mw_output_port_label")), byte_text);

	/* Update the program counter label */
	g_sprintf (byte_text, "%02X", mcus->program_counter);
	gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (mcus->builder, "mw_program_counter_label")), byte_text);

	/* Update the stack pointer label */
	g_sprintf (byte_text, "%02X", mcus->stack_pointer);
	gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (mcus->builder, "mw_stack_pointer_label")), byte_text);

	/* Move the current line mark */
	if (mcus->offset_map != NULL) {
		mcus_tag_range (mcus->current_instruction_tag,
				mcus->offset_map[mcus->program_counter].offset,
				mcus->offset_map[mcus->program_counter].offset + mcus->offset_map[mcus->program_counter].length,
				TRUE);
	}

	mcus_print_debug_data ();
}
