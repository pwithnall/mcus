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

#ifndef MCUS_SIMULATION_H
#define MCUS_SIMULATION_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define MCUS_SIMULATION_ERROR		(mcus_simulation_error_quark ())

enum {
	MCUS_SIMULATION_ERROR_MEMORY_OVERFLOW,
	MCUS_SIMULATION_ERROR_STACK_OVERFLOW,
	MCUS_SIMULATION_ERROR_STACK_UNDERFLOW,
	MCUS_SIMULATION_ERROR_INVALID_OPCODE
};

GQuark mcus_simulation_error_quark (void) G_GNUC_CONST;

#define MCUS_TYPE_SIMULATION		(mcus_simulation_get_type ())
#define MCUS_SIMULATION(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), MCUS_TYPE_SIMULATION, MCUSSimulation))
#define MCUS_SIMULATION_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), MCUS_TYPE_SIMULATION, MCUSSimulationClass))
#define MCUS_IS_SIMULATION(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), MCUS_TYPE_SIMULATION))
#define MCUS_IS_SIMULATION_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), MCUS_TYPE_SIMULATION))
#define MCUS_SIMULATION_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), MCUS_TYPE_SIMULATION, MCUSSimulationClass))

typedef struct _MCUSSimulationPrivate	MCUSSimulationPrivate;

typedef struct {
	GObject parent;
	MCUSSimulationPrivate *priv;
} MCUSSimulation;

typedef struct {
	GObjectClass parent;
} MCUSSimulationClass;

GType mcus_simulation_get_type (void) G_GNUC_CONST;

void mcus_simulation_start (MCUSSimulation *self);
gboolean mcus_simulation_iterate (MCUSSimulation *self, GError **error);
void mcus_simulation_finish (MCUSSimulation *self);

guchar *mcus_simulation_get_memory (MCUSSimulation *self);
guchar *mcus_simulation_get_lookup_table (MCUSSimulation *self);
guchar *mcus_simulation_get_registers (MCUSSimulation *self);

#include "main.h"
MCUSStackFrame *mcus_simulation_get_stack_head (MCUSSimulation *self);

guint mcus_simulation_get_iteration (MCUSSimulation *self);
guchar mcus_simulation_get_program_counter (MCUSSimulation *self);
gboolean mcus_simulation_get_zero_flag (MCUSSimulation *self);
guchar mcus_simulation_get_output_port (MCUSSimulation *self);

guchar mcus_simulation_get_input_port (MCUSSimulation *self);
void mcus_simulation_set_input_port (MCUSSimulation *self, guchar input_port);

gdouble mcus_simulation_get_analogue_input (MCUSSimulation *self);
void mcus_simulation_set_analogue_input (MCUSSimulation *self, gdouble analogue_input);

G_END_DECLS

#endif /* !MCUS_SIMULATION_H */
