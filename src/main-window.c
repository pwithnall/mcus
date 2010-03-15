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
#include <pango/pango.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <gtksourceview/gtksourceview.h>
#include <gtksourceview/gtksourceprintcompositor.h>
#include <gtksourceview/gtksourcelanguagemanager.h>
#include <math.h>

#include "main-window.h"
#include "config.h"
#include "main.h"
#include "compiler.h"
#include "simulation.h"
#include "widgets/led.h"
#include "widgets/seven-segment-display.h"

GQuark
mcus_io_error_quark (void)
{
	static GQuark q = 0;

	if (q == 0)
		q = g_quark_from_static_string ("mcus-io-error-quark");

	return q;
}

static void mcus_main_window_dispose (GObject *object);
static void mcus_main_window_finalize (GObject *object);

static void update_simulation_ui (MCUSMainWindow *self);
static void read_analogue_input (MCUSMainWindow *self);
static void remove_tag (MCUSMainWindow *self, GtkTextTag *tag);
static void tag_range (MCUSMainWindow *self, GtkTextTag *tag, guint start_offset, guint end_offset, gboolean remove_previous_occurrences);
static gboolean input_port_read_entry (MCUSMainWindow *self, GError **error);
static void input_port_update_entry (MCUSMainWindow *self);
static void input_port_read_check_buttons (MCUSMainWindow *self);
static void input_port_update_check_buttons (MCUSMainWindow *self);

/* Normal callbacks */
static void simulation_iteration_started_cb (MCUSSimulation *self, MCUSMainWindow *main_window);
static void simulation_iteration_finished_cb (MCUSSimulation *self, GError *error, MCUSMainWindow *main_window);
static void notify_simulation_state_cb (GObject *object, GParamSpec *param_spec, MCUSMainWindow *main_window);
static void notify_can_undo_cb (GObject *object, GParamSpec *param_spec, MCUSMainWindow *main_window);
static void notify_can_redo_cb (GObject *object, GParamSpec *param_spec, MCUSMainWindow *main_window);
static void notify_has_selection_cb (GObject *object, GParamSpec *param_spec, MCUSMainWindow *main_window);
static void notify_program_counter_cb (GObject *object, GParamSpec *param_spec, MCUSMainWindow *main_window);
static void notify_zero_flag_cb (GObject *object, GParamSpec *param_spec, MCUSMainWindow *main_window);
static void notify_output_port_cb (GObject *object, GParamSpec *param_spec, MCUSMainWindow *main_window);
static void buffer_modified_changed_cb (GtkTextBuffer *self, MCUSMainWindow *main_window);

/* GtkBuilder callbacks */
G_MODULE_EXPORT void mw_input_entry_changed (GtkEntry *self, MCUSMainWindow *main_window);
G_MODULE_EXPORT void mw_input_check_button_toggled (GtkToggleButton *self, MCUSMainWindow *main_window);
G_MODULE_EXPORT void mw_output_single_ssd_option_changed_cb (GtkToggleButton *self, MCUSMainWindow *main_window);
G_MODULE_EXPORT void mw_output_notebook_switch_page_cb (GtkNotebook *self, GtkNotebookPage *page, guint page_num, MCUSMainWindow *main_window);
G_MODULE_EXPORT void mw_adc_constant_option_toggled_cb (GtkToggleButton *self, MCUSMainWindow *main_window);
G_MODULE_EXPORT gboolean mw_delete_event_cb (GtkWidget *widget, GdkEvent *event, MCUSMainWindow *main_window);
G_MODULE_EXPORT gboolean mw_key_press_event_cb (GtkWidget *widget, GdkEventKey *event, MCUSMainWindow *main_window);
G_MODULE_EXPORT void mw_quit_activate_cb (GtkAction *self, MCUSMainWindow *main_window);
G_MODULE_EXPORT void mw_undo_activate_cb (GtkAction *self, MCUSMainWindow *main_window);
G_MODULE_EXPORT void mw_redo_activate_cb (GtkAction *self, MCUSMainWindow *main_window);
G_MODULE_EXPORT void mw_cut_activate_cb (GtkAction *self, MCUSMainWindow *main_window);
G_MODULE_EXPORT void mw_copy_activate_cb (GtkAction *self, MCUSMainWindow *main_window);
G_MODULE_EXPORT void mw_paste_activate_cb (GtkAction *self, MCUSMainWindow *main_window);
G_MODULE_EXPORT void mw_delete_activate_cb (GtkAction *self, MCUSMainWindow *main_window);
G_MODULE_EXPORT void mw_run_activate_cb (GtkAction *self, MCUSMainWindow *main_window);
G_MODULE_EXPORT void mw_pause_activate_cb (GtkAction *self, MCUSMainWindow *main_window);
G_MODULE_EXPORT void mw_stop_activate_cb (GtkAction *self, MCUSMainWindow *main_window);
G_MODULE_EXPORT void mw_step_forward_activate_cb (GtkAction *self, MCUSMainWindow *main_window);
G_MODULE_EXPORT void mw_clock_speed_spin_button_value_changed_cb (GtkSpinButton *self, MCUSMainWindow *main_window);
G_MODULE_EXPORT void mw_contents_activate_cb (GtkAction *self, MCUSMainWindow *main_window);
G_MODULE_EXPORT void mw_about_activate_cb (GtkAction *self, MCUSMainWindow *main_window);
G_MODULE_EXPORT void mw_new_activate_cb (GtkAction *self, MCUSMainWindow *main_window);
G_MODULE_EXPORT void mw_open_activate_cb (GtkAction *self, MCUSMainWindow *main_window);
G_MODULE_EXPORT void mw_save_activate_cb (GtkAction *self, MCUSMainWindow *main_window);
G_MODULE_EXPORT void mw_save_as_activate_cb (GtkAction *self, MCUSMainWindow *main_window);
G_MODULE_EXPORT void mw_print_activate_cb (GtkAction *self, MCUSMainWindow *main_window);

/* The number of stack frames to show */
/* TODO: Shouldn't have this */
#define STACK_PREVIEW_SIZE 5

typedef enum {
	OUTPUT_LED_DEVICE = 0,
	OUTPUT_SINGLE_SSD_DEVICE,
	OUTPUT_DUAL_SSD_DEVICE,
	OUTPUT_MULTIPLEXED_SSD_DEVICE
} OutputDevice;

struct _MCUSMainWindowPrivate {
	/* Simulation */
	MCUSSimulation *simulation;

	/* Displays */
	GtkLabel *registers_label;
	GtkLabel *memory_label;
	GtkLabel *stack_label;
	GtkLabel *stack_pointer_label;
	GtkLabel *program_counter_label;
	GtkLabel *output_port_label;

	/* Analogue input interface */
	GtkAdjustment *adc_frequency_adjustment;
	GtkAdjustment *adc_amplitude_adjustment;
	GtkAdjustment *adc_offset_adjustment;
	GtkAdjustment *adc_phase_adjustment;

	GtkToggleButton *adc_constant_option;
	GtkToggleButton *adc_sine_wave_option;
	GtkToggleButton *adc_square_wave_option;
	GtkToggleButton *adc_triangle_wave_option;

	/* Input buttons */
	GtkToggleButton *input_check_button[8];

	/* Input port */
	GtkEntry *input_port_entry;

	/* Outputs */
	OutputDevice output_device;

	/* Single SSD output */
	MCUSSevenSegmentDisplay *output_single_ssd;
	GtkToggleButton *output_single_ssd_segment_option;

	/* Dual SSD output */
	MCUSSevenSegmentDisplay *output_dual_ssd[2];

	/* Multi SSD output */
	MCUSSevenSegmentDisplay *output_multi_ssd[16];

	/* LED output */
	MCUSLED *output_led[8];

