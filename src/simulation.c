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
#include "simulation-enums.h"

/* This is also in the UI file (in Volts) */
#define ANALOGUE_INPUT_MAX_VOLTAGE 5.0

/* These are also in the UI file (in Hz) */
#define DEFAULT_CLOCK_SPEED 1
#define MAX_CLOCK_SPEED 1000

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

static void free_stack (MCUSSimulation *self);

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
	MCUSSimulationState state;
	gulong clock_speed;
	guint iteration_event;
};

enum {
	PROP_PROGRAM_COUNTER = 1,
	PROP_ZERO_FLAG,
	PROP_INPUT_PORT,
	PROP_OUTPUT_PORT,
	PROP_ANALOGUE_INPUT,
	PROP_ITERATION,
	PROP_STATE,
	PROP_CLOCK_SPEED,
	PROP_MEMORY,
	PROP_LOOKUP_TABLE,
	PROP_REGISTERS
};

enum {
	SIGNAL_ITERATION_STARTED,
	SIGNAL_ITERATION_FINISHED,
	SIGNAL_STACK_PUSHED,
	SIGNAL_STACK_POPPED,
	SIGNAL_STACK_EMPTIED,
	LAST_SIGNAL
};

static int signals[LAST_SIGNAL] = { 0, };

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
	 * MCUSSimulation:state:
	 *
	 * The state of execution of the simulation.
	 **/
	g_object_class_install_property (gobject_class, PROP_STATE,
				g_param_spec_enum ("state",
					"State", "The state of execution of the simulation.",
					MCUS_TYPE_SIMULATION_STATE, MCUS_SIMULATION_STOPPED,
					G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * MCUSSimulation:clock-speed:
	 *
	 * The frequency of simulation execution, in Hertz.
	 **/
	g_object_class_install_property (gobject_class, PROP_CLOCK_SPEED,
				g_param_spec_ulong ("clock-speed",
					"Clock Speed", "The frequency of simulation execution, in Hertz.",
					1, G_MAXULONG, DEFAULT_CLOCK_SPEED,
					G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * MCUSSimulation:memory:
	 *
	 * The statically allocated block of memory for the microcontroller.
	 **/
	g_object_class_install_property (gobject_class, PROP_MEMORY,
				g_param_spec_pointer ("memory",
					"Memory", "The statically allocated block of memory for the microcontroller.",
					G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * MCUSSimulation:lookup-table:
	 *
	 * The statically allocated block of memory for the microcontroller's lookup table.
	 **/
	g_object_class_install_property (gobject_class, PROP_LOOKUP_TABLE,
				g_param_spec_pointer ("lookup-table",
					"Lookup Table", "The statically allocated block of memory for the microcontroller's lookup table.",
					G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * MCUSSimulation:registers:
	 *
	 * The statically allocated block of memory for the microcontroller's registers.
	 **/
	g_object_class_install_property (gobject_class, PROP_REGISTERS,
				g_param_spec_pointer ("registers",
					"Registers", "The statically allocated block of memory for the microcontroller's registers.",
					G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * MCUSSimulation::iteration-started:
	 * @simulation: the #MCUSSimulation which has started an iteration
	 *
	 * Emitted when an iteration of the simulation is just about to be started, and the microcontroller is about to scan the inputs.
	 **/
	signals[SIGNAL_ITERATION_STARTED] = g_signal_new ("iteration-started",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				0,
				NULL, NULL,
				g_cclosure_marshal_VOID__VOID,
				G_TYPE_NONE, 0);

	/**
	 * MCUSSimulation::iteration-finished:
	 * @simulation: the #MCUSSimulation which has finished an iteration
	 * @error: a #GError representing any error which occurred during the iteration, or %NULL
	 *
	 * Emitted when an iteration of the simulation has been finished and output data is ready to be pushed to the UI.
	 *
	 * If an error occurred during the iteration, it is returned in @error. Otherwise, @error is %NULL.
	 **/
	signals[SIGNAL_ITERATION_FINISHED] = g_signal_new ("iteration-finished",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				0,
				NULL, NULL,
				g_cclosure_marshal_VOID__POINTER,
				G_TYPE_NONE, 1, G_TYPE_POINTER /* GError */);

	/**
	 * MCUSSimulation::stack-pushed:
	 * @simulation: the #MCUSSimulation which has had a new frame pushed onto its stack
	 * @frame: the #MCUSStackFrame which was pushed onto the stack
	 *
	 * Emitted when a new frame is pushed onto the simulation's stack.
	 **/
	signals[SIGNAL_STACK_PUSHED] = g_signal_new ("stack-pushed",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				0,
				NULL, NULL,
				g_cclosure_marshal_VOID__POINTER,
				G_TYPE_NONE, 1, G_TYPE_POINTER /* MCUSStackFrame */);

	/**
	 * MCUSSimulation::stack-popped:
	 * @simulation: the #MCUSSimulation which has had a frame popped off its stack
	 * @frame: the #MCUSStackFrame which is the new head of the stack, or %NULL
	 *
	 * Emitted when a frame is popped off the simulation's stack. The @frame passed to the signal
	 * handler is the frame which is now at the top of the stack, or %NULL if the stack is now empty.
	 **/
	signals[SIGNAL_STACK_POPPED] = g_signal_new ("stack-popped",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				0,
				NULL, NULL,
				g_cclosure_marshal_VOID__POINTER,
				G_TYPE_NONE, 1, G_TYPE_POINTER /* MCUSStackFrame */);

	/**
	 * MCUSSimulation::stack-emptied:
	 * @simulation: the #MCUSSimulation which had its stack emptied
	 *
	 * Emitted when the stack of the simulation is emptied completely.
	 **/
	signals[SIGNAL_STACK_EMPTIED] = g_signal_new ("stack-emptied",
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

	self->priv->state = MCUS_SIMULATION_STOPPED;
	self->priv->clock_speed = DEFAULT_CLOCK_SPEED;
}

static void
mcus_simulation_finalize (GObject *object)
{
	MCUSSimulation *self = MCUS_SIMULATION (object);

	if (self->priv->state != MCUS_SIMULATION_STOPPED)
		mcus_simulation_finish (self);
	free_stack (self);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (mcus_simulation_parent_class)->finalize (object);
}

static void
mcus_simulation_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	MCUSSimulationPrivate *priv = MCUS_SIMULATION (object)->priv;

	switch (property_id) {
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
		case PROP_ITERATION:
			g_value_set_uint (value, priv->iteration);
			break;
		case PROP_STATE:
			g_value_set_enum (value, priv->state);
			break;
		case PROP_CLOCK_SPEED:
			g_value_set_ulong (value, priv->clock_speed);
			break;
		case PROP_MEMORY:
			g_value_set_pointer (value, priv->memory);
			break;
		case PROP_LOOKUP_TABLE:
			g_value_set_pointer (value, priv->lookup_table);
			break;
		case PROP_REGISTERS:
			g_value_set_pointer (value, priv->registers);
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
		case PROP_CLOCK_SPEED:
			mcus_simulation_set_clock_speed (MCUS_SIMULATION (object), g_value_get_ulong (value));
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

MCUSSimulation *
mcus_simulation_new (void)
{
	return g_object_new (MCUS_TYPE_SIMULATION, NULL);
}

static void
free_stack (MCUSSimulation *self)
{
	MCUSStackFrame *stack_frame = self->priv->stack;
	while (stack_frame != NULL) {
		MCUSStackFrame *prev_frame;
		prev_frame = stack_frame->prev;
		g_slice_free (MCUSStackFrame, stack_frame);
		stack_frame = prev_frame;
	}
	self->priv->stack = NULL;

	g_signal_emit (self, signals[SIGNAL_STACK_EMPTIED], 0);
}

static void
reset (MCUSSimulation *self, gboolean reset_memory)
{
	GObject *obj = G_OBJECT (self);
	MCUSSimulationPrivate *priv = self->priv;

	g_object_freeze_notify (obj);

	/* Reset the memory */
	if (reset_memory == TRUE) {
		memset (priv->memory, 0, sizeof (guchar) * MEMORY_SIZE);
		g_object_notify (obj, "memory");

		memset (priv->lookup_table, 0, sizeof (guchar) * LOOKUP_TABLE_SIZE);
		g_object_notify (obj, "lookup-table");
	}

	/* Set up various properties */
	priv->program_counter = PROGRAM_START_ADDRESS;
	g_object_notify (obj, "program-counter");

	priv->zero_flag = 0;
	g_object_notify (obj, "zero-flag");

	memset (priv->registers, 0, sizeof (guchar) * REGISTER_COUNT);
	g_object_notify (obj, "registers");

	priv->output_port = 0;
	g_object_notify (obj, "output-port");

	priv->iteration = 0;
	g_object_notify (obj, "iteration");

	g_object_thaw_notify (obj);

	/* Empty the stack after all the notifications, so that the signal handler for the resulting signal can read a consistent
	 * state from the rest of the microcontroller */
	free_stack (self);
}

/* Reset the simulated microcontroller as if it was rebooted */
void
mcus_simulation_reset (MCUSSimulation *self)
{
	g_return_if_fail (MCUS_IS_SIMULATION (self));
	g_return_if_fail (self->priv->state == MCUS_SIMULATION_STOPPED);

	reset (self, TRUE);
}

static gboolean
simulation_iterate_cb (MCUSSimulation *self)
{
	/* Remove the timeout event if we're not running */
	if (self->priv->state != MCUS_SIMULATION_RUNNING)
		return FALSE;

	if (mcus_simulation_iterate (self, NULL) == FALSE)
		return FALSE;

	return TRUE;
}

void
mcus_simulation_start (MCUSSimulation *self)
{
	MCUSSimulationPrivate *priv = self->priv;

	g_return_if_fail (MCUS_IS_SIMULATION (self));
	g_return_if_fail (priv->state == MCUS_SIMULATION_STOPPED);

	/* Reset the microcontroller state */
	reset (self, FALSE);

	priv->state = MCUS_SIMULATION_RUNNING;
	g_object_notify (G_OBJECT (self), "state");

	/* Add the timeout for the simulation iterations */
	priv->iteration_event = g_timeout_add (1000 / priv->clock_speed, (GSourceFunc) simulation_iterate_cb, self);
}

/* Returns FALSE on error or if the simulation's ended */
gboolean
mcus_simulation_iterate (MCUSSimulation *self, GError **error)
{
	guchar opcode, operand1, operand2;
	MCUSStackFrame *stack_frame;
	MCUSSimulationState old_state;
	MCUSSimulationPrivate *priv = self->priv;

	g_return_val_if_fail (MCUS_IS_SIMULATION (self), FALSE);
	g_return_val_if_fail (priv->state != MCUS_SIMULATION_STOPPED, FALSE);

	/* If iterate() is called while we're paused, we temporarily go to the running state */
	old_state = priv->state;
	if (old_state == MCUS_SIMULATION_PAUSED) {
		priv->state = MCUS_SIMULATION_RUNNING;
		g_object_notify (G_OBJECT (self), "state");
	}

	/* Can't check it with >= as it does a check against guchar, which
	 * is always true due to the datatype's range. */
	if (priv->program_counter + 1 > MEMORY_SIZE) {
		GError *real_error = g_error_new (MCUS_SIMULATION_ERROR, MCUS_SIMULATION_ERROR_MEMORY_OVERFLOW,
		                                  _("The program counter overflowed available memory in simulation iteration %u."),
		                                  priv->iteration);
		g_signal_emit (self, signals[SIGNAL_ITERATION_FINISHED], 0, real_error);
		g_propagate_error (error, real_error);

		mcus_simulation_finish (self);
		return FALSE;
	}

	/* Signal the start of the iteration */
	g_signal_emit (self, signals[SIGNAL_ITERATION_STARTED], 0);

	/* Fetch and decode the instruction */
	opcode = priv->memory[priv->program_counter];
	operand1 = (priv->program_counter + 1 < MEMORY_SIZE) ? priv->memory[priv->program_counter + 1] : 0;
	operand2 = (priv->program_counter + 2 < MEMORY_SIZE) ? priv->memory[priv->program_counter + 2] : 0;

	g_object_freeze_notify (G_OBJECT (self));

	switch (opcode) {
	case OPCODE_HALT:
		g_object_thaw_notify (G_OBJECT (self));
		g_signal_emit (self, signals[SIGNAL_ITERATION_FINISHED], 0, NULL);

		mcus_simulation_finish (self);
		return FALSE;
	case OPCODE_MOVI:
		priv->registers[operand1] = operand2;
		g_object_notify (G_OBJECT (self), "registers");
		break;
	case OPCODE_MOV:
		priv->registers[operand1] = priv->registers[operand2];
		g_object_notify (G_OBJECT (self), "registers");
		break;
	case OPCODE_ADD:
		priv->registers[operand1] += priv->registers[operand2];
		priv->zero_flag = (priv->registers[operand1] == 0) ? TRUE : FALSE;
		g_object_notify (G_OBJECT (self), "zero-flag");
		g_object_notify (G_OBJECT (self), "registers");
		break;
	case OPCODE_SUB:
		priv->registers[operand1] -= priv->registers[operand2];
		priv->zero_flag = (priv->registers[operand1] == 0) ? TRUE : FALSE;
		g_object_notify (G_OBJECT (self), "zero-flag");
		g_object_notify (G_OBJECT (self), "registers");
		break;
	case OPCODE_AND:
		priv->registers[operand1] &= priv->registers[operand2];
		priv->zero_flag = (priv->registers[operand1] == 0) ? TRUE : FALSE;
		g_object_notify (G_OBJECT (self), "zero-flag");
		g_object_notify (G_OBJECT (self), "registers");
		break;
	case OPCODE_EOR:
		priv->registers[operand1] ^= priv->registers[operand2];
		priv->zero_flag = (priv->registers[operand1] == 0) ? TRUE : FALSE;
		g_object_notify (G_OBJECT (self), "zero-flag");
		g_object_notify (G_OBJECT (self), "registers");
		break;
	case OPCODE_INC:
		priv->registers[operand1] += 1;
		priv->zero_flag = (priv->registers[operand1] == 0) ? TRUE : FALSE;
		g_object_notify (G_OBJECT (self), "zero-flag");
		g_object_notify (G_OBJECT (self), "registers");
		break;
	case OPCODE_DEC:
		priv->registers[operand1] -= 1;
		priv->zero_flag = (priv->registers[operand1] == 0) ? TRUE : FALSE;
		g_object_notify (G_OBJECT (self), "zero-flag");
		g_object_notify (G_OBJECT (self), "registers");
		break;
	case OPCODE_IN:
		priv->registers[operand1] = priv->input_port; /* only one operand is stored */
		g_object_notify (G_OBJECT (self), "registers");
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
			g_object_notify (G_OBJECT (self), "registers");
			break;
		} else if (operand1 == priv->program_counter + 1) {
			/* wait1ms */
			g_usleep (1000);
			break;
		} else if (operand1 == priv->program_counter + 2) {
			/* readadc */
			priv->registers[0] = 255.0 * priv->analogue_input / ANALOGUE_INPUT_MAX_VOLTAGE;
			g_object_notify (G_OBJECT (self), "registers");
			break;
		}

		/* If we're just calling a normal subroutine, push the
		 * current state as a new frame onto the stack */
		stack_frame = g_slice_new (MCUSStackFrame);
		stack_frame->prev = priv->stack;
		stack_frame->program_counter = priv->program_counter + mcus_instruction_data[opcode].size;
		memcpy (stack_frame->registers, priv->registers, sizeof (guchar) * REGISTER_COUNT);

		priv->stack = stack_frame;

		/* Signal that the stack's changed */
		g_signal_emit (self, signals[SIGNAL_STACK_PUSHED], 0, stack_frame);

		/* Jump to the subroutine */
		priv->program_counter = operand1;
		g_object_notify (G_OBJECT (self), "program-counter");
		goto update_and_exit;
	case OPCODE_RET:
		/* Check for underflows */
		if (priv->stack == NULL) {
			GError *real_error = g_error_new (MCUS_SIMULATION_ERROR, MCUS_SIMULATION_ERROR_STACK_UNDERFLOW,
			                                  _("The stack pointer underflowed available stack space in simulation iteration %u."),
			                                  priv->iteration);
			g_object_thaw_notify (G_OBJECT (self));
			g_signal_emit (self, signals[SIGNAL_ITERATION_FINISHED], 0, real_error);
			g_propagate_error (error, real_error);

			mcus_simulation_finish (self);
			return FALSE;
		}

		/* Pop the old state off the stack */
		stack_frame = priv->stack;
		priv->stack = stack_frame->prev;
		priv->program_counter = stack_frame->program_counter;
		g_object_notify (G_OBJECT (self), "program-counter");
		memcpy (priv->registers, stack_frame->registers, sizeof (guchar) * REGISTER_COUNT);
		g_object_notify (G_OBJECT (self), "registers");
		g_slice_free (MCUSStackFrame, stack_frame);

		/* Signal that the stack's changed */
		g_signal_emit (self, signals[SIGNAL_STACK_POPPED], 0, priv->stack);

		goto update_and_exit;
	case OPCODE_SHL:
		priv->registers[operand1] <<= 1;
		priv->zero_flag = (priv->registers[operand1] == 0) ? TRUE : FALSE;
		g_object_notify (G_OBJECT (self), "zero-flag");
		g_object_notify (G_OBJECT (self), "registers");
		break;
	case OPCODE_SHR:
		priv->registers[operand1] >>= 1;
		priv->zero_flag = (priv->registers[operand1] == 0) ? TRUE : FALSE;
		g_object_notify (G_OBJECT (self), "zero-flag");
		g_object_notify (G_OBJECT (self), "registers");
		break;
	default: {
		/* We've encountered some data? */
		GError *real_error = g_error_new (MCUS_SIMULATION_ERROR, MCUS_SIMULATION_ERROR_INVALID_OPCODE,
		                                  _("An invalid opcode \"%02X\" was encountered at address %02X in simulation iteration %u."),
		                                  (guint) opcode,
		                                  (guint) priv->program_counter,
		                                  priv->iteration);
		g_object_thaw_notify (G_OBJECT (self));
		g_signal_emit (self, signals[SIGNAL_ITERATION_FINISHED], 0, real_error);
		g_propagate_error (error, real_error);

		mcus_simulation_finish (self);
		return FALSE;
	}}

	/* Don't forget to increment the PC */
	priv->program_counter += mcus_instruction_data[opcode].size;
	g_object_notify (G_OBJECT (self), "program-counter");

update_and_exit:
	g_object_thaw_notify (G_OBJECT (self));

	/* Reset the simulation state if we changed it to step forward */
	if (old_state == MCUS_SIMULATION_PAUSED) {
		priv->state = MCUS_SIMULATION_PAUSED;
		g_object_notify (G_OBJECT (self), "state");
	}

	/* Announce that we've finished the iteration */
	g_signal_emit (self, signals[SIGNAL_ITERATION_FINISHED], 0, NULL);
	priv->iteration++;

	return TRUE;
}

void
mcus_simulation_pause (MCUSSimulation *self)
{
	MCUSSimulationPrivate *priv = self->priv;

	g_return_if_fail (MCUS_IS_SIMULATION (self));
	g_return_if_fail (priv->state == MCUS_SIMULATION_RUNNING);

	/* Stop timeouts */
	if (priv->iteration_event != 0)
		g_source_remove (priv->iteration_event);
	priv->iteration_event = 0;

	priv->state = MCUS_SIMULATION_PAUSED;
	g_object_notify (G_OBJECT (self), "state");
}

void
mcus_simulation_resume (MCUSSimulation *self)
{
	g_return_if_fail (MCUS_IS_SIMULATION (self));
	g_return_if_fail (self->priv->state == MCUS_SIMULATION_PAUSED);

	self->priv->state = MCUS_SIMULATION_RUNNING;
	g_object_notify (G_OBJECT (self), "state");

	/* Resume the timeouts for simulation iterations */
	self->priv->iteration_event = g_timeout_add (1000 / self->priv->clock_speed, (GSourceFunc) simulation_iterate_cb, self);
}

void
mcus_simulation_finish (MCUSSimulation *self)
{
	MCUSSimulationPrivate *priv = self->priv;

	g_return_if_fail (MCUS_IS_SIMULATION (self));
	g_return_if_fail (priv->state != MCUS_SIMULATION_STOPPED);

	/* Stop timeouts */
	if (priv->iteration_event != 0)
		g_source_remove (priv->iteration_event);
	priv->iteration_event = 0;

	/* Stop the simulation */
	priv->state = MCUS_SIMULATION_STOPPED;
	g_object_notify (G_OBJECT (self), "state");
}

/**
 * mcus_simulation_get_memory:
 * @self: an #MCUSSimulation
 *
 * Returns the block of memory which is used by the simulated microcontroller to store compiled programs. This memory is statically allocated,
 * and should not be freed. It may, however, be modified by the caller, but mcus_simulation_notify_memory() must be called afterwards.
 *
 * Return value: the microcontroller's memory
 **/
guchar *
mcus_simulation_get_memory (MCUSSimulation *self)
{
	g_return_val_if_fail (MCUS_IS_SIMULATION (self), NULL);
	return self->priv->memory;
}

/**
 * mcus_simulation_notify_memory:
 * @self: an #MCUSSimulation
 *
 * This notifies the simulation that its program memory has been modified. This should be called and only be called after modifying the memory
 * returned by mcus_simulation_get_memory().
 **/
void
mcus_simulation_notify_memory (MCUSSimulation *self)
{
	g_return_if_fail (MCUS_IS_SIMULATION (self));
	g_object_notify (G_OBJECT (self), "memory");
}

/**
 * mcus_simulation_get_lookup_table:
 * @self: an #MCUSSimulation
 *
 * Returns the block of memory which is used by the simulated microcontroller to store lookup tables. This memory is statically allocated,
 * and should not be freed. It may, however, be modified by the caller, but mcus_simulation_notify_lookup_table() must be called afterwards.
 *
 * Return value: the microcontroller's lookup table memory
 **/
guchar *
mcus_simulation_get_lookup_table (MCUSSimulation *self)
{
	g_return_val_if_fail (MCUS_IS_SIMULATION (self), NULL);
	return self->priv->lookup_table;
}

/**
 * mcus_simulation_notify_lookup_table:
 * @self: an #MCUSSimulation
 *
 * This notifies the simulation that its lookup table memory has been modified. This should be called and only be called after modifying the memory
 * returned by mcus_simulation_get_lookup_table().
 **/
void
mcus_simulation_notify_lookup_table (MCUSSimulation *self)
{
	g_return_if_fail (MCUS_IS_SIMULATION (self));
	g_object_notify (G_OBJECT (self), "lookup-table");
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

MCUSSimulationState
mcus_simulation_get_state (MCUSSimulation *self)
{
	g_return_val_if_fail (MCUS_IS_SIMULATION (self), MCUS_SIMULATION_STOPPED);
	return self->priv->state;
}

gulong
mcus_simulation_get_clock_speed (MCUSSimulation *self)
{
	g_return_val_if_fail (MCUS_IS_SIMULATION (self), 0);
	return self->priv->clock_speed;
}

void
mcus_simulation_set_clock_speed (MCUSSimulation *self, gulong clock_speed)
{
	MCUSSimulationPrivate *priv = self->priv;

	g_return_if_fail (MCUS_IS_SIMULATION (self));
	g_return_if_fail (clock_speed <= MAX_CLOCK_SPEED);

	/* Set the clock speed */
	priv->clock_speed = clock_speed;

	/* Change the events if we're running */
	if (priv->state == MCUS_SIMULATION_RUNNING) {
		if (priv->iteration_event != 0)
			g_source_remove (priv->iteration_event);
		priv->iteration_event = g_timeout_add (1000 / clock_speed, (GSourceFunc) simulation_iterate_cb, self);
	}
}
