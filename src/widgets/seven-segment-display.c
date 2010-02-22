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
#include <string.h>

#include "seven-segment-display.h"

/* Ratio of width to height */
#define WIDTH_HEIGHT_RATIO EXTERNAL_HEIGHT / EXTERNAL_WIDTH

/* Minimum widget height and width in device units */
#define MINIMUM_WIDTH 50.0
#define MINIMUM_HEIGHT MINIMUM_WIDTH * WIDTH_HEIGHT_RATIO

/* External casing width in mm */
#define EXTERNAL_WIDTH 12.7
/* External casing height in mm */
#define EXTERNAL_HEIGHT 19.0
/* Total internal height of all the segments */
#define INTERNAL_HEIGHT 12.7

/* Decimal point radius in mm */
#define DOT_RADIUS 0.75

/* Segment width in mm */
#define SEGMENT_WIDTH 1.4
/* Segment length in mm */
#define SEGMENT_LENGTH 6.0
/* Chamfer size in mm */
#define SEGMENT_CHAMFER SEGMENT_WIDTH / 2.0
/* Angle of inclination of the segments (10 degrees) */
#define SEGMENT_ANGLE 10.0 / 360.0 * 2 * M_PI
/* Separation between segments at the joints */
#define SEGMENT_SEPARATION 0.2

enum {
	SEGMENT_A_ACTIVE = 1 << 0,
	SEGMENT_B_ACTIVE = 1 << 1,
	SEGMENT_C_ACTIVE = 1 << 2,
	SEGMENT_D_ACTIVE = 1 << 3,
	SEGMENT_E_ACTIVE = 1 << 4,
	SEGMENT_F_ACTIVE = 1 << 5,
	SEGMENT_G_ACTIVE = 1 << 6,
	POINT_ACTIVE = 1 << 7,
	ALL_SEGMENTS_ACTIVE = 0xFF
};

static const guint8 segment_digit_map[10] = {
	ALL_SEGMENTS_ACTIVE ^ SEGMENT_G_ACTIVE ^ POINT_ACTIVE,						/* "0" */
	SEGMENT_B_ACTIVE | SEGMENT_C_ACTIVE,								/* "1" */
	ALL_SEGMENTS_ACTIVE ^ SEGMENT_C_ACTIVE ^ SEGMENT_F_ACTIVE ^ POINT_ACTIVE,			/* "2" */
	ALL_SEGMENTS_ACTIVE ^ SEGMENT_E_ACTIVE ^ SEGMENT_F_ACTIVE ^ POINT_ACTIVE,			/* "3" */
	ALL_SEGMENTS_ACTIVE ^ SEGMENT_A_ACTIVE ^ SEGMENT_D_ACTIVE ^ SEGMENT_E_ACTIVE ^ POINT_ACTIVE,	/* "4" */
	ALL_SEGMENTS_ACTIVE ^ SEGMENT_B_ACTIVE ^ SEGMENT_E_ACTIVE ^ POINT_ACTIVE,			/* "5" */
	ALL_SEGMENTS_ACTIVE ^ SEGMENT_B_ACTIVE ^ POINT_ACTIVE,						/* "6" */
	SEGMENT_A_ACTIVE | SEGMENT_B_ACTIVE | SEGMENT_C_ACTIVE,						/* "7" */
	ALL_SEGMENTS_ACTIVE ^ POINT_ACTIVE,								/* "8" */
	ALL_SEGMENTS_ACTIVE ^ SEGMENT_E_ACTIVE ^ POINT_ACTIVE						/* "9" */
};

static void mcus_seven_segment_display_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void mcus_seven_segment_display_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void mcus_seven_segment_display_realize (GtkWidget *widget);
static void mcus_seven_segment_display_size_request (GtkWidget *widget, GtkRequisition *requisition);
static void mcus_seven_segment_display_size_allocate (GtkWidget *widget, GtkAllocation *allocation);
static gint mcus_seven_segment_display_expose_event (GtkWidget *widget, GdkEventExpose *event);