	/* Code editing/highlighting */
	GtkTextBuffer *code_buffer;
	MCUSInstructionOffset *offset_map; /* maps memory locations to the text buffer offsets where the corresponding instructions are */
	GtkTextTag *current_instruction_tag;
	GtkTextTag *error_tag;
	GtkSourceLanguageManager *language_manager;

	/* File choosing */
	gchar *current_filename;
	GtkFileFilter *filter;

	/* Widgets which only need their sensitivity changing */
	GtkWidget *code_view;
	GtkWidget *adc_spin_button;
	GtkWidget *clock_speed_spin_button;
	GtkWidget *adc_hbox;
	GtkWidget *output_notebook;
	GtkWidget *input_alignment;
	GtkWidget *adc_frequency_spin_button;
	GtkWidget *adc_amplitude_spin_button;
	GtkWidget *adc_phase_spin_button;

	/* Menus */
	GtkActionGroup *file_action_group;
	GtkAction *save_action;
	GtkAction *save_as_action;
	GtkAction *undo_action;
	GtkAction *redo_action;
	GtkAction *cut_action;
	GtkAction *copy_action;
	GtkAction *paste_action;
	GtkAction *delete_action;
	GtkAction *run_action;
	GtkAction *pause_action;
	GtkAction *stop_action;
	GtkAction *step_forward_action;
};

G_DEFINE_TYPE (MCUSMainWindow, mcus_main_window, GTK_TYPE_WINDOW)

static void
mcus_main_window_class_init (MCUSMainWindowClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (MCUSMainWindowPrivate));

	gobject_class->dispose = mcus_main_window_dispose;
	gobject_class->finalize = mcus_main_window_finalize;
}

static void
mcus_main_window_init (MCUSMainWindow *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, MCUS_TYPE_MAIN_WINDOW, MCUSMainWindowPrivate);

	/* Set up the file filter */
	self->priv->filter = gtk_file_filter_new ();
	g_object_ref_sink (self->priv->filter);
	gtk_file_filter_add_pattern (self->priv->filter, "*.asm");

	/* Set up the simulation */
	self->priv->simulation = mcus_simulation_new ();
}

static void
mcus_main_window_dispose (GObject *object)
{
	MCUSMainWindowPrivate *priv = MCUS_MAIN_WINDOW (object)->priv;

	/* TODO */
	/*g_object_unref (priv->current_instruction_tag);
	g_object_unref (priv->error_tag);*/
	if (priv->language_manager != NULL)
		g_object_unref (priv->language_manager);
	priv->language_manager = NULL;

	if (priv->filter != NULL)
		g_object_unref (priv->filter);
	priv->filter = NULL;

	if (priv->simulation != NULL)
		g_object_unref (priv->simulation);
	priv->simulation = NULL;

	/* Chain up to the parent class */
	G_OBJECT_CLASS (mcus_main_window_parent_class)->dispose (object);
}

static void
mcus_main_window_finalize (GObject *object)
{
	MCUSMainWindowPrivate *priv = MCUS_MAIN_WINDOW (object)->priv;

	g_free (priv->current_filename);
	g_free (priv->offset_map);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (mcus_main_window_parent_class)->finalize (object);
}

