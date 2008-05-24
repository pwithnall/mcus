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
#include <gtksourceview/gtksourcelanguagemanager.h>

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

#define REGISTER_COUNT 8
#define MEMORY_SIZE 256
#define STACK_SIZE 64
#define PROGRAM_START_ADDRESS 0
/* This is also in the UI file */
#define ANALOGUE_INPUT_MAX_VOLTAGE 5.0
/* This is also in the UI file (in Hz) */
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
	MCUSInstructionOffset *offset_map; /* maps memory locations to the text buffer offsets where the corresponding instructions are */

	GtkTextTag *current_instruction_tag;
	GtkTextTag *error_tag;
	gchar *current_filename;
	GtkSourceLanguageManager *language_manager;
} MCUS;

MCUS *mcus;

gboolean mcus_save_changes (void);
void mcus_new_program (void);
void mcus_open_program (void);
void mcus_save_program (void);
void mcus_save_program_as (void);
void mcus_quit (void);

G_END_DECLS

#endif /* MCUS_MAIN_H */
