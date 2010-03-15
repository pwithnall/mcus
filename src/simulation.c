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
#include <limits.h>

#include "instructions.h"
#include "simulation.h"

/* This is also in the UI file (in Volts) */
#define ANALOGUE_INPUT_MAX_VOLTAGE 5.0

GQuark
mcus_simulation_error_quark (void)
{
	static GQuark q = 0;

	if (q == 0)
		q = g_quark_from_static_string ("mcus-simulation-error-quark");

	return q;
}

static void mcus_simulation_finalize (GObject *object);
static void mcus_simulation_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void mcus_simulation_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

struct _MCUSSimulationPrivate {
	/* Simulated hardware */
	guchar program_counter;
	gboolean zero_flag;
	guchar registers[REGISTER_COUNT];
	guchar input_port;
	guchar output_port;
	gdouble analogue_input;
	guchar memory[MEMORY_SIZE];
	guchar lookup_table[LOOKUP_TABLE_SIZE];
	MCUSStackFrame *stack;

	/* Simulation metadata */
	guint iteration;
};

enum {
	PROP_ITERATION = 1,
	PROP_PROGRAM_COUNTER,
	PROP_ZERO_FLAG,
	PROP_INPUT_PORT,
	PROP_OUTPUT_PORT,
	PROP_ANALOGUE_INPUT
};

enum {
	SIGNAL_ITERATION_FINISHED,
	LAST_SIGNAL
};

static int signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (MCUSSimulation, mcus_simulation, G_TYPE_OBJECT)