GtkWindow *
mcus_main_window_new (void)
{
	GtkBuilder *builder;
	MCUSMainWindow *main_window;
	MCUSMainWindowPrivate *priv;
	GError *error = NULL;
	GtkTextBuffer *text_buffer;
	GtkSourceLanguage *language;
	const gchar * const *old_language_dirs;
	const gchar * const *language_ids;
	const gchar **language_dirs;
	guint i = 0;
	gchar *interface_filename;

	builder = gtk_builder_new ();

	interface_filename = g_build_filename (mcus_get_data_directory (), "mcus.ui", NULL);

	if (gtk_builder_add_from_file (builder, interface_filename, &error) == FALSE) {
		/* Show an error */
		GtkWidget *dialog = gtk_message_dialog_new (NULL,
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_OK,
				_("UI file \"%s\" could not be loaded"), interface_filename);
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", error->message);
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

		g_free (interface_filename);
		g_error_free (error);
		g_object_unref (builder);

		return NULL;
	}

	g_free (interface_filename);

	gtk_builder_set_translation_domain (builder, GETTEXT_PACKAGE);
	main_window = MCUS_MAIN_WINDOW (gtk_builder_get_object (builder, "mcus_main_window"));
	gtk_builder_connect_signals (builder, main_window);

	if (main_window == NULL) {
		g_object_unref (builder);
		return NULL;
	}

	priv = MCUS_MAIN_WINDOW (main_window)->priv;

	/* Grab our child widgets */
	priv->code_buffer = GTK_TEXT_BUFFER (gtk_builder_get_object (builder, "mw_code_buffer"));
	priv->registers_label = GTK_LABEL (gtk_builder_get_object (builder, "mw_registers_label"));
	priv->memory_label = GTK_LABEL (gtk_builder_get_object (builder, "mw_memory_label"));
	priv->stack_label = GTK_LABEL (gtk_builder_get_object (builder, "mw_stack_label"));
	priv->stack_pointer_label = GTK_LABEL (gtk_builder_get_object (builder, "mw_stack_pointer_label"));
	priv->program_counter_label = GTK_LABEL (gtk_builder_get_object (builder, "mw_program_counter_label"));
	priv->output_port_label = GTK_LABEL (gtk_builder_get_object (builder, "mw_output_port_label"));

	/* Grab widgets which only need their sensitivity changing */
	priv->code_view = GTK_WIDGET (gtk_builder_get_object (builder, "mw_code_view"));
	priv->adc_spin_button = GTK_WIDGET (gtk_builder_get_object (builder, "mw_analogue_input_spin_button"));
	priv->clock_speed_spin_button = GTK_WIDGET (gtk_builder_get_object (builder, "mw_clock_speed_spin_button"));
	priv->adc_hbox = GTK_WIDGET (gtk_builder_get_object (builder, "mw_adc_hbox"));
	priv->output_notebook = GTK_WIDGET (gtk_builder_get_object (builder, "mw_output_notebook"));
	priv->input_alignment = GTK_WIDGET (gtk_builder_get_object (builder, "mw_input_alignment"));
	priv->adc_frequency_spin_button = GTK_WIDGET (gtk_builder_get_object (builder, "mw_adc_frequency_spin_button"));
	priv->adc_amplitude_spin_button = GTK_WIDGET (gtk_builder_get_object (builder, "mw_adc_amplitude_spin_button"));
	priv->adc_phase_spin_button = GTK_WIDGET (gtk_builder_get_object (builder, "mw_adc_phase_spin_button"));

	/* Menus */
	priv->file_action_group = GTK_ACTION_GROUP (gtk_builder_get_object (builder, "mw_file_action_group"));
	priv->save_action = GTK_ACTION (gtk_builder_get_object (builder, "mcus_save_action"));
	priv->save_as_action = GTK_ACTION (gtk_builder_get_object (builder, "mcus_save_as_action"));
	priv->undo_action = GTK_ACTION (gtk_builder_get_object (builder, "mcus_undo_action"));
	priv->redo_action = GTK_ACTION (gtk_builder_get_object (builder, "mcus_redo_action"));
	priv->cut_action = GTK_ACTION (gtk_builder_get_object (builder, "mcus_cut_action"));
	priv->copy_action = GTK_ACTION (gtk_builder_get_object (builder, "mcus_copy_action"));
	priv->paste_action = GTK_ACTION (gtk_builder_get_object (builder, "mcus_paste_action"));
	priv->delete_action = GTK_ACTION (gtk_builder_get_object (builder, "mcus_delete_action"));
	priv->run_action = GTK_ACTION (gtk_builder_get_object (builder, "mcus_run_action"));
	priv->pause_action = GTK_ACTION (gtk_builder_get_object (builder, "mcus_pause_action"));
	priv->stop_action = GTK_ACTION (gtk_builder_get_object (builder, "mcus_stop_action"));
	priv->step_forward_action = GTK_ACTION (gtk_builder_get_object (builder, "mcus_step_forward_action"));

	/* Grab the ADC controls */
	priv->adc_frequency_adjustment = GTK_ADJUSTMENT (gtk_builder_get_object (builder, "mw_adc_frequency_adjustment"));
	priv->adc_amplitude_adjustment = GTK_ADJUSTMENT (gtk_builder_get_object (builder, "mw_adc_amplitude_adjustment"));
	priv->adc_offset_adjustment = GTK_ADJUSTMENT (gtk_builder_get_object (builder, "mw_adc_offset_adjustment"));
	priv->adc_phase_adjustment = GTK_ADJUSTMENT (gtk_builder_get_object (builder, "mw_adc_phase_adjustment"));

	priv->adc_constant_option = GTK_TOGGLE_BUTTON (gtk_builder_get_object (builder, "mw_adc_constant_option"));
	priv->adc_sine_wave_option = GTK_TOGGLE_BUTTON (gtk_builder_get_object (builder, "mw_adc_sine_wave_option"));
	priv->adc_square_wave_option = GTK_TOGGLE_BUTTON (gtk_builder_get_object (builder, "mw_adc_square_wave_option"));
	priv->adc_triangle_wave_option = GTK_TOGGLE_BUTTON (gtk_builder_get_object (builder, "mw_adc_triangle_wave_option"));

	/* TODO: define? */
	/* Grab the input check buttons */
	for (i = 0; i < 8; i++) {
		gchar button_id[24];

		/* Grab the control */
		g_sprintf (button_id, "mw_input_check_button_%u", i);
		priv->input_check_button[i] = GTK_TOGGLE_BUTTON (gtk_builder_get_object (builder, button_id));
	}

	/* TODO: Define? */
	/* Grab the dual SSD outputs */
	priv->output_dual_ssd[0] = MCUS_SEVEN_SEGMENT_DISPLAY (gtk_builder_get_object (builder, "mw_output_dual_ssd0"));
	priv->output_dual_ssd[1] = MCUS_SEVEN_SEGMENT_DISPLAY (gtk_builder_get_object (builder, "mw_output_dual_ssd1"));

	/* TODO: Define? */
	/* Grab the multi SSD outputs */
	for (i = 0; i < 16; i++) {
		gchar ssd_id[21];

		/* Grab the control */
		g_sprintf (ssd_id, "mw_output_multi_ssd%u", i);
		priv->output_multi_ssd[i] = MCUS_SEVEN_SEGMENT_DISPLAY (gtk_builder_get_object (builder, ssd_id));
	}

	/* TODO: Define? */
	/* Grab the LED outputs */
	for (i = 0; i < 8; i++) {
		gchar led_id[16];

		g_sprintf (led_id, "mw_output_led_%u", i);
		priv->output_led[i] = MCUS_LED (gtk_builder_get_object (builder, led_id));
	}

	/* Grab the input port */
	priv->input_port_entry = GTK_ENTRY (gtk_builder_get_object (builder, "mw_input_port_entry"));

	/* Grab the single SSD output widgets */
	priv->output_single_ssd = MCUS_SEVEN_SEGMENT_DISPLAY (gtk_builder_get_object (builder, "mw_output_single_ssd"));
	priv->output_single_ssd_segment_option = GTK_TOGGLE_BUTTON (gtk_builder_get_object (builder, "mw_output_single_ssd_segment_option"));

	/* Set up the simulation state */
	g_signal_connect (priv->simulation, "iteration-started", (GCallback) simulation_iteration_started_cb, main_window);
	g_signal_connect (priv->simulation, "iteration-finished", (GCallback) simulation_iteration_finished_cb, main_window);
	g_signal_connect (priv->simulation, "notify::state", (GCallback) notify_simulation_state_cb, main_window);

	/* Call notify_simulation_state_cb() initialise the interface */
	notify_simulation_state_cb (G_OBJECT (priv->simulation), NULL, main_window);

	/* Create the highlighting tags */
	text_buffer = GTK_TEXT_BUFFER (gtk_builder_get_object (builder, "mw_code_buffer"));
	priv->current_instruction_tag = gtk_text_buffer_create_tag (text_buffer, "current-instruction",
	                                                            "weight", PANGO_WEIGHT_BOLD,
	                                                            NULL);
	priv->error_tag = gtk_text_buffer_create_tag (text_buffer, "error",
	                                              "background", "pink",
	                                              NULL);

	/* Connect so we can update the undo/redo/cut/copy/delete actions */
	g_signal_connect (text_buffer, "notify::can-undo", (GCallback) notify_can_undo_cb, main_window);
	g_signal_connect (text_buffer, "notify::can-redo", (GCallback) notify_can_redo_cb, main_window);
	g_signal_connect (text_buffer, "notify::has-selection", (GCallback) notify_has_selection_cb, main_window);

	/* Watch for changes in the simulation */
	g_signal_connect (priv->simulation, "notify::program-counter", (GCallback) notify_program_counter_cb, main_window);
	g_signal_connect (priv->simulation, "notify::zero-flag", (GCallback) notify_zero_flag_cb, main_window);
	g_signal_connect (priv->simulation, "notify::output-port", (GCallback) notify_output_port_cb, main_window);

	/* Watch for modification of the code buffer */
	g_signal_connect (text_buffer, "modified-changed", (GCallback) buffer_modified_changed_cb, main_window);
	gtk_text_buffer_set_modified (text_buffer, FALSE);

	/* Set up the syntax highlighting */
	priv->language_manager = gtk_source_language_manager_new ();

	/* Sort out the search paths so that our own are in there */
	old_language_dirs = gtk_source_language_manager_get_search_path (priv->language_manager);

	while (old_language_dirs[i] != NULL)
		i++;

	language_dirs = g_malloc0 ((i + 3) * sizeof (gchar*));
	language_dirs[0] = mcus_get_data_directory ();
	language_dirs[1] = "data/";

	i = 0;
	while (old_language_dirs[i] != NULL) {
		language_dirs[i + 2] = old_language_dirs[i];
		i++;
	}
	language_dirs[i + 2] = NULL;

	/* Debug */
	g_debug ("Current language manager search path:");
	for (i = 0; language_dirs[i] != NULL; i++)
		g_debug ("\t%s", language_dirs[i]);

	gtk_source_language_manager_set_search_path (priv->language_manager, (gchar**) language_dirs);
	g_free (language_dirs);

	/* Debug */
	language_ids = gtk_source_language_manager_get_language_ids (priv->language_manager);

	g_debug ("Languages installed:");
	for (i = 0; language_ids[i] != NULL; i++)
		g_debug ("\t%s", language_ids[i]);

	language = gtk_source_language_manager_get_language (priv->language_manager, "ocr-assembly");
	if (language == NULL)
		g_warning ("Could not load syntax highlighting file.");

	gtk_source_buffer_set_language (GTK_SOURCE_BUFFER (text_buffer), language);
	if (gtk_source_buffer_get_style_scheme (GTK_SOURCE_BUFFER (text_buffer)) == NULL)
		g_debug ("NULL style scheme");

	g_object_unref (builder);

	return GTK_WINDOW (main_window);
}

/* Returns TRUE if changes were saved, or FALSE if the operation was cancelled */
static gboolean
save_changes (MCUSMainWindow *self)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new (GTK_WINDOW (self), GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
	                                 _("Save changes to the program before closing?"));
	gtk_dialog_add_buttons (GTK_DIALOG (dialog),
				_("Close without Saving"), GTK_RESPONSE_CLOSE,
				"gtk-cancel", GTK_RESPONSE_CANCEL,
				(self->priv->current_filename == NULL) ? "gtk-save-as" : "gtk-save", GTK_RESPONSE_OK,
				NULL);
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), _("If you don't save, your changes will be permanently lost."));

	switch (gtk_dialog_run (GTK_DIALOG (dialog))) {
	case GTK_RESPONSE_CLOSE:
		gtk_widget_destroy (dialog);
		return TRUE;
	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (dialog);
		return FALSE;
	default:
		gtk_widget_destroy (dialog);
		mcus_main_window_save_program (self);
		return TRUE;
	}
}