struct _MCUSSevenSegmentDisplayPrivate {
	guint8 segments;
	gint digit;
	gdouble render_width;
	gdouble render_height;
	gdouble render_x;
	gdouble render_y;
};

enum {
	PROP_DIGIT = 1,
	PROP_SEGMENT_A_ENABLED,
	PROP_SEGMENT_B_ENABLED,
	PROP_SEGMENT_C_ENABLED,
	PROP_SEGMENT_D_ENABLED,
	PROP_SEGMENT_E_ENABLED,
	PROP_SEGMENT_F_ENABLED,
	PROP_SEGMENT_G_ENABLED,
	PROP_POINT_ENABLED
};

G_DEFINE_TYPE (MCUSSevenSegmentDisplay, mcus_seven_segment_display, GTK_TYPE_WIDGET)
#define MCUS_SEVEN_SEGMENT_DISPLAY_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MCUS_TYPE_SEVEN_SEGMENT_DISPLAY, MCUSSevenSegmentDisplayPrivate))

MCUSSevenSegmentDisplay *
mcus_seven_segment_display_new (void)
{
	return g_object_new (MCUS_TYPE_SEVEN_SEGMENT_DISPLAY, NULL);
}

static void
mcus_seven_segment_display_class_init (MCUSSevenSegmentDisplayClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	g_type_class_add_private (klass, sizeof (MCUSSevenSegmentDisplayPrivate));

	gobject_class->set_property = mcus_seven_segment_display_set_property;
	gobject_class->get_property = mcus_seven_segment_display_get_property;

	widget_class->realize = mcus_seven_segment_display_realize;
	widget_class->size_request = mcus_seven_segment_display_size_request;
	widget_class->size_allocate = mcus_seven_segment_display_size_allocate;
	widget_class->expose_event = mcus_seven_segment_display_expose_event;

	g_object_class_install_property (gobject_class, PROP_DIGIT,
				g_param_spec_int ("digit",
					"Digit", "The digit to display on the seven-segment display.",
					-1, 9, -1,
					G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (gobject_class, PROP_SEGMENT_A_ENABLED,
				g_param_spec_boolean ("segment-a-enabled",
					"Segment A Enabled", "Whether segment A is enabled.",
					FALSE,
					G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (gobject_class, PROP_SEGMENT_B_ENABLED,
				g_param_spec_boolean ("segment-b-enabled",
					"Segment B Enabled", "Whether segment B is enabled.",
					FALSE,
					G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (gobject_class, PROP_SEGMENT_C_ENABLED,
				g_param_spec_boolean ("segment-c-enabled",
					"Segment C Enabled", "Whether segment C is enabled.",
					FALSE,
					G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (gobject_class, PROP_SEGMENT_D_ENABLED,
				g_param_spec_boolean ("segment-d-enabled",
					"Segment D Enabled", "Whether segment D is enabled.",
					FALSE,
					G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (gobject_class, PROP_SEGMENT_E_ENABLED,
				g_param_spec_boolean ("segment-e-enabled",
					"Segment E Enabled", "Whether segment E is enabled.",
					FALSE,
					G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (gobject_class, PROP_SEGMENT_F_ENABLED,
				g_param_spec_boolean ("segment-f-enabled",
					"Segment F Enabled", "Whether segment F is enabled.",
					FALSE,
					G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (gobject_class, PROP_SEGMENT_G_ENABLED,
				g_param_spec_boolean ("segment-g-enabled",
					"Segment G Enabled", "Whether segment G is enabled.",
					FALSE,
					G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (gobject_class, PROP_POINT_ENABLED,
				g_param_spec_boolean ("point-enabled",
					"Point Enabled", "Whether the point is enabled.",
					FALSE,
					G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
mcus_seven_segment_display_init (MCUSSevenSegmentDisplay *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, MCUS_TYPE_SEVEN_SEGMENT_DISPLAY, MCUSSevenSegmentDisplayPrivate);
	self->priv->digit = -1;
}

static void
mcus_seven_segment_display_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	MCUSSevenSegmentDisplayPrivate *priv = MCUS_SEVEN_SEGMENT_DISPLAY_GET_PRIVATE (object);

	switch (property_id) {
		case PROP_DIGIT:
			g_value_set_int (value, priv->digit);
			break;
		case PROP_SEGMENT_A_ENABLED:
			g_value_set_boolean (value, priv->segments & SEGMENT_A_ACTIVE);
			break;
		case PROP_SEGMENT_B_ENABLED:
			g_value_set_boolean (value, priv->segments & SEGMENT_B_ACTIVE);
			break;
		case PROP_SEGMENT_C_ENABLED:
			g_value_set_boolean (value, priv->segments & SEGMENT_C_ACTIVE);
			break;
		case PROP_SEGMENT_D_ENABLED:
			g_value_set_boolean (value, priv->segments & SEGMENT_D_ACTIVE);
			break;
		case PROP_SEGMENT_E_ENABLED:
			g_value_set_boolean (value, priv->segments & SEGMENT_E_ACTIVE);
			break;
		case PROP_SEGMENT_F_ENABLED:
			g_value_set_boolean (value, priv->segments & SEGMENT_F_ACTIVE);
			break;
		case PROP_SEGMENT_G_ENABLED:
			g_value_set_boolean (value, priv->segments & SEGMENT_G_ACTIVE);
			break;
		case PROP_POINT_ENABLED:
			g_value_set_boolean (value, priv->segments & POINT_ACTIVE);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
mcus_seven_segment_display_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	MCUSSevenSegmentDisplayPrivate *priv = MCUS_SEVEN_SEGMENT_DISPLAY_GET_PRIVATE (object);

	switch (property_id) {
		case PROP_DIGIT:
			priv->digit = g_value_get_int (value);
			if (priv->digit != -1)
				mcus_seven_segment_display_set_digit (MCUS_SEVEN_SEGMENT_DISPLAY (object), priv->digit);
			break;
		case PROP_SEGMENT_A_ENABLED:
			if (g_value_get_boolean (value) == TRUE)
				priv->segments |= SEGMENT_A_ACTIVE;
			else
				priv->segments &= ~SEGMENT_A_ACTIVE;
			break;
		case PROP_SEGMENT_B_ENABLED:
			if (g_value_get_boolean (value) == TRUE)
				priv->segments |= SEGMENT_B_ACTIVE;
			else
				priv->segments &= ~SEGMENT_B_ACTIVE;
			break;
		case PROP_SEGMENT_C_ENABLED:
			if (g_value_get_boolean (value) == TRUE)
				priv->segments |= SEGMENT_C_ACTIVE;
			else
				priv->segments &= ~SEGMENT_C_ACTIVE;
			break;
		case PROP_SEGMENT_D_ENABLED:
			if (g_value_get_boolean (value) == TRUE)
				priv->segments |= SEGMENT_D_ACTIVE;
			else
				priv->segments &= ~SEGMENT_D_ACTIVE;
			break;
		case PROP_SEGMENT_E_ENABLED:
			if (g_value_get_boolean (value) == TRUE)
				priv->segments |= SEGMENT_E_ACTIVE;
			else
				priv->segments &= ~SEGMENT_E_ACTIVE;
			break;
		case PROP_SEGMENT_F_ENABLED:
			if (g_value_get_boolean (value) == TRUE)
				priv->segments |= SEGMENT_F_ACTIVE;
			else
				priv->segments &= ~SEGMENT_F_ACTIVE;
			break;
		case PROP_SEGMENT_G_ENABLED:
			if (g_value_get_boolean (value) == TRUE)
				priv->segments |= SEGMENT_G_ACTIVE;
			else
				priv->segments &= ~SEGMENT_G_ACTIVE;
			break;
		case PROP_POINT_ENABLED:
			if (g_value_get_boolean (value) == TRUE)
				priv->segments |= POINT_ACTIVE;
			else
				priv->segments &= ~POINT_ACTIVE;
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
mcus_seven_segment_display_realize (GtkWidget *widget)
{
	MCUSSevenSegmentDisplay *self;
	GdkWindowAttr attributes;
	gint attr_mask;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (MCUS_IS_SEVEN_SEGMENT_DISPLAY (widget));

	/* Mark the widget as realised */
	GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
	self = MCUS_SEVEN_SEGMENT_DISPLAY (widget);

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
mcus_seven_segment_display_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
	g_return_if_fail (widget != NULL || requisition != NULL);
	g_return_if_fail (MCUS_IS_SEVEN_SEGMENT_DISPLAY (widget));

	requisition->width = MINIMUM_WIDTH;
	requisition->height = MINIMUM_HEIGHT;
}

static void
mcus_seven_segment_display_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	MCUSSevenSegmentDisplayPrivate *priv;

	g_return_if_fail (widget != NULL || allocation != NULL);
	g_return_if_fail (MCUS_IS_SEVEN_SEGMENT_DISPLAY (widget));

	widget->allocation = *allocation;
	priv = MCUS_SEVEN_SEGMENT_DISPLAY_GET_PRIVATE (widget);

	/* Sort out sizes, ratios, etc. */
	if (allocation->height < allocation->width * WIDTH_HEIGHT_RATIO) {
		priv->render_width = allocation->height;
		priv->render_width /= WIDTH_HEIGHT_RATIO; /* for some reason, this has to be done in two stages, or we end up with a completely incorrect value */
		priv->render_height = allocation->height;
	} else {
		priv->render_width = allocation->width;
		priv->render_height = allocation->width * WIDTH_HEIGHT_RATIO;
	}

	/* Ensure the borders aren't clipped */
	priv->render_width -= widget->style->xthickness * 2.0;
	priv->render_height -= widget->style->xthickness * 2.0;

	priv->render_x = (allocation->width - priv->render_width) / 2.0;
	priv->render_y = (allocation->height - priv->render_height) / 2.0;

	if (GTK_WIDGET_REALIZED (widget))
		gdk_window_move_resize (widget->window, allocation->x, allocation->y, allocation->width, allocation->height);
}

static void
draw_segment (cairo_t *cr, GdkColor *fill_colour, GdkColor *stroke_colour, gboolean enabled)
{
	cairo_save (cr);

	cairo_move_to (cr, -SEGMENT_LENGTH / 2.0 + SEGMENT_CHAMFER, SEGMENT_WIDTH / 2.0);
	cairo_line_to (cr, SEGMENT_LENGTH / 2.0 - SEGMENT_CHAMFER, SEGMENT_WIDTH / 2.0);
	cairo_line_to (cr, SEGMENT_LENGTH / 2.0, 0);
	cairo_line_to (cr, SEGMENT_LENGTH / 2.0 - SEGMENT_CHAMFER, -SEGMENT_WIDTH / 2.0);
	cairo_line_to (cr, -SEGMENT_LENGTH / 2.0 + SEGMENT_CHAMFER, -SEGMENT_WIDTH / 2.0);
	cairo_line_to (cr, -SEGMENT_LENGTH / 2.0, 0);
	cairo_line_to (cr, -SEGMENT_LENGTH / 2.0 + SEGMENT_CHAMFER, SEGMENT_WIDTH / 2.0);

	gdk_cairo_set_source_color (cr, enabled ? fill_colour : stroke_colour);
	cairo_fill_preserve (cr);
	gdk_cairo_set_source_color (cr, stroke_colour);
	cairo_stroke (cr);

	cairo_restore (cr);
}

static gint
mcus_seven_segment_display_expose_event (GtkWidget *widget, GdkEventExpose *event)
{
	cairo_t *cr;
	MCUSSevenSegmentDisplayPrivate *priv;
	GdkColor segment_fill, segment_stroke;

	g_return_val_if_fail (widget != NULL || event != NULL, FALSE);
	g_return_val_if_fail (MCUS_IS_SEVEN_SEGMENT_DISPLAY (widget), FALSE);

	/* Compress exposes */
	if (event->count > 0)
		return TRUE;

	priv = MCUS_SEVEN_SEGMENT_DISPLAY_GET_PRIVATE (widget);

	/* Clear the area first */
	gdk_window_clear_area (widget->window, event->area.x, event->area.y, event->area.width, event->area.height);

	/* Prepare our custom colours */
	segment_fill.red = 29555; /* Tango's medium "chameleon" */
	segment_fill.green = 53970;
	segment_fill.blue = 5654;
	segment_stroke.red = 34952; /* Tango's lightest "aluminium" --- 888a85 */
	segment_stroke.green = 35466;
	segment_stroke.blue = 34181;

	/* Draw! */
	cr = gdk_cairo_create (widget->window);

	/* Clip to the exposed area */
	cairo_rectangle (cr, event->area.x, event->area.y, event->area.width, event->area.height);
	cairo_clip (cr);

	/* Sort out sizes, ratios, etc. */
	cairo_translate (cr, priv->render_x, priv->render_y);
	cairo_set_line_width (cr, widget->style->xthickness / (priv->render_width / EXTERNAL_WIDTH)); /* make sure the thickness isn't scaled */
	cairo_scale (cr, priv->render_width / EXTERNAL_WIDTH, priv->render_height / EXTERNAL_HEIGHT);

	/* Draw the body of the display */
	cairo_rectangle (cr, 0, 0, EXTERNAL_WIDTH, EXTERNAL_HEIGHT);
	gdk_cairo_set_source_color (cr, &widget->style->mid[GTK_STATE_NORMAL]);
	cairo_fill_preserve (cr);
	gdk_cairo_set_source_color (cr, &widget->style->dark[GTK_STATE_NORMAL]);
	cairo_stroke (cr);

	/* Draw the decimal point */
	cairo_arc (cr, EXTERNAL_WIDTH - (EXTERNAL_HEIGHT - INTERNAL_HEIGHT) / 2.0 + SEGMENT_SEPARATION * 2.0,
		       EXTERNAL_HEIGHT - (EXTERNAL_HEIGHT - INTERNAL_HEIGHT) / 2.0,
		       DOT_RADIUS, 0, 2 * M_PI);
	gdk_cairo_set_source_color (cr, priv->segments & POINT_ACTIVE ? &segment_fill : &segment_stroke);
	cairo_fill_preserve (cr);
	gdk_cairo_set_source_color (cr, &segment_stroke);
	cairo_stroke (cr);

	/* Start with the middle segment (G) */
	cairo_translate (cr, EXTERNAL_WIDTH / 2.0, EXTERNAL_HEIGHT / 2.0);
	draw_segment (cr, &segment_fill, &segment_stroke, priv->segments & SEGMENT_G_ACTIVE);

	/* The other segments are angled */
	cairo_rotate (cr, SEGMENT_ANGLE);

	/* Segment A */
	cairo_translate (cr, 0, -SEGMENT_LENGTH - SEGMENT_SEPARATION * 2.0);
	cairo_save (cr);
	cairo_rotate (cr, -SEGMENT_ANGLE);
	draw_segment (cr, &segment_fill, &segment_stroke, priv->segments & SEGMENT_A_ACTIVE);
	cairo_restore (cr);

	/* Segment D */
	cairo_translate (cr, 0, SEGMENT_LENGTH * 2.0 + SEGMENT_SEPARATION * 4.0);
	cairo_save (cr);
	cairo_rotate (cr, -SEGMENT_ANGLE);
	draw_segment (cr, &segment_fill, &segment_stroke, priv->segments & SEGMENT_D_ACTIVE);
	cairo_restore (cr);

	/* Segments F, B, E and C are vertical */
	cairo_rotate (cr, M_PI / 2.0);

	/* Segment E */
	cairo_translate (cr, -SEGMENT_LENGTH / 2.0 + SEGMENT_SEPARATION, SEGMENT_LENGTH / 2.0 + SEGMENT_SEPARATION);
	draw_segment (cr, &segment_fill, &segment_stroke, priv->segments & SEGMENT_E_ACTIVE);

	/* Segment C */
	cairo_rotate (cr, -SEGMENT_ANGLE);
	cairo_translate (cr, SEGMENT_SEPARATION, -SEGMENT_LENGTH - SEGMENT_SEPARATION * 2.0);
	cairo_rotate (cr, SEGMENT_ANGLE);
	draw_segment (cr, &segment_fill, &segment_stroke, priv->segments & SEGMENT_C_ACTIVE);

	/* Segment B */
	cairo_translate (cr, -SEGMENT_LENGTH - SEGMENT_SEPARATION * 2.0, 0);
	draw_segment (cr, &segment_fill, &segment_stroke, priv->segments & SEGMENT_B_ACTIVE);

	/* Segment F */
	cairo_rotate (cr, -SEGMENT_ANGLE);
	cairo_translate (cr, 0, SEGMENT_LENGTH + SEGMENT_SEPARATION * 2.0);
	cairo_rotate (cr, SEGMENT_ANGLE);
	draw_segment (cr, &segment_fill, &segment_stroke, priv->segments & SEGMENT_F_ACTIVE);

	cairo_destroy (cr);

	return TRUE;
}

guint8
mcus_seven_segment_display_get_segment_mask (MCUSSevenSegmentDisplay *self)
{
	g_return_val_if_fail (self != NULL, 0);
	g_return_val_if_fail (MCUS_IS_SEVEN_SEGMENT_DISPLAY (self), 0);

	return self->priv->segments;
}

void
mcus_seven_segment_display_set_segment_mask (MCUSSevenSegmentDisplay *self, guint8 segment_mask)
{
	g_return_if_fail (self != NULL);
	g_return_if_fail (MCUS_IS_SEVEN_SEGMENT_DISPLAY (self));

	self->priv->segments = segment_mask;

	/* Ensure we're redrawn */
	gtk_widget_queue_draw (GTK_WIDGET (self));
}

gint
mcus_seven_segment_display_get_digit (MCUSSevenSegmentDisplay *self)
{
	g_return_val_if_fail (self != NULL, -1);
	g_return_val_if_fail (MCUS_IS_SEVEN_SEGMENT_DISPLAY (self), -1);

	return self->priv->digit;
}

void
mcus_seven_segment_display_set_digit (MCUSSevenSegmentDisplay *self, guint digit)
{
	g_return_if_fail (/*digit >= 0 && */digit <= 9);
	g_return_if_fail (self != NULL);
	g_return_if_fail (MCUS_IS_SEVEN_SEGMENT_DISPLAY (self));

	self->priv->digit = digit;

	/* Preserve the state of the point */
	self->priv->segments = segment_digit_map[digit] | (self->priv->segments & POINT_ACTIVE);

	/* Ensure we're redrawn */
	gtk_widget_queue_draw (GTK_WIDGET (self));
}

gboolean
mcus_seven_segment_display_get_segment (MCUSSevenSegmentDisplay *self, MCUSSevenSegmentDisplaySegment segment)
{
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (MCUS_IS_SEVEN_SEGMENT_DISPLAY (self), FALSE);

	return self->priv->segments & (1 << segment);
}

void
mcus_seven_segment_display_set_segment (MCUSSevenSegmentDisplay *self, MCUSSevenSegmentDisplaySegment segment, gboolean enabled)
{
	g_return_if_fail (self != NULL);
	g_return_if_fail (MCUS_IS_SEVEN_SEGMENT_DISPLAY (self));

	if (enabled == TRUE)
		self->priv->segments |= 1 << segment;
	else
		self->priv->segments &= ~(1 << segment);
	self->priv->digit = -1;

	/* Ensure we're redrawn */
	gtk_widget_queue_draw (GTK_WIDGET (self));
}
