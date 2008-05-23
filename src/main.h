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

#ifndef MCUS_MAIN_H
#define MCUS_MAIN_H

G_BEGIN_DECLS

typedef enum {
	SIMULATION_STOPPED,
	SIMULATION_PAUSED,
	SIMULATION_RUNNING
} MCUSSimulationState;

#define REGISTER_COUNT 8
#define MEMORY_SIZE 256
#define STACK_SIZE 64
#define PROGRAM_START_ADDRESS 0
/* This is also in the UI file */
#define ANALOGUE_INPUT_MAX_VOLTAGE 5.0
/* This is also in the UI file */
#define DEFAULT_CLOCK_SPEED 1

typedef struct {
	GtkWidget *main_window;
	GtkBuilder *builder;

	guchar program_counter;
	guchar stack_pointer;
	gboolean zero_flag;
	guchar registers[REGISTER_COUNT];
	guchar input_port;
	guchar output_port;
	gdouble analogue_input;
	guchar memory[MEMORY_SIZE];
	guchar stack[STACK_SIZE];

	gulong clock_speed;
	gboolean debug;
	guint iteration;
	MCUSSimulationState simulation_state;
} MCUS;

MCUS *mcus;

void mcus_print_debug_data (void);
void mcus_quit (void);

G_END_DECLS

#endif /* MCUS_MAIN_H */