void
mcus_main_window_new_program (MCUSMainWindow *self)
{
	GtkTextBuffer *text_buffer = self->priv->code_buffer;

	/* Ask to save old files */
	if (gtk_text_buffer_get_modified (text_buffer) == FALSE ||
	    save_changes (self) == TRUE) {
		gtk_text_buffer_set_text (text_buffer, "", -1);
		gtk_text_buffer_set_modified (text_buffer, FALSE);
	}
}

void
mcus_main_window_open_program (MCUSMainWindow *self)
{
	GtkWidget *dialog;
	GtkTextBuffer *text_buffer = self->priv->code_buffer;

	/* Ask to save old files */
	if (gtk_text_buffer_get_modified (text_buffer) == FALSE ||
	    save_changes (self) == TRUE) {
		/* Get a filename to open */
		dialog = gtk_file_chooser_dialog_new (_("Open File"), GTK_WINDOW (self), GTK_FILE_CHOOSER_ACTION_OPEN,
		                                      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		                                      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		                                      NULL);
		gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog), self->priv->filter);

		if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
			gchar *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
			mcus_main_window_open_file (self, filename);
		}
		gtk_widget_destroy (dialog);
	}
}

void
mcus_main_window_save_program (MCUSMainWindow *self)
{
	GtkTextIter start_iter, end_iter;
	GtkTextBuffer *text_buffer;
	GIOChannel *channel;
	GtkWidget *dialog;
	gchar *file_contents = NULL;
	GError *error = NULL;

	if (self->priv->current_filename == NULL)
		return mcus_main_window_save_program_as (self);

	channel = g_io_channel_new_file (self->priv->current_filename, "w", &error);
	if (error != NULL)
		goto file_error;

	text_buffer = self->priv->code_buffer;

	gtk_text_buffer_get_bounds (text_buffer, &start_iter, &end_iter);
	file_contents = gtk_text_buffer_get_text (text_buffer, &start_iter, &end_iter, FALSE);

	g_io_channel_write_chars (channel, file_contents, -1, NULL, &error);
	if (error != NULL)
		goto file_error;

	gtk_text_buffer_set_modified (text_buffer, FALSE);
	g_free (file_contents);

	g_io_channel_shutdown (channel, TRUE, NULL);
	g_io_channel_unref (channel);

	return;

file_error:
	dialog = gtk_message_dialog_new (GTK_WINDOW (self),
	                                 GTK_DIALOG_MODAL,
	                                 GTK_MESSAGE_ERROR,
	                                 GTK_BUTTONS_OK,
	                                 _("Program could not be saved to \"%s\""), self->priv->current_filename);
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", error->message);
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);

	g_error_free (error);
	g_io_channel_unref (channel);
	if (file_contents != NULL)
		g_free (file_contents);
}

void
mcus_main_window_save_program_as (MCUSMainWindow *self)
{
	GtkWidget *dialog;

	dialog = gtk_file_chooser_dialog_new (_("Save File"), GTK_WINDOW (self), GTK_FILE_CHOOSER_ACTION_SAVE,
	                                      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	                                      GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
	                                      NULL);
	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
	gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog), self->priv->filter);

	if (self->priv->current_filename != NULL)
		gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog), self->priv->current_filename);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		g_free (self->priv->current_filename);
		self->priv->current_filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

		/* Ensure it ends with ".asm" */
		if (g_str_has_suffix (self->priv->current_filename, ".asm") == FALSE) {
			gchar *temp_string = g_strconcat (self->priv->current_filename, ".asm", NULL);
			g_free (self->priv->current_filename);
			self->priv->current_filename = temp_string;
		}

		mcus_main_window_save_program (self);
	}
	gtk_widget_destroy (dialog);
}

/* Takes ownership of filename */
void
mcus_main_window_open_file (MCUSMainWindow *self, const gchar *filename)
{
	GIOChannel *channel;
	GtkWidget *dialog;
	gchar *file_contents = NULL;
	GError *error = NULL;
	GtkTextBuffer *text_buffer = self->priv->code_buffer;

	channel = g_io_channel_new_file (filename, "r", &error);
	if (error != NULL)
		goto file_error;

	g_io_channel_read_to_end (channel, &file_contents, NULL, &error);
	if (error != NULL)
		goto file_error;

	gtk_text_buffer_set_text (text_buffer, file_contents, -1);
	gtk_text_buffer_set_modified (text_buffer, FALSE);
	g_free (file_contents);

	g_io_channel_shutdown (channel, FALSE, NULL);
	g_io_channel_unref (channel);

	g_free (self->priv->current_filename);
	self->priv->current_filename = g_strdup (filename);

	return;

file_error:
	dialog = gtk_message_dialog_new (GTK_WINDOW (self),
	                                 GTK_DIALOG_MODAL,
	                                 GTK_MESSAGE_ERROR,
	                                 GTK_BUTTONS_OK,
	                                 _("Program could not be opened from \"%s\""), filename);
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", error->message);
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);

	g_error_free (error);
	g_io_channel_unref (channel);
	g_free (file_contents);
}

gboolean
mcus_main_window_quit (MCUSMainWindow *self)
{
	/* Ask to save old files */
	if (gtk_text_buffer_get_modified (self->priv->code_buffer) == TRUE &&
	    save_changes (self) == FALSE) {
		return FALSE;
	}

	return TRUE;
}

static void
remove_tag (MCUSMainWindow *self, GtkTextTag *tag)
{
	GtkTextIter start_iter, end_iter;
	GtkTextBuffer *text_buffer = self->priv->code_buffer;

	gtk_text_buffer_get_bounds (text_buffer, &start_iter, &end_iter);
	gtk_text_buffer_remove_tag (text_buffer, tag, &start_iter, &end_iter);
}

static void
tag_range (MCUSMainWindow *self, GtkTextTag *tag, guint start_offset, guint end_offset, gboolean remove_previous_occurrences)
{
	GtkTextIter start_iter, end_iter;
	GtkTextBuffer *text_buffer = self->priv->code_buffer;

	/* Remove previous occurrences */
	if (remove_previous_occurrences == TRUE) {
		gtk_text_buffer_get_bounds (text_buffer, &start_iter, &end_iter);
		gtk_text_buffer_remove_tag (text_buffer, tag, &start_iter, &end_iter);
	}

	/* Apply the new tag */
	gtk_text_buffer_get_iter_at_offset (text_buffer, &start_iter, start_offset);
	gtk_text_buffer_get_iter_at_offset (text_buffer, &end_iter, end_offset);

	gtk_text_buffer_apply_tag (text_buffer, tag, &start_iter, &end_iter);
}

static gboolean
input_port_read_entry (MCUSMainWindow *self, GError **error)
{
	gint digit_value;
	guchar input_port;
	const gchar *entry_text;

	entry_text = gtk_entry_get_text (self->priv->input_port_entry);
	if (strlen (entry_text) != 2) {
		g_set_error (error, MCUS_IO_ERROR, MCUS_IO_ERROR_INPUT,
			     _("The input port value was not two digits long."));
		return FALSE;
	}

	/* Deal with the first digit */
	digit_value = g_ascii_xdigit_value (entry_text[0]);
	if (digit_value == -1) {
		g_set_error (error, MCUS_IO_ERROR, MCUS_IO_ERROR_INPUT,
			     _("The input port contained a non-hexadecimal digit (\"%c\")."),
			     entry_text[0]);
		return FALSE;
	}
	input_port = digit_value * 16;

	/* Deal with the second digit */
	digit_value = g_ascii_xdigit_value (entry_text[1]);
	if (digit_value == -1) {
		g_set_error (error, MCUS_IO_ERROR, MCUS_IO_ERROR_INPUT,
			     _("The input port contained a non-hexadecimal digit (\"%c\")."),
			     entry_text[1]);
		return FALSE;
	}
	input_port += digit_value;

	mcus_simulation_set_input_port (self->priv->simulation, input_port);

	return TRUE;
}

