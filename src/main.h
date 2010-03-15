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

#include "simulation.h"
#include "main-window.h"

#ifndef MCUS_MAIN_H
#define MCUS_MAIN_H

G_BEGIN_DECLS

typedef enum {
	SIMULATION_STOPPED,
	SIMULATION_PAUSED,
	SIMULATION_RUNNING
} MCUSSimulationState;

typedef struct {
	gint offset;
	guint length;
} MCUSInstructionOffset;

typedef struct {
	/* Simulation state */
	MCUSSimulation *simulation;
	MCUSSimulationState simulation_state;
	gulong clock_speed;
	gboolean debug;
	MCUSInstructionOffset *offset_map; /* maps memory locations to the text buffer offsets where the corresponding instructions are */
} MCUS;

MCUS *mcus;

const gchar *mcus_get_data_directory (void);
void mcus_quit (MCUSMainWindow *main_window);

G_END_DECLS

#endif /* MCUS_MAIN_H */