static void
mcus_simulation_class_init (MCUSSimulationClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (MCUSSimulationPrivate));

	gobject_class->get_property = mcus_simulation_get_property;
	gobject_class->set_property = mcus_simulation_set_property;
	gobject_class->finalize = mcus_simulation_finalize;

	/**
	 * MCUSSimulation:iteration:
	 *
	 * The current (zero-based) iteration of the simulation.
	 **/
	g_object_class_install_property (gobject_class, PROP_ITERATION,
				g_param_spec_uint ("iteration",
					"Iteration", "The current (zero-based) iteration of the simulation.",
					0, G_MAXUINT, 0,
					G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * MCUSSimulation:program-counter:
	 *
	 * The address of the opcode of the currently executing instruction in the microcontroller memory.
	 **/
	g_object_class_install_property (gobject_class, PROP_PROGRAM_COUNTER,
				g_param_spec_uchar ("program-counter",
					"Program Counter", "The address of the opcode of the currently executing instruction in memory.",
					0, UCHAR_MAX, PROGRAM_START_ADDRESS,
					G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * MCUSSimulation:zero-flag:
	 *
	 * A flag indicating whether the result of the last arithmetical instruction was zero.
	 **/
	g_object_class_install_property (gobject_class, PROP_ZERO_FLAG,
				g_param_spec_boolean ("zero-flag",
					"Zero Flag", "A flag indicating whether the result of the last arithmetical instruction was zero.",
					FALSE,
					G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * MCUSSimulation:input-port:
	 *
	 * A byte-wide input port for the microcontroller.
	 **/
	g_object_class_install_property (gobject_class, PROP_INPUT_PORT,
				g_param_spec_uchar ("input-port",
					"Input Port", "A byte-wide input port for the microcontroller.",
					0, UCHAR_MAX, 0,
					G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * MCUSSimulation:output-port:
	 *
	 * A byte-wide output port from the microcontroller.
	 **/
	g_object_class_install_property (gobject_class, PROP_OUTPUT_PORT,
				g_param_spec_uchar ("output-port",
					"Output Port", "A byte-wide output port from the microcontroller.",
					0, UCHAR_MAX, 0,
					G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * MCUSSimulation:analogue-input:
	 *
	 * An analogue voltage input to an ADC in the microcontroller.
	 **/
	g_object_class_install_property (gobject_class, PROP_ANALOGUE_INPUT,
				g_param_spec_double ("analogue-input",
					"Analogue Input", "An analogue voltage input to an ADC in the microcontroller.",
					0.0, ANALOGUE_INPUT_MAX_VOLTAGE, 0.0,
					G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * MCUSSimulation::iteration-finished:
	 * @simulation: the #MCUSSimulation which has finished an iteration
	 *
	 * Emitted when an iteration of the simulation has been finished and output data is ready to be pushed to the UI.
	 **/
	signals[SIGNAL_ITERATION_FINISHED] = g_signal_new ("iteration-finished",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				0,
				NULL, NULL,
				g_cclosure_marshal_VOID__VOID,
				G_TYPE_NONE, 0);
}

static void
mcus_simulation_init (MCUSSimulation *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, MCUS_TYPE_SIMULATION, MCUSSimulationPrivate);
}

static void
mcus_simulation_finalize (GObject *object)
{
	mcus_simulation_finish (MCUS_SIMULATION (object));

	/* Chain up to the parent class */
	G_OBJECT_CLASS (mcus_simulation_parent_class)->finalize (object);
}

static void
mcus_simulation_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	MCUSSimulationPrivate *priv = MCUS_SIMULATION (object)->priv;

	switch (property_id) {
		case PROP_ITERATION:
			g_value_set_uint (value, priv->iteration);
			break;
		case PROP_PROGRAM_COUNTER:
			g_value_set_uchar (value, priv->program_counter);
			break;
		case PROP_ZERO_FLAG:
			g_value_set_boolean (value, priv->zero_flag);
			break;
		case PROP_INPUT_PORT:
			g_value_set_uchar (value, priv->input_port);
			break;
		case PROP_OUTPUT_PORT:
			g_value_set_uchar (value, priv->output_port);
			break;
		case PROP_ANALOGUE_INPUT:
			g_value_set_double (value, priv->analogue_input);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
mcus_simulation_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_INPUT_PORT:
			mcus_simulation_set_input_port (MCUS_SIMULATION (object), g_value_get_uchar (value));
			break;
		case PROP_ANALOGUE_INPUT:
			mcus_simulation_set_analogue_input (MCUS_SIMULATION (object), g_value_get_double (value));
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

void
mcus_simulation_start (MCUSSimulation *self)
{
	MCUSSimulationPrivate *priv = self->priv;

	g_return_if_fail (MCUS_IS_SIMULATION (self));

	g_object_freeze_notify (G_OBJECT (self));

	priv->iteration = 0;
	g_object_notify (G_OBJECT (self), "iteration");

	priv->program_counter = PROGRAM_START_ADDRESS;
	g_object_notify (G_OBJECT (self), "program-counter");

	priv->zero_flag = 0;
	g_object_notify (G_OBJECT (self), "zero-flag");

	memset (priv->registers, 0, sizeof (guchar) * REGISTER_COUNT);
	priv->stack = NULL;

	g_object_thaw_notify (G_OBJECT (self));
}

/* Returns FALSE on error or if the simulation's ended */
gboolean
mcus_simulation_iterate (MCUSSimulation *self, GError **error)
{
	guchar opcode, operand1, operand2;
	MCUSStackFrame *stack_frame;
	MCUSSimulationPrivate *priv = self->priv;

	g_return_val_if_fail (MCUS_IS_SIMULATION (self), FALSE);

	/* Can't check it with >= as it does a check against guchar, which
	 * is always true due to the datatype's range. */
	if (priv->program_counter + 1 > MEMORY_SIZE) {
		g_set_error (error, MCUS_SIMULATION_ERROR, MCUS_SIMULATION_ERROR_MEMORY_OVERFLOW,
		             _("The program counter overflowed available memory in simulation iteration %u."),
		             priv->iteration);
		mcus_simulation_finish (self);
		return FALSE;
	}

	opcode = priv->memory[priv->program_counter];
	operand1 = (priv->program_counter + 1 < MEMORY_SIZE) ? priv->memory[priv->program_counter + 1] : 0;
	operand2 = (priv->program_counter + 2 < MEMORY_SIZE) ? priv->memory[priv->program_counter + 2] : 0;

	g_object_freeze_notify (G_OBJECT (self));

	switch (opcode) {
	case OPCODE_HALT:
		mcus_simulation_finish (self);
		g_object_thaw_notify (G_OBJECT (self));
		return FALSE;
	case OPCODE_MOVI:
		priv->registers[operand1] = operand2;
		break;
	case OPCODE_MOV:
		priv->registers[operand1] = priv->registers[operand2];
		break;
	case OPCODE_ADD:
		priv->registers[operand1] += priv->registers[operand2];
		priv->zero_flag = (priv->registers[operand1] == 0) ? TRUE : FALSE;
		g_object_notify (G_OBJECT (self), "zero-flag");
		break;
	case OPCODE_SUB:
		priv->registers[operand1] -= priv->registers[operand2];
		priv->zero_flag = (priv->registers[operand1] == 0) ? TRUE : FALSE;
		g_object_notify (G_OBJECT (self), "zero-flag");
		break;
	case OPCODE_AND:
		priv->registers[operand1] &= priv->registers[operand2];
		priv->zero_flag = (priv->registers[operand1] == 0) ? TRUE : FALSE;
		g_object_notify (G_OBJECT (self), "zero-flag");
		break;
	case OPCODE_EOR:
		priv->registers[operand1] ^= priv->registers[operand2];
		priv->zero_flag = (priv->registers[operand1] == 0) ? TRUE : FALSE;
		g_object_notify (G_OBJECT (self), "zero-flag");
		break;
	case OPCODE_INC:
		priv->registers[operand1] += 1;
		priv->zero_flag = (priv->registers[operand1] == 0) ? TRUE : FALSE;
		g_object_notify (G_OBJECT (self), "zero-flag");
		break;
	case OPCODE_DEC:
		priv->registers[operand1] -= 1;
		priv->zero_flag = (priv->registers[operand1] == 0) ? TRUE : FALSE;
		g_object_notify (G_OBJECT (self), "zero-flag");
		break;
	case OPCODE_IN:
		priv->registers[operand1] = priv->input_port; /* only one operand is stored */
		break;
	case OPCODE_OUT:
		priv->output_port = priv->registers[operand1]; /* only one operand is stored */
		g_object_notify (G_OBJECT (self), "output-port");
		break;
	case OPCODE_JP:
		priv->program_counter = operand1;
		g_object_notify (G_OBJECT (self), "program-counter");
		goto update_and_exit;
	case OPCODE_JZ:
		if (priv->zero_flag == TRUE) {
			priv->program_counter = operand1;
			g_object_notify (G_OBJECT (self), "program-counter");
			goto update_and_exit;
		}
		break;
	case OPCODE_JNZ:
		if (priv->zero_flag == FALSE) {
			priv->program_counter = operand1;
			g_object_notify (G_OBJECT (self), "program-counter");
			goto update_and_exit;
		}
		break;
	case OPCODE_RCALL:
		/* Check for calling the built-in subroutines */
		if (operand1 == priv->program_counter) {
			/* readtable */
			priv->registers[0] = priv->lookup_table[priv->registers[7]];
			break;
		} else if (operand1 == priv->program_counter + 1) {
			/* wait1ms */
			g_usleep (1000);
			break;
		} else if (operand1 == priv->program_counter + 2) {
			/* readadc */
			priv->registers[0] = 255.0 * priv->analogue_input / ANALOGUE_INPUT_MAX_VOLTAGE;
			break;
		}

		/* If we're just calling a normal subroutine, push the
		 * current state as a new frame onto the stack */
		stack_frame = g_new (MCUSStackFrame, 1);
		stack_frame->prev = priv->stack;
		stack_frame->program_counter = priv->program_counter + mcus_instruction_data[opcode].size;
		memcpy (stack_frame->registers, priv->registers, sizeof (guchar) * REGISTER_COUNT);

		priv->stack = stack_frame;

		/* Jump to the subroutine */
		priv->program_counter = operand1;
		g_object_notify (G_OBJECT (self), "program-counter");
		goto update_and_exit;
	case OPCODE_RET:
		/* Check for underflows */
		if (priv->stack == NULL) {
			g_set_error (error, MCUS_SIMULATION_ERROR, MCUS_SIMULATION_ERROR_STACK_UNDERFLOW,
			             _("The stack pointer underflowed available stack space in simulation iteration %u."),
			             priv->iteration);
			mcus_simulation_finish (self);
			g_object_thaw_notify (G_OBJECT (self));
			return FALSE;
		}

		/* Pop the old state off the stack */
		stack_frame = priv->stack;
		priv->stack = stack_frame->prev;
		priv->program_counter = stack_frame->program_counter;
		g_object_notify (G_OBJECT (self), "program-counter");
		memcpy (priv->registers, stack_frame->registers, sizeof (guchar) * REGISTER_COUNT);
		g_free (stack_frame);

		goto update_and_exit;
	case OPCODE_SHL:
		priv->registers[operand1] <<= 1;
		priv->zero_flag = (priv->registers[operand1] == 0) ? TRUE : FALSE;
		g_object_notify (G_OBJECT (self), "zero-flag");
		break;
	case OPCODE_SHR:
		priv->registers[operand1] >>= 1;
		priv->zero_flag = (priv->registers[operand1] == 0) ? TRUE : FALSE;
		g_object_notify (G_OBJECT (self), "zero-flag");
		break;
	default:
		/* We've encountered some data? */
		g_set_error (error, MCUS_SIMULATION_ERROR, MCUS_SIMULATION_ERROR_INVALID_OPCODE,
		             _("An invalid opcode \"%02X\" was encountered at address %02X in simulation iteration %u."),
		             (guint) opcode,
		             (guint) priv->program_counter,
		             priv->iteration);
		mcus_simulation_finish (self);
		g_object_thaw_notify (G_OBJECT (self));
		return FALSE;
	}

	/* Don't forget to increment the PC */
	priv->program_counter += mcus_instruction_data[opcode].size;
	g_object_notify (G_OBJECT (self), "program-counter");

	g_object_thaw_notify (G_OBJECT (self));

update_and_exit:
	g_signal_emit (self, signals[SIGNAL_ITERATION_FINISHED], 0);
	priv->iteration++;

	return TRUE;
}

void
mcus_simulation_finish (MCUSSimulation *self)
{
	MCUSStackFrame *stack_frame;
	MCUSSimulationPrivate *priv = self->priv;

	g_return_if_fail (MCUS_IS_SIMULATION (self));

	/* Free up any remaining frames on the stack */
	stack_frame = priv->stack;
	while (stack_frame != NULL) {
		MCUSStackFrame *prev_frame;
		prev_frame = stack_frame->prev;
		g_free (stack_frame);
		stack_frame = prev_frame;
	}
}

/* TODO: Remove this */
/* The number of stack frames to show */
#define STACK_PREVIEW_SIZE 5

void
mcus_simulation_print_debug_data (MCUSSimulation *self)
{
	MCUSSimulationPrivate *priv = self->priv;
	guint i;
	MCUSStackFrame *stack_frame;

	/* TODO if (mcus->debug == FALSE)
		return;*/

	/* General data */
	g_printf ("Program counter: %02X\nZero flag: %u",/*\nClock speed: %lu\n",*/
	          (guint) priv->program_counter,
	          (priv->zero_flag == TRUE) ? 1 : 0/*,
	          TODO mcus->clock_speed*/);

	/* Registers */
	g_printf ("Registers:");
	for (i = 0; i < REGISTER_COUNT; i++)
		g_printf (" %02X", (guint) priv->registers[i]);
	g_printf ("\n");

	/* Stack */
	g_printf ("Stack:\n");
	stack_frame = priv->stack;
	i = 0;
	while (stack_frame != NULL && i < STACK_PREVIEW_SIZE) {
		g_printf (" %02X", (guint) stack_frame->program_counter);

		if (i % 16 == 15)
			g_printf ("\n");

		i++;
		stack_frame = stack_frame->prev;
	}
	if (i == 0)
		g_printf ("(Empty)");
	g_printf ("\n");

	/* Ports */
	g_printf ("Input port: %02X\nOutput port: %02X\nAnalogue input: %f\n",
	          (guint) priv->input_port,
	          (guint) priv->output_port,
	          priv->analogue_input);

	/* Memory */
	g_printf ("Memory:\n");
	for (i = 0; i < MEMORY_SIZE; i++) {
		if (i == priv->program_counter)
			g_printf ("\033[1m%02X\033[0m ", (guint) priv->memory[i]);
		else
			g_printf ("%02X ", (guint) priv->memory[i]);

		if (i % 16 == 15)
			g_printf ("\n");
	}
	g_printf ("\n");
}

guchar *
mcus_simulation_get_memory (MCUSSimulation *self)
{
	g_return_val_if_fail (MCUS_IS_SIMULATION (self), NULL);
	return self->priv->memory;
}

guchar *
mcus_simulation_get_lookup_table (MCUSSimulation *self)
{
	g_return_val_if_fail (MCUS_IS_SIMULATION (self), NULL);
	return self->priv->lookup_table;
}

guchar *
mcus_simulation_get_registers (MCUSSimulation *self)
{
	g_return_val_if_fail (MCUS_IS_SIMULATION (self), NULL);
	return self->priv->registers;
}

MCUSStackFrame *
mcus_simulation_get_stack_head (MCUSSimulation *self)
{
	g_return_val_if_fail (MCUS_IS_SIMULATION (self), NULL);
	return self->priv->stack;
}

guint
mcus_simulation_get_iteration (MCUSSimulation *self)
{
	g_return_val_if_fail (MCUS_IS_SIMULATION (self), 0);
	return self->priv->iteration;
}

guchar
mcus_simulation_get_program_counter (MCUSSimulation *self)
{
	g_return_val_if_fail (MCUS_IS_SIMULATION (self), 0);
	return self->priv->program_counter;
}

gboolean
mcus_simulation_get_zero_flag (MCUSSimulation *self)
{
	g_return_val_if_fail (MCUS_IS_SIMULATION (self), FALSE);
	return self->priv->zero_flag;
}

guchar
mcus_simulation_get_output_port (MCUSSimulation *self)
{
	g_return_val_if_fail (MCUS_IS_SIMULATION (self), 0);
	return self->priv->output_port;
}

guchar
mcus_simulation_get_input_port (MCUSSimulation *self)
{
	g_return_val_if_fail (MCUS_IS_SIMULATION (self), 0);
	return self->priv->input_port;
}

void
mcus_simulation_set_input_port (MCUSSimulation *self, guchar input_port)
{
	g_return_if_fail (MCUS_IS_SIMULATION (self));

	self->priv->input_port = input_port;
	g_object_notify (G_OBJECT (self), "input-port");
}

gdouble
mcus_simulation_get_analogue_input (MCUSSimulation *self)
{
	g_return_val_if_fail (MCUS_IS_SIMULATION (self), 0.0);
	return self->priv->analogue_input;
}

void
mcus_simulation_set_analogue_input (MCUSSimulation *self, gdouble analogue_input)
{
	g_return_if_fail (MCUS_IS_SIMULATION (self));
	g_return_if_fail (analogue_input >= 0.0 && analogue_input <= 5.0);

	self->priv->analogue_input = analogue_input;
	g_object_notify (G_OBJECT (self), "analogue-input");
}