static void
input_port_update_entry (MCUSMainWindow *self)
{
	gchar *output = g_strdup_printf ("%02X", mcus_simulation_get_input_port (self->priv->simulation));
	gtk_entry_set_text (self->priv->input_port_entry, output);
	g_free (output);
}

static void
input_port_read_check_buttons (MCUSMainWindow *self)
{
	guint i;
	guchar input_port = 0;

	/* 0 is LSB, 7 is MSB */
	for (i = 0; i < 8; i++) {
		/* Grab the control (have to take the inverse of i, since 7 is the MSB) */
		/* Shift the new bit in as the LSB */
		input_port = gtk_toggle_button_get_active (self->priv->input_check_button[7 - i]) | (input_port << 1);
	}

	mcus_simulation_set_input_port (self->priv->simulation, input_port);
}

static void
input_port_update_check_buttons (MCUSMainWindow *self)
{
	guint i;
	guchar input_port = mcus_simulation_get_input_port (self->priv->simulation);

	/* 0 is LSB, 7 is MSB */
	for (i = 0; i < 8; i++) {
		/* Mask out everything except the interesting bit */
		gtk_toggle_button_set_active (self->priv->input_check_button[i], input_port & (1 << i));
	}
}

static void
update_single_ssd_output (MCUSMainWindow *self)
{
	if (gtk_toggle_button_get_active (self->priv->output_single_ssd_segment_option) == TRUE) {
		/* Each bit in the output corresponds to one segment */
		mcus_seven_segment_display_set_segment_mask (self->priv->output_single_ssd, mcus_simulation_get_output_port (self->priv->simulation));
	} else {
		/* The output is BCD-encoded, and we should display that number */
		guint digit = mcus_simulation_get_output_port (self->priv->simulation) & 0x0F;

		if (digit > 9)
			digit = 0;

		mcus_seven_segment_display_set_digit (self->priv->output_single_ssd, digit);
	}
}

static void
update_multi_ssd_output (MCUSMainWindow *self)
{
	guint i;
	guchar output_port = mcus_simulation_get_output_port (self->priv->simulation);

	for (i = 0; i < 16; i++) {
		/* Work out which SSD we're setting */
		if (i == output_port >> 4) {
			guint digit;

			/* Get the new value */
			digit = output_port & 0x0F;
			if (digit > 9)
				digit = 0;

			mcus_seven_segment_display_set_digit (self->priv->output_multi_ssd[i], digit);
		} else {
			/* Blank the display */
			mcus_seven_segment_display_set_segment_mask (self->priv->output_multi_ssd[i], 0);
		}
	}
}

static void
update_outputs (MCUSMainWindow *self)
{
	guint i;
	guchar output_port = mcus_simulation_get_output_port (self->priv->simulation);

	/* Only update outputs if they're visible */
	switch (self->priv->output_device) {
	case OUTPUT_LED_DEVICE:
		/* Update the LED outputs */
		for (i = 0; i < 8; i++)
			mcus_led_set_enabled (self->priv->output_led[i], output_port & (1 << i));
		break;
	case OUTPUT_SINGLE_SSD_DEVICE:
		/* Update the single SSD output */
		update_single_ssd_output (self);
		break;
	case OUTPUT_DUAL_SSD_DEVICE:
		/* Update the dual-SSD output */
		i = output_port >> 4;
		if (i > 9)
			i = 0;
		mcus_seven_segment_display_set_digit (self->priv->output_dual_ssd[1], i);

		i = output_port & 0x0F;
		if (i > 9)
			i = 0;
		mcus_seven_segment_display_set_digit (self->priv->output_dual_ssd[0], i);
		break;
	case OUTPUT_MULTIPLEXED_SSD_DEVICE:
		/* Update the multi-SSD output */
		update_multi_ssd_output (self);
		break;
	default:
		g_assert_not_reached ();
	}
}

static void
update_simulation_ui (MCUSMainWindow *self)
{
	guint i;
	/* 3 characters for each memory byte (two hexadecimal digits plus either a space, newline or \0)
	 * plus 7 characters for <b></b> around the byte pointed to by the program counter. */
	gchar memory_markup[3 * MEMORY_SIZE + 7];
	/* 3 characters for each register as above */
	gchar register_text[3 * REGISTER_COUNT];
	/* 3 characters for each stack byte as above */
	gchar stack_text[3 * STACK_PREVIEW_SIZE];
	/* 3 characters for one byte as above */
	gchar byte_text[3];
	gchar *f = memory_markup;
	MCUSStackFrame *stack_frame, *stack;
	guchar *memory, *registers, program_counter;

	memory = mcus_simulation_get_memory (self->priv->simulation);
	registers = mcus_simulation_get_registers (self->priv->simulation);
	stack = mcus_simulation_get_stack_head (self->priv->simulation);
	program_counter = mcus_simulation_get_program_counter (self->priv->simulation);

	/* Update the memory label */
	for (i = 0; i < MEMORY_SIZE; i++) {
		g_sprintf (f, G_UNLIKELY (i == program_counter) ? "<b>%02X</b> " : "%02X ", memory[i]);
		f += 3;

		if (G_UNLIKELY (i == program_counter))
			f += 7;
		if (G_UNLIKELY (i % 16 == 15))
			*(f - 1) = '\n';
	}
	*(f - 1) = '\0';

	gtk_label_set_markup (self->priv->memory_label, memory_markup);

	/* Update the register label */
	f = register_text;
	for (i = 0; i < REGISTER_COUNT; i++) {
		g_sprintf (f, "%02X ", registers[i]);
		f += 3;
	}
	*(f - 1) = '\0';

	gtk_label_set_text (self->priv->registers_label, register_text);

	/* Update the stack label */
	f = stack_text;
	i = 0;
	stack_frame = stack;
	while (stack_frame != NULL && i < STACK_PREVIEW_SIZE) {
		g_sprintf (f, "%02X ", stack_frame->program_counter);
		f += 3;

		if (G_UNLIKELY (i % 16 == 15))
			*(f - 1) = '\n';

		i++;
		stack_frame = stack_frame->prev;
	}
	if (i == 0)
		g_sprintf (f, "(Empty)");
	*(f - 1) = '\0';

	gtk_label_set_text (self->priv->stack_label, stack_text);

	/* Update the stack pointer label */
	g_sprintf (byte_text, "%02X", (stack == NULL) ? 0 : stack->program_counter);
	gtk_label_set_text (self->priv->stack_pointer_label, byte_text);

	update_outputs (self);

	/* Move the current line mark */
	if (self->priv->offset_map != NULL) {
		tag_range (self, self->priv->current_instruction_tag,
		           self->priv->offset_map[program_counter].offset,
		           self->priv->offset_map[program_counter].offset + self->priv->offset_map[program_counter].length,
		           TRUE);
	}

	mcus_simulation_print_debug_data (self->priv->simulation);
}

