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

#define MCUS_SIMULATION_ERROR		(mcus_simulation_error_quark ())

enum {
	MCUS_SIMULATION_ERROR_MEMORY_OVERFLOW,
	MCUS_SIMULATION_ERROR_STACK_OVERFLOW,
	MCUS_SIMULATION_ERROR_STACK_UNDERFLOW,
	MCUS_SIMULATION_ERROR_INVALID_INSTRUCTION
};

#define REGISTER_COUNT 8
#define MEMORY_SIZE 256
#define STACK_SIZE 50
#define PROGRAM_START_ADDRESS 0

typedef struct {
	GtkWidget *main_window;

	guchar program_counter;
	guchar stack_pointer;
	gboolean zero_flag;
	guchar registers[REGISTER_COUNT];
	guchar input_port;
	guchar output_port;
	gfloat analogue_input;
	guchar memory[MEMORY_SIZE];
	guchar stack[STACK_SIZE];
	gulong clock_speed;

	gboolean debug;
	guint iteration;
} MCUS;

MCUS *mcus;

GQuark mcus_simulation_error_quark (void);
void mcus_initialise_simulation (gulong clock_speed);
gboolean mcus_iterate_simulation (GError **error);
void mcus_print_debug_data (void);
void mcus_quit (void);

G_END_DECLS

#endif /* MCUS_MAIN_H */
