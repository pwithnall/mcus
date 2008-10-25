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
#include <gtk/gtk.h>
#include <math.h>
#include <string.h>

#include "led.h"

#define MINIMUM_SIZE 30

static void mcus_led_init (MCUSLED *self);
static void mcus_led_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void mcus_led_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void mcus_led_realize (GtkWidget *widget);
static void mcus_led_size_request (GtkWidget *widget, GtkRequisition *requisition);
static void mcus_led_size_allocate (GtkWidget *widget, GtkAllocation *allocation);
static gint mcus_led_expose_event (GtkWidget *widget, GdkEventExpose *event);

struct _MCUSLEDPrivate {
	gboolean enabled;
	gdouble render_size;
};

enum {
	PROP_ENABLED = 1
};

G_DEFINE_TYPE (MCUSLED, mcus_led, GTK_TYPE_WIDGET)
#define MCUS_LED_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MCUS_TYPE_LED, MCUSLEDPrivate))

MCUSLED *
mcus_led_new (void)
{
	return g_object_new (MCUS_TYPE_LED, NULL);
}

static void
mcus_led_class_init (MCUSLEDClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	g_type_class_add_private (klass, sizeof (MCUSLEDPrivate));

	gobject_class->set_property = mcus_led_set_property;
	gobject_class->get_property = mcus_led_get_property;

	widget_class->realize = mcus_led_realize;
	widget_class->size_request = mcus_led_size_request;
	widget_class->size_allocate = mcus_led_size_allocate;
	widget_class->expose_event = mcus_led_expose_event;

	g_object_class_install_property (gobject_class, PROP_ENABLED,
				g_param_spec_boolean ("enabled",
					"Enabled", "Whether the LED is enabled.",
					FALSE,
					G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
mcus_led_init (MCUSLED *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, MCUS_TYPE_LED, MCUSLEDPrivate);
}

static void
mcus_led_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	MCUSLEDPrivate *priv = MCUS_LED_GET_PRIVATE (object);

	switch (property_id) {
		case PROP_ENABLED:
			g_value_set_boolean (value, priv->enabled);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
mcus_led_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	MCUSLEDPrivate *priv = MCUS_LED_GET_PRIVATE (object);

	switch (property_id) {
		case PROP_ENABLED:
			priv->enabled = g_value_get_boolean (value);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}

	/* Ensure we're redrawn */
	gtk_widget_queue_draw (GTK_WIDGET (object));
}

static void
mcus_led_realize (GtkWidget *widget)
{
	MCUSLED *self;
	GdkWindowAttr attributes;
	gint attr_mask;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (MCUS_IS_LED (widget));

	/* Mark the widget as realised */
	GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
	self = MCUS_LED (widget);

	/* Create the attribute object */
	attributes.x = widget->allocation.x;
	attributes.y = widget->allocation.y;
	attributes.width = widget->allocation.width;
	attributes.height = widget->allocation.height;
	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.event_mask = GDK_EXPOSURE_MASK | GDK_STRUCTURE_MASK | GDK_PROPERTY_CHANGE_MASK | GDK_VISIBILITY_NOTIFY_MASK;
	attributes.visual = gtk_widget_get_visual (widget);
	attributes.colormap = gtk_widget_get_colormap (widget);

	/* Create a new GdkWindow for the widget */
	attr_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
	widget->window = gdk_window_new (widget->parent->window, &attributes, attr_mask);
	gdk_window_set_user_data (widget->window, self);

	/* Attach a style to the window and draw a background colour */
	widget->style = gtk_style_attach (widget->style, widget->window);
	gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);
	gdk_window_show (widget->window);
}

static void
mcus_led_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
	g_return_if_fail (widget != NULL || requisition != NULL);
	g_return_if_fail (MCUS_IS_LED (widget));

	requisition->width = MINIMUM_SIZE;
	requisition->height = MINIMUM_SIZE;
}

static void
mcus_led_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	MCUSLEDPrivate *priv;

	g_return_if_fail (widget != NULL || allocation != NULL);
	g_return_if_fail (MCUS_IS_LED (widget));

	widget->allocation = *allocation;
	priv = MCUS_LED_GET_PRIVATE (widget);

	/* Sort out sizes, ratios, etc. */
	if (allocation->height < allocation->width)
		priv->render_size = allocation->height;
	else
		priv->render_size = allocation->width;

	/* Ensure the borders aren't clipped */
	priv->render_size -= widget->style->xthickness * 2.0;

	if (GTK_WIDGET_REALIZED (widget))
		gdk_window_move_resize (widget->window, allocation->x, allocation->y, allocation->width, allocation->height);
}

static gint
mcus_led_expose_event (GtkWidget *widget, GdkEventExpose *event)
{
	cairo_t *cr;
	MCUSLEDPrivate *priv;
	GdkColor fill, stroke;

	g_return_val_if_fail (widget != NULL || event != NULL, FALSE);
	g_return_val_if_fail (MCUS_IS_LED (widget), FALSE);

	/* Compress exposes */
	if (event->count > 0)
		return TRUE;

	priv = MCUS_LED_GET_PRIVATE (widget);

	/* Clear the area first */
	gdk_window_clear_area (widget->window, event->area.x, event->area.y, event->area.width, event->area.height);

	/* Prepare our custom colours */
	fill.red = 29555; /* Tango's medium "chameleon" --- 73d216 */
	fill.green = 53970;
	fill.blue = 5654;
	stroke.red = 34952; /* Tango's lightest "aluminium" --- 888a85 */
	stroke.green = 35466;
	stroke.blue = 34181;

	/* Draw! */
	cr = gdk_cairo_create (widget->window);

	/* Clip to the exposed area */
	cairo_rectangle (cr, event->area.x, event->area.y, event->area.width, event->area.height);
	cairo_clip (cr);

	cairo_set_line_width (cr, widget->style->xthickness);

	/* Draw the LED */
	cairo_arc (cr,
		   widget->allocation.width / 2.0,
		   widget->allocation.height / 2.0,
		   priv->render_size / 2.0,
		   0, 2 * M_PI);
	gdk_cairo_set_source_color (cr, priv->enabled ? &fill : &stroke);
	cairo_fill_preserve (cr);
	gdk_cairo_set_source_color (cr, &stroke);
	cairo_stroke (cr);

	cairo_destroy (cr);

	return TRUE;
}

gboolean
mcus_led_get_enabled (MCUSLED *self)
{
	g_return_val_if_fail (self != NULL, -1);
	g_return_val_if_fail (MCUS_IS_LED (self), -1);

	return self->priv->enabled;
}

void
mcus_led_set_enabled (MCUSLED *self, gboolean enabled)
{
	g_return_if_fail (self != NULL);
	g_return_if_fail (MCUS_IS_LED (self));

	self->priv->enabled = enabled ? TRUE : FALSE;

	/* Ensure we're redrawn */
	gtk_widget_queue_draw (GTK_WIDGET (self));
}