static void
read_analogue_input (MCUSMainWindow *self)
{
	MCUSMainWindowPrivate *priv = self->priv;
	gdouble amplitude, frequency, phase, offset, analogue_input;
	guint iteration;
	gulong clock_speed;

	iteration = mcus_simulation_get_iteration (priv->simulation);
	clock_speed = mcus_simulation_get_clock_speed (priv->simulation);

	/* Update the analogue input from the function generator */
	amplitude = gtk_adjustment_get_value (priv->adc_amplitude_adjustment);
	frequency = gtk_adjustment_get_value (priv->adc_frequency_adjustment);
	phase = gtk_adjustment_get_value (priv->adc_phase_adjustment);
	offset = gtk_adjustment_get_value (priv->adc_offset_adjustment);

	if (gtk_toggle_button_get_active (priv->adc_constant_option) == TRUE) {
		/* Constant signal */
		analogue_input = offset;
	} else if (gtk_toggle_button_get_active (priv->adc_sine_wave_option) == TRUE) {
		/* Sine wave */
		analogue_input = amplitude * sin (2.0 * M_PI * frequency * ((gdouble)(iteration) / clock_speed) + phase) + offset;
	} else if (gtk_toggle_button_get_active (priv->adc_square_wave_option) == TRUE) {
		/* Square wave */
		gdouble sine = sin (2.0 * M_PI * frequency * ((gdouble)(iteration) / clock_speed) + phase);
		analogue_input = (sine > 0) ? 1.0 : (sine == 0) ? 0.0 : -1.0;
		analogue_input = amplitude * analogue_input + offset;
	} else if (gtk_toggle_button_get_active (priv->adc_triangle_wave_option) == TRUE) {
		/* Triangle wave */
		analogue_input = amplitude * asin (sin (2.0 * M_PI * frequency *
		                                        ((gdouble)(iteration) / clock_speed) + phase)) + offset;
	} else {
		/* Sawtooth wave */
		gdouble t = ((gdouble)(iteration) / clock_speed) * frequency;
		analogue_input = amplitude * 2.0 * (t - floor (t + 0.5)) + offset;
	}

	/* Clamp the value to 0--5V and set it */
	mcus_simulation_set_analogue_input (priv->simulation, CLAMP (analogue_input, 0.0, 5.0));

	g_debug ("Analogue input: %f", analogue_input);
}

static void
simulation_iteration_started_cb (MCUSSimulation *self, MCUSMainWindow *main_window)
{
	read_analogue_input (main_window);
}

static void
simulation_iteration_finished_cb (MCUSSimulation *self, GError *error, MCUSMainWindow *main_window)
{
	if (error != NULL) {
		GtkWidget *dialog;
		guchar program_counter;

		program_counter = mcus_simulation_get_program_counter (main_window->priv->simulation);

		/* Highlight the offending line */
		tag_range (main_window, main_window->priv->error_tag,
		           main_window->priv->offset_map[program_counter].offset,
		           main_window->priv->offset_map[program_counter].offset + main_window->priv->offset_map[program_counter].length,
		           FALSE);

		/* Display an error message */
		dialog = gtk_message_dialog_new (GTK_WINDOW (main_window),
		                                 GTK_DIALOG_MODAL,
		                                 GTK_MESSAGE_ERROR,
		                                 GTK_BUTTONS_OK,
		                                 _("Error iterating simulation"));
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", error->message);
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		g_error_free (error);
	} else {
		update_simulation_ui (main_window);
	}
}

static void
notify_simulation_state_cb (GObject *object, GParamSpec *param_spec, MCUSMainWindow *main_window)
{
	GtkSourceBuffer *source_buffer;
	gboolean stopped, not_running, has_selection;
	MCUSMainWindowPrivate *priv = main_window->priv;
	MCUSSimulationState state = mcus_simulation_get_state (priv->simulation);

	stopped = (state == MCUS_SIMULATION_STOPPED);
	not_running = (state != MCUS_SIMULATION_RUNNING);
	has_selection = gtk_text_buffer_get_has_selection (main_window->priv->code_buffer);
	source_buffer = GTK_SOURCE_BUFFER (main_window->priv->code_buffer);

#define SET_SENSITIVITY_A(W,S) \
	gtk_action_set_sensitive (GTK_ACTION (priv->W), (S))
#define SET_SENSITIVITY_W(W,S) \
	gtk_widget_set_sensitive (GTK_WIDGET (priv->W), (S))

	/* Update the UI */
	SET_SENSITIVITY_W (code_view, stopped);
	SET_SENSITIVITY_W (input_port_entry, not_running);
	SET_SENSITIVITY_W (adc_spin_button, not_running);
	SET_SENSITIVITY_W (clock_speed_spin_button, not_running);
	SET_SENSITIVITY_W (adc_hbox, not_running);
	SET_SENSITIVITY_W (output_notebook, not_running);
	SET_SENSITIVITY_W (input_alignment, not_running);

	gtk_action_group_set_sensitive (priv->file_action_group, stopped);

	SET_SENSITIVITY_A (undo_action, stopped && gtk_source_buffer_can_undo (source_buffer));
	SET_SENSITIVITY_A (redo_action, stopped && gtk_source_buffer_can_redo (source_buffer));
	SET_SENSITIVITY_A (cut_action, not_running && has_selection);
	SET_SENSITIVITY_A (copy_action, not_running && has_selection);
	SET_SENSITIVITY_A (paste_action, stopped);
	SET_SENSITIVITY_A (delete_action, not_running && has_selection);

	SET_SENSITIVITY_A (run_action, not_running);
	SET_SENSITIVITY_A (pause_action, state == MCUS_SIMULATION_RUNNING);
	SET_SENSITIVITY_A (stop_action, state != MCUS_SIMULATION_STOPPED);
	SET_SENSITIVITY_A (step_forward_action, state == MCUS_SIMULATION_PAUSED);

#undef SET_SENSITIVITY_A
#undef SET_SENSITIVITY_W

	if (stopped) {
		/* If we're finished, remove the current instruction tag */
		remove_tag (main_window, main_window->priv->current_instruction_tag);
	}
}

static void
notify_can_undo_cb (GObject *object, GParamSpec *param_spec, MCUSMainWindow *main_window)
{
	gtk_action_set_sensitive (main_window->priv->undo_action,
	                          mcus_simulation_get_state (main_window->priv->simulation) == MCUS_SIMULATION_STOPPED &&
	                          gtk_source_buffer_can_undo (GTK_SOURCE_BUFFER (object)));
}

static void
notify_can_redo_cb (GObject *object, GParamSpec *param_spec, MCUSMainWindow *main_window)
{
	gtk_action_set_sensitive (main_window->priv->redo_action,
	                          mcus_simulation_get_state (main_window->priv->simulation) == MCUS_SIMULATION_STOPPED &&
	                          gtk_source_buffer_can_undo (GTK_SOURCE_BUFFER (object)));
}

static void
notify_has_selection_cb (GObject *object, GParamSpec *param_spec, MCUSMainWindow *main_window)
{
	gboolean sensitive = mcus_simulation_get_state (main_window->priv->simulation) != MCUS_SIMULATION_RUNNING &&
	                     gtk_text_buffer_get_has_selection (GTK_TEXT_BUFFER (object));

	gtk_action_set_sensitive (main_window->priv->cut_action, sensitive);
	gtk_action_set_sensitive (main_window->priv->copy_action, sensitive);
	gtk_action_set_sensitive (main_window->priv->delete_action, sensitive);
}

static void
notify_program_counter_cb (GObject *object, GParamSpec *param_spec, MCUSMainWindow *main_window)
{
	/* 3 characters for two hexadecimal characters and one \0 */
	gchar byte_text[3];
	guchar program_counter = mcus_simulation_get_program_counter (MCUS_SIMULATION (object));

	/* Update the program counter label */
	g_sprintf (byte_text, "%02X", program_counter);
	gtk_label_set_text (main_window->priv->program_counter_label, byte_text);
}

static void
notify_zero_flag_cb (GObject *object, GParamSpec *param_spec, MCUSMainWindow *main_window)
{
	/* TODO */
}

