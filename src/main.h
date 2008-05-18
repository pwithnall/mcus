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

#define REGISTER_COUNT 8
#define MEMORY_SIZE 256
#define PROGRAM_START_ADDRESS 0

typedef struct {
	GtkWidget *main_window;

	gchar program_counter;
	gchar stack_pointer;
	gchar registers[REGISTER_COUNT];
	gchar input_port;
	gchar output_port;
	gchar memory[MEMORY_SIZE];
	glong clock_speed;

	/* TODO: Analogue input */

	gboolean debug;
} MCUS;

MCUS *mcus;

void mcus_quit (void);

G_END_DECLS

#endif /* MCUS_MAIN_H */
