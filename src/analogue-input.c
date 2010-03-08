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
#include <gtk/gtk.h>
#include <math.h>

#include "main.h"
#include "analogue-input.h"

G_MODULE_EXPORT void mw_adc_constant_option_toggled_cb (GtkToggleButton *self, gpointer user_data);

void
mcus_read_analogue_input (void)
{
	gdouble amplitude, frequency, phase, offset;

	/* Update the analogue input from the function generator */
	amplitude = gtk_adjustment_get_value (mcus->analogue_input_amplitude_adjustment);
	frequency = gtk_adjustment_get_value (mcus->analogue_input_frequency_adjustment);
	phase = gtk_adjustment_get_value (mcus->analogue_input_phase_adjustment);
	offset = gtk_adjustment_get_value (mcus->analogue_input_offset_adjustment);

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (mcus->builder, "mw_adc_constant_option"))) == TRUE) {
		/* Constant signal */
		mcus->analogue_input = offset;
	} else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (mcus->builder, "mw_adc_sine_wave_option"))) == TRUE) {
		/* Sine wave */
		mcus->analogue_input = amplitude * sin (2.0 * M_PI * frequency * ((gdouble)(mcus->iteration) * mcus->clock_speed / 1000.0) + phase) + offset;
	} else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (mcus->builder, "mw_adc_square_wave_option"))) == TRUE) {
		/* Square wave */
		gdouble sine = sin (2.0 * M_PI * frequency * ((gdouble)(mcus->iteration) * mcus->clock_speed / 1000.0) + phase);
		mcus->analogue_input = (sine > 0) ? 1.0 : (sine == 0) ? 0.0 : -1.0;
		mcus->analogue_input = amplitude * mcus->analogue_input + offset;
	} else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (mcus->builder, "mw_adc_triangle_wave_option"))) == TRUE) {
		/* Triangle wave */
		mcus->analogue_input = amplitude * asin (sin (2.0 * M_PI * frequency * ((gdouble)(mcus->iteration) * mcus->clock_speed / 1000.0) + phase)) + offset;
	} else {
		/* Sawtooth wave */
		gdouble t = ((gdouble)(mcus->iteration) * mcus->clock_speed / 1000.0) * frequency;
		mcus->analogue_input = amplitude * 2.0 * (t - floor (t + 0.5)) + offset;
	}

	/* Clamp the value to 0--5V */
	if (mcus->analogue_input > 5.0)
		mcus->analogue_input = 5.0;
	else if (mcus->analogue_input < 0.0)
		mcus->analogue_input = 0.0;

	if (mcus->debug == TRUE)
		g_debug ("Analogue input: %f", mcus->analogue_input);
}

G_MODULE_EXPORT void
mw_adc_constant_option_toggled_cb (GtkToggleButton *self, gpointer user_data)
{
	gboolean constant_signal = gtk_toggle_button_get_active (self);

	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (mcus->builder, "mw_adc_frequency_spin_button")), !constant_signal);
	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (mcus->builder, "mw_adc_amplitude_spin_button")), !constant_signal);
	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (mcus->builder, "mw_adc_phase_spin_button")), !constant_signal);
}