static void
notify_output_port_cb (GObject *object, GParamSpec *param_spec, MCUSMainWindow *main_window)
{
	/* 3 characters for two hexadecimal characters and one \0 */
	gchar byte_text[3];
	guchar output_port = mcus_simulation_get_output_port (MCUS_SIMULATION (object));

	/* Update the output port label */
	g_sprintf (byte_text, "%02X", output_port);
	gtk_label_set_text (main_window->priv->output_port_label, byte_text);
}

static void
buffer_modified_changed_cb (GtkTextBuffer *self, MCUSMainWindow *main_window)
{
	gtk_action_set_sensitive (main_window->priv->save_action, gtk_text_buffer_get_modified (self));
	gtk_action_set_sensitive (main_window->priv->save_as_action, TRUE);
}

G_MODULE_EXPORT gboolean
mw_delete_event_cb (GtkWidget *widget, GdkEvent *event, MCUSMainWindow *main_window)
{
	mcus_quit (main_window);
	return TRUE;
}

G_MODULE_EXPORT gboolean
mw_key_press_event_cb (GtkWidget *widget, GdkEventKey *event, MCUSMainWindow *main_window)
{
	guint shift;

	/* The Ctrl key should be pressed */
	if (!(event->state & GDK_CONTROL_MASK))
		return FALSE;

	switch (event->keyval) {
	case GDK_1:
		shift = 0;
		break;
	case GDK_2:
		shift = 1;
		break;
	case GDK_3:
		shift = 2;
		break;
	case GDK_4:
		shift = 3;
		break;
	case GDK_5:
		shift = 4;
		break;
	case GDK_6:
		shift = 5;
		break;
	case GDK_7:
		shift = 6;
		break;
	case GDK_8:
		shift = 7;
		break;
	default:
		/* We don't want to handle this key */
		return FALSE;
	}

	/* Toggle the relevant bit in the input port */
	mcus_simulation_set_input_port (main_window->priv->simulation, mcus_simulation_get_input_port (main_window->priv->simulation) ^ (1 << shift));

	/* Update the UI (updating the check buttons updates the entry too) */
	input_port_update_check_buttons (main_window);

	return TRUE;
}

G_MODULE_EXPORT void
mw_quit_activate_cb (GtkAction *self, MCUSMainWindow *main_window)
{
	mcus_quit (main_window);
}

G_MODULE_EXPORT void
mw_undo_activate_cb (GtkAction *self, MCUSMainWindow *main_window)
{
	gtk_source_buffer_undo (GTK_SOURCE_BUFFER (main_window->priv->code_buffer));
}

G_MODULE_EXPORT void
mw_redo_activate_cb (GtkAction *self, MCUSMainWindow *main_window)
{
	gtk_source_buffer_redo (GTK_SOURCE_BUFFER (main_window->priv->code_buffer));
}

G_MODULE_EXPORT void
mw_cut_activate_cb (GtkAction *self, MCUSMainWindow *main_window)
{
	GtkClipboard *clipboard = gtk_clipboard_get_for_display (gtk_widget_get_display (GTK_WIDGET (main_window)), GDK_SELECTION_CLIPBOARD);
	gtk_text_buffer_cut_clipboard (main_window->priv->code_buffer, clipboard, TRUE);
}

G_MODULE_EXPORT void
mw_copy_activate_cb (GtkAction *self, MCUSMainWindow *main_window)
{
	GtkClipboard *clipboard = gtk_clipboard_get_for_display (gtk_widget_get_display (GTK_WIDGET (main_window)), GDK_SELECTION_CLIPBOARD);
	gtk_text_buffer_copy_clipboard (main_window->priv->code_buffer, clipboard);
}

G_MODULE_EXPORT void
mw_paste_activate_cb (GtkAction *self, MCUSMainWindow *main_window)
{
	GtkClipboard *clipboard = gtk_clipboard_get_for_display (gtk_widget_get_display (GTK_WIDGET (main_window)), GDK_SELECTION_CLIPBOARD);
	gtk_text_buffer_paste_clipboard (main_window->priv->code_buffer, clipboard, NULL, TRUE);
}

G_MODULE_EXPORT void
mw_delete_activate_cb (GtkAction *self, MCUSMainWindow *main_window)
{
	gtk_text_buffer_delete_selection (main_window->priv->code_buffer, TRUE, TRUE);
}

G_MODULE_EXPORT void
mw_run_activate_cb (GtkAction *self, MCUSMainWindow *main_window)
{
	MCUSCompiler *compiler;
	GtkTextBuffer *code_buffer;
	GtkTextIter start_iter, end_iter;
	gchar *code;
	guint error_start, error_end;
	GtkWidget *dialog;
	GError *error = NULL;

	/* Remove previous errors */
	remove_tag (main_window, main_window->priv->error_tag);

	/* Get the assembly code */
	code_buffer = main_window->priv->code_buffer;
	gtk_text_buffer_get_bounds (code_buffer, &start_iter, &end_iter);
	code = gtk_text_buffer_get_text (code_buffer, &start_iter, &end_iter, FALSE);

	mcus_simulation_print_debug_data (main_window->priv->simulation);

	/* Parse it */
	compiler = mcus_compiler_new ();
	mcus_compiler_parse (compiler, code, &error);
	g_free (code);

	if (error != NULL)
		goto compiler_error;
	mcus_simulation_print_debug_data (main_window->priv->simulation);

	/* Compile it */
	mcus_compiler_compile (compiler, main_window->priv->simulation, &(main_window->priv->offset_map), &error);

	if (error != NULL)
		goto compiler_error;
	g_object_unref (compiler);

	/* Initialise the simulator */
	mcus_simulation_start (main_window->priv->simulation);
	update_simulation_ui (main_window);

	return;

compiler_error:
	/* Highlight the offending line */
	mcus_compiler_get_error_location (compiler, &error_start, &error_end);
	tag_range (main_window, main_window->priv->error_tag, error_start, error_end, FALSE);

	/* Display an error message */
	dialog = gtk_message_dialog_new (GTK_WINDOW (main_window),
	                                 GTK_DIALOG_MODAL,
	                                 GTK_MESSAGE_ERROR,
	                                 GTK_BUTTONS_OK,
	                                 _("Error compiling program"));
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", error->message);
	gtk_dialog_run (GTK_DIALOG (dialog));

	gtk_widget_destroy (dialog);
	g_error_free (error);
	g_object_unref (compiler);
}

G_MODULE_EXPORT void
mw_pause_activate_cb (GtkAction *self, MCUSMainWindow *main_window)
{
	mcus_simulation_pause (main_window->priv->simulation);
}

G_MODULE_EXPORT void
mw_stop_activate_cb (GtkAction *self, MCUSMainWindow *main_window)
{
	mcus_simulation_finish (main_window->priv->simulation);
}

G_MODULE_EXPORT void
mw_step_forward_activate_cb (GtkAction *self, MCUSMainWindow *main_window)
{
	/* Errors are handled in the MCUSSimulation::iteration-finished callback */
	mcus_simulation_iterate (main_window->priv->simulation, NULL);
}

G_MODULE_EXPORT void
mw_clock_speed_spin_button_value_changed_cb (GtkSpinButton *self, MCUSMainWindow *main_window)
{
	mcus_simulation_set_clock_speed (main_window->priv->simulation, gtk_spin_button_get_value (self));
}

G_MODULE_EXPORT void
mw_contents_activate_cb (GtkAction *self, MCUSMainWindow *main_window)
{
	GError *error = NULL;
#ifdef G_OS_WIN32
	GFile *file;
	GAppInfo *app_info;
	GList list;
	gchar *path;

	path = g_build_filename (mcus_get_data_directory (), "help.pdf", NULL);
	file = g_file_new_for_path (path);
	g_free (path);

	list.data = file;
	list.next = list.prev = NULL;

	app_info = g_app_info_get_default_for_type (".pdf", FALSE);

	if (app_info == NULL || g_app_info_launch (app_info, &list, NULL, &error) == FALSE) {
#else /* !G_OS_WIN32 */
	if (gtk_show_uri (gtk_widget_get_screen (GTK_WIDGET (main_window)), "ghelp:mcus", gtk_get_current_event_time (), &error) == FALSE) {
#endif /* !G_OS_WIN32 */
		GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW (main_window),
		                                            GTK_DIALOG_MODAL,
		                                            GTK_MESSAGE_ERROR,
		                                            GTK_BUTTONS_OK,
		                                            _("Error displaying help"));
		if (error == NULL)
			gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), _("Couldn't find a program to open .pdf files."));
		else
			gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", error->message);

		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

		if (error != NULL)
			g_error_free (error);
	}

