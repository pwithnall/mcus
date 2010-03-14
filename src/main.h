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

/* These correspond to the tabs in the UI */
typedef enum {
	ANALOGUE_INPUT_LINEAR_DEVICE = 0,
	ANALOGUE_INPUT_FUNCTION_GENERATOR_DEVICE
} MCUSAnalogueInputDevice;

typedef enum {
	OUTPUT_LED_DEVICE = 0,
	OUTPUT_SINGLE_SSD_DEVICE,
	OUTPUT_DUAL_SSD_DEVICE,
	OUTPUT_MULTIPLEXED_SSD_DEVICE
} MCUSOutputDevice;

#define REGISTER_COUNT 8
#define MEMORY_SIZE 256
#define LOOKUP_TABLE_SIZE 256
/* The number of stack frames to show */
#define STACK_PREVIEW_SIZE 5
#define PROGRAM_START_ADDRESS 0
/* This is also in the UI file (in Volts) */
#define ANALOGUE_INPUT_MAX_VOLTAGE 5.0
/* This is also in the UI file (in Hz) */
#define DEFAULT_CLOCK_SPEED 1

typedef struct _MCUSStackFrame MCUSStackFrame;

struct _MCUSStackFrame {
	guchar program_counter;
	guchar registers[REGISTER_COUNT];
	MCUSStackFrame *prev;
};

typedef struct {
	GtkWidget *main_window;
	GtkBuilder *builder;

	/* Microcontroller components */
	guchar program_counter;
	gboolean zero_flag;
	guchar registers[REGISTER_COUNT];
	guchar input_port;
	guchar output_port;
	gdouble analogue_input;
	guchar memory[MEMORY_SIZE];
	guchar lookup_table[LOOKUP_TABLE_SIZE];
	MCUSStackFrame *stack;

	/* Simulation state */
	gulong clock_speed;
	gboolean debug;
	guint iteration;
	MCUSSimulationState simulation_state;
	MCUSInstructionOffset *offset_map; /* maps memory locations to the text buffer offsets where the corresponding instructions are */
	MCUSAnalogueInputDevice analogue_input_device;
	MCUSOutputDevice output_device;

	/* Interface */
	GtkTextTag *current_instruction_tag;
	GtkTextTag *error_tag;
	gchar *current_filename;
	GtkSourceLanguageManager *language_manager;

	/* Analogue input interface */
	GtkAdjustment *analogue_input_frequency_adjustment;
	GtkAdjustment *analogue_input_amplitude_adjustment;
	GtkAdjustment *analogue_input_offset_adjustment;
	GtkAdjustment *analogue_input_phase_adjustment;
} MCUS;

MCUS *mcus;

gboolean mcus_save_changes (void);
void mcus_new_program (void);
void mcus_open_program (void);
void mcus_save_program (void);
void mcus_save_program_as (void);
void mcus_open_file (gchar *filename);
const gchar *mcus_get_data_directory (void);
void mcus_quit (void);

G_END_DECLS

#endif /* MCUS_MAIN_H */