#ifdef G_OS_WIN32
	g_object_unref (file);
	if (app_info != NULL)
		g_object_unref (app_info);
#endif
}

G_MODULE_EXPORT void
mw_about_activate_cb (GtkAction *self, MCUSMainWindow *main_window)
{
	gchar *license;

	const gchar *authors[] = {
		"Philip Withnall <philip@tecnocode.co.uk>",
		NULL
	};
	const gchar *license_parts[] = {
		N_("MCUS is free software: you can redistribute it and/or modify "
		   "it under the terms of the GNU General Public License as published by "
		   "the Free Software Foundation, either version 3 of the License, or "
		   "(at your option) any later version."),
		N_("MCUS is distributed in the hope that it will be useful, "
		   "but WITHOUT ANY WARRANTY; without even the implied warranty of "
		   "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
		   "GNU General Public License for more details."),
		N_("You should have received a copy of the GNU General Public License "
		   "along with MCUS.  If not, see <http://www.gnu.org/licenses/>."),
	};
	const gchar *description = N_("A program to simulate the microcontroller specified in the 2008 OCR A-level electronics syllabus.");

	license = g_strjoin ("\n\n",
	                     _(license_parts[0]),
	                     _(license_parts[1]),
	                     _(license_parts[2]),
	                     NULL);

	gtk_show_about_dialog (GTK_WINDOW (main_window),
	                       "version", VERSION,
	                       "copyright", _("Copyright \xc2\xa9 2008 Philip Withnall"),
	                       "comments", _(description),
	                       "authors", authors,
	                       /* Translators: please include your names here to be credited for your hard work!
	                        * Format:
	                        * "Translator name 1 <translator@email.address>\n"
	                        * "Translator name 2 <translator2@email.address>"
	                        */
	                       "translator-credits", _("translator-credits"),
	                       "logo-icon-name", "mcus",
	                       "license", license,
	                       "wrap-license", TRUE,
	                       "website-label", _("MCUS Website"),
	                       "website", "http://tecnocode.co.uk/projects/mcus",
	                       NULL);

	g_free (license);
}

G_MODULE_EXPORT void
mw_new_activate_cb (GtkAction *self, MCUSMainWindow *main_window)
{
	mcus_main_window_new_program (main_window);
}

G_MODULE_EXPORT void
mw_open_activate_cb (GtkAction *self, MCUSMainWindow *main_window)
{
	mcus_main_window_open_program (main_window);
}

G_MODULE_EXPORT void
mw_save_activate_cb (GtkAction *self, MCUSMainWindow *main_window)
{
	mcus_main_window_save_program (main_window);
}

G_MODULE_EXPORT void
mw_save_as_activate_cb (GtkAction *self, MCUSMainWindow *main_window)
{
	mcus_main_window_save_program_as (main_window);
}

static gboolean
paginate_cb (GtkPrintOperation *operation, GtkPrintContext *context, GtkSourcePrintCompositor *source_compositor)
{
	if (gtk_source_print_compositor_paginate (source_compositor, context)) {
		gint n_pages;

		n_pages = gtk_source_print_compositor_get_n_pages (source_compositor);
		gtk_print_operation_set_n_pages (operation, n_pages);

		return TRUE;
	}

	return FALSE;
}

static void
draw_page_cb (GtkPrintOperation *operation, GtkPrintContext *context, gint page_number, GtkSourcePrintCompositor *source_compositor)
{
	gtk_source_print_compositor_draw_page (source_compositor, context, page_number);
}

G_MODULE_EXPORT void
mw_print_activate_cb (GtkAction *self, MCUSMainWindow *main_window)
{
	GtkPrintOperation *operation;
	GtkPrintOperationResult res;
	GtkSourcePrintCompositor *source_compositor;
	static GtkPrintSettings *settings;

	/* TODO: Header/Footer? */
	operation = gtk_print_operation_new ();
	source_compositor = gtk_source_print_compositor_new (GTK_SOURCE_BUFFER (main_window->priv->code_buffer));
	gtk_source_print_compositor_set_print_line_numbers (source_compositor, 1);
	gtk_source_print_compositor_set_highlight_syntax (source_compositor, TRUE);

	if (settings != NULL)
		gtk_print_operation_set_print_settings (operation, settings);

	g_signal_connect (operation, "paginate", G_CALLBACK (paginate_cb), source_compositor);
	g_signal_connect (operation, "draw-page", G_CALLBACK (draw_page_cb), source_compositor);

	res = gtk_print_operation_run (operation, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG, GTK_WINDOW (main_window), NULL);

	if (res == GTK_PRINT_OPERATION_RESULT_APPLY) {
		if (settings != NULL)
			g_object_unref (settings);
		settings = g_object_ref (gtk_print_operation_get_print_settings (operation));
	}

	g_object_unref (source_compositor);
	g_object_unref (operation);
}

G_MODULE_EXPORT void
mw_input_check_button_toggled (GtkToggleButton *self, MCUSMainWindow *main_window)
{
	/* Signal fluff prevents race condition between this signal handler and the one below */
	g_signal_handlers_block_by_func (main_window->priv->input_port_entry, mw_input_entry_changed, NULL);

	input_port_read_check_buttons (main_window);
	input_port_update_entry (main_window);

	g_signal_handlers_unblock_by_func (main_window->priv->input_port_entry, mw_input_entry_changed, NULL);
}

static void
set_input_check_button_signal_state (MCUSMainWindow *self, gboolean blocked)
{
	guint i;

	for (i = 0; i < 8; i++) {
		/* Either block or unblock the signal as appropriate */
		if (blocked == TRUE)
			g_signal_handlers_block_by_func (self->priv->input_check_button[i], mw_input_check_button_toggled, NULL);
		else
			g_signal_handlers_unblock_by_func (self->priv->input_check_button[i], mw_input_check_button_toggled, NULL);
	}
}

G_MODULE_EXPORT void
mw_input_entry_changed (GtkEntry *self, MCUSMainWindow *main_window)
{
	set_input_check_button_signal_state (main_window, TRUE);

	/* TODO: Do something on error? */
	if (input_port_read_entry (main_window, NULL))
		input_port_update_check_buttons (main_window);

	set_input_check_button_signal_state (main_window, FALSE);
}

G_MODULE_EXPORT void
mw_output_single_ssd_option_changed_cb (GtkToggleButton *self, MCUSMainWindow *main_window)
{
	update_single_ssd_output (main_window);
}

G_MODULE_EXPORT void
mw_output_notebook_switch_page_cb (GtkNotebook *self, GtkNotebookPage *page, guint page_num, MCUSMainWindow *main_window)
{
	main_window->priv->output_device = page_num;
	update_outputs (main_window);
}

G_MODULE_EXPORT void
mw_adc_constant_option_toggled_cb (GtkToggleButton *self, MCUSMainWindow *main_window)
{
	gboolean non_constant_signal = gtk_toggle_button_get_active (self) == FALSE;

	gtk_widget_set_sensitive (main_window->priv->adc_frequency_spin_button, non_constant_signal);
	gtk_widget_set_sensitive (main_window->priv->adc_amplitude_spin_button, non_constant_signal);
	gtk_widget_set_sensitive (main_window->priv->adc_phase_spin_button, non_constant_signal);
}
