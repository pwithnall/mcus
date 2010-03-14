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
#include <gtk/gtk.h>
#include <atk/atk.h>
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

/* Table mapping digits to the active segment assignments required to display them */
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
static void mcus_seven_segment_display_finalize (GObject *object);
static void mcus_seven_segment_display_size_request (GtkWidget *widget, GtkRequisition *requisition);
static void mcus_seven_segment_display_size_allocate (GtkWidget *widget, GtkAllocation *allocation);
static gint mcus_seven_segment_display_expose_event (GtkWidget *widget, GdkEventExpose *event);
static AtkObject *mcus_seven_segment_display_get_accessible (GtkWidget *widget);
static GType mcus_seven_segment_display_accessible_get_type (void) G_GNUC_CONST;

struct _MCUSSevenSegmentDisplayPrivate {
	guint8 segments;
	gint digit;
	gdouble render_width;
	gdouble render_height;
	gdouble render_x;
	gdouble render_y;
	gchar *description; /* accessible description of the SSD's state */
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

	gobject_class->get_property = mcus_seven_segment_display_get_property;
	gobject_class->set_property = mcus_seven_segment_display_set_property;
	gobject_class->finalize = mcus_seven_segment_display_finalize;

	widget_class->size_request = mcus_seven_segment_display_size_request;
	widget_class->size_allocate = mcus_seven_segment_display_size_allocate;
	widget_class->expose_event = mcus_seven_segment_display_expose_event;
	widget_class->get_accessible = mcus_seven_segment_display_get_accessible;

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

	/* We don't have a window of our own; we use our parent's */
	gtk_widget_set_has_window (GTK_WIDGET (self), FALSE);
}

static void
mcus_seven_segment_display_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	MCUSSevenSegmentDisplayPrivate *priv = MCUS_SEVEN_SEGMENT_DISPLAY (object)->priv;

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
	MCUSSevenSegmentDisplay *self = MCUS_SEVEN_SEGMENT_DISPLAY (object);

	switch (property_id) {
		case PROP_DIGIT:
			self->priv->digit = g_value_get_int (value);
			if (self->priv->digit != -1)
				mcus_seven_segment_display_set_digit (self, self->priv->digit);
			break;
		case PROP_SEGMENT_A_ENABLED:
			mcus_seven_segment_display_set_segment (self, SEGMENT_A, g_value_get_boolean (value));
			break;
		case PROP_SEGMENT_B_ENABLED:
			mcus_seven_segment_display_set_segment (self, SEGMENT_B, g_value_get_boolean (value));
			break;
		case PROP_SEGMENT_C_ENABLED:
			mcus_seven_segment_display_set_segment (self, SEGMENT_C, g_value_get_boolean (value));
			break;
		case PROP_SEGMENT_D_ENABLED:
			mcus_seven_segment_display_set_segment (self, SEGMENT_D, g_value_get_boolean (value));
			break;
		case PROP_SEGMENT_E_ENABLED:
			mcus_seven_segment_display_set_segment (self, SEGMENT_E, g_value_get_boolean (value));
			break;
		case PROP_SEGMENT_F_ENABLED:
			mcus_seven_segment_display_set_segment (self, SEGMENT_F, g_value_get_boolean (value));
			break;
		case PROP_SEGMENT_G_ENABLED:
			mcus_seven_segment_display_set_segment (self, SEGMENT_G, g_value_get_boolean (value));
			break;
		case PROP_POINT_ENABLED:
			mcus_seven_segment_display_set_segment (self, SEGMENT_POINT, g_value_get_boolean (value));
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
mcus_seven_segment_display_finalize (GObject *object)
{
	MCUSSevenSegmentDisplayPrivate *priv = MCUS_SEVEN_SEGMENT_DISPLAY (object)->priv;

	g_free (priv->description);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (mcus_seven_segment_display_parent_class)->finalize (object);
}

static void
mcus_seven_segment_display_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
	g_return_if_fail (requisition != NULL);
	g_return_if_fail (MCUS_IS_SEVEN_SEGMENT_DISPLAY (widget));

	requisition->width = MINIMUM_WIDTH;
	requisition->height = MINIMUM_HEIGHT;
}

static void
mcus_seven_segment_display_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	MCUSSevenSegmentDisplayPrivate *priv;
	GtkStyle *style;

	g_return_if_fail (allocation != NULL);
	g_return_if_fail (MCUS_IS_SEVEN_SEGMENT_DISPLAY (widget));

	GTK_WIDGET_CLASS (mcus_seven_segment_display_parent_class)->size_allocate (widget, allocation);

	priv = MCUS_SEVEN_SEGMENT_DISPLAY (widget)->priv;

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
	style = gtk_widget_get_style (widget);
	priv->render_width -= style->xthickness * 2.0;
	priv->render_height -= style->xthickness * 2.0;

	priv->render_x = (allocation->width - priv->render_width) / 2.0;
	priv->render_y = (allocation->height - priv->render_height) / 2.0;
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
	GtkStyle *style;
	GtkAllocation allocation;

	g_return_val_if_fail (event != NULL, FALSE);
	g_return_val_if_fail (MCUS_IS_SEVEN_SEGMENT_DISPLAY (widget), FALSE);

	if (gtk_widget_is_drawable (widget) == FALSE)
		return FALSE;

	priv = MCUS_SEVEN_SEGMENT_DISPLAY (widget)->priv;

	/* Prepare our custom colours */
	segment_fill.red = 29555; /* Tango's medium "chameleon" */
	segment_fill.green = 53970;
	segment_fill.blue = 5654;
	segment_stroke.red = 34952; /* Tango's lightest "aluminium" --- 888a85 */
	segment_stroke.green = 35466;
	segment_stroke.blue = 34181;

	/* Draw! */
	cr = gdk_cairo_create (gtk_widget_get_window (widget));

	/* Clip to the exposed area */
	cairo_rectangle (cr, event->area.x, event->area.y, event->area.width, event->area.height);
	cairo_clip (cr);

	/* Sort out sizes, ratios, etc. */
	style = gtk_widget_get_style (widget);
	gtk_widget_get_allocation (widget, &allocation);
	cairo_translate (cr, allocation.x + priv->render_x, allocation.y + priv->render_y);
	cairo_set_line_width (cr, style->xthickness / (priv->render_width / EXTERNAL_WIDTH)); /* make sure the thickness isn't scaled */
	cairo_scale (cr, priv->render_width / EXTERNAL_WIDTH, priv->render_height / EXTERNAL_HEIGHT);

	/* Draw the body of the display */
	cairo_rectangle (cr, 0.0, 0.0, EXTERNAL_WIDTH, EXTERNAL_HEIGHT);
	gdk_cairo_set_source_color (cr, &(style->mid[GTK_STATE_NORMAL]));
	cairo_fill_preserve (cr);
	gdk_cairo_set_source_color (cr, &(style->dark[GTK_STATE_NORMAL]));
	cairo_stroke (cr);

	/* Draw the decimal point */
	cairo_arc (cr, EXTERNAL_WIDTH - (EXTERNAL_HEIGHT - INTERNAL_HEIGHT) / 2.0 + SEGMENT_SEPARATION * 2.0,
	               EXTERNAL_HEIGHT - (EXTERNAL_HEIGHT - INTERNAL_HEIGHT) / 2.0,
	               DOT_RADIUS, 0.0, 2.0 * M_PI);
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
	cairo_translate (cr, 0.0, -SEGMENT_LENGTH - SEGMENT_SEPARATION * 2.0);
	cairo_save (cr);
	cairo_rotate (cr, -SEGMENT_ANGLE);
	draw_segment (cr, &segment_fill, &segment_stroke, priv->segments & SEGMENT_A_ACTIVE);
	cairo_restore (cr);

	/* Segment D */
	cairo_translate (cr, 0.0, SEGMENT_LENGTH * 2.0 + SEGMENT_SEPARATION * 4.0);
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
	cairo_translate (cr, -SEGMENT_LENGTH - SEGMENT_SEPARATION * 2.0, 0.0);
	draw_segment (cr, &segment_fill, &segment_stroke, priv->segments & SEGMENT_B_ACTIVE);

	/* Segment F */
	cairo_rotate (cr, -SEGMENT_ANGLE);
	cairo_translate (cr, 0.0, SEGMENT_LENGTH + SEGMENT_SEPARATION * 2.0);
	cairo_rotate (cr, SEGMENT_ANGLE);
	draw_segment (cr, &segment_fill, &segment_stroke, priv->segments & SEGMENT_F_ACTIVE);

	cairo_destroy (cr);

	return FALSE;
}

static void
update_accessible_description (MCUSSevenSegmentDisplay *self)
{
	guint8 segments = self->priv->segments;

	g_free (self->priv->description);

	/* If a digit is being displayed, have that (and the point) as the accessible description */
	if (self->priv->digit != -1) {
		self->priv->description = g_strdup_printf ("%u%s", self->priv->digit, (segments & POINT_ACTIVE) ? "." : "");
		return;
	}

	/* Otherwise, the string is just a concatenation of the letters representing the different segments which are currently active */
	self->priv->description = g_strdup_printf ("%s%s%s%s%s%s%s%s",
	                                           /* Translators: this is the name of the top segment of a seven-segment display */
	                                           (segments & SEGMENT_A_ACTIVE) ? _("A") : "",
	                                           /* Translators: this is the name of the top-right segment of a seven-segment display */
	                                           (segments & SEGMENT_B_ACTIVE) ? _("B") : "",
	                                           /* Translators: this is the name of the bottom-right segment of a seven-segment display */
	                                           (segments & SEGMENT_C_ACTIVE) ? _("C") : "",
	                                           /* Translators: this is the name of the bottom segment of a seven-segment display */
	                                           (segments & SEGMENT_D_ACTIVE) ? _("D") : "",
	                                           /* Translators: this is the name of the bottom-left segment of a seven-segment display */
	                                           (segments & SEGMENT_E_ACTIVE) ? _("E") : "",
	                                           /* Translators: this is the name of the top-left segment of a seven-segment display */
	                                           (segments & SEGMENT_F_ACTIVE) ? _("F") : "",
	                                           /* Translators: this is the name of the middle segment of a seven-segment display */
	                                           (segments & SEGMENT_G_ACTIVE) ? _("G") : "",
	                                           /* Translators: this is the name of the decimal point segment of a seven-segment display */
	                                           (segments & POINT_ACTIVE) ? _(".") : "");
}

guint8
mcus_seven_segment_display_get_segment_mask (MCUSSevenSegmentDisplay *self)
{
	g_return_val_if_fail (MCUS_IS_SEVEN_SEGMENT_DISPLAY (self), 0);

	return self->priv->segments;
}

void
mcus_seven_segment_display_set_segment_mask (MCUSSevenSegmentDisplay *self, guint8 segment_mask)
{
	g_return_if_fail (MCUS_IS_SEVEN_SEGMENT_DISPLAY (self));

	self->priv->segments = segment_mask;
	self->priv->digit = -1;

	/* Update the accessible description */
	update_accessible_description (self);

	/* Ensure we're redrawn */
	gtk_widget_queue_draw (GTK_WIDGET (self));
}

gint
mcus_seven_segment_display_get_digit (MCUSSevenSegmentDisplay *self)
{
	g_return_val_if_fail (MCUS_IS_SEVEN_SEGMENT_DISPLAY (self), -1);

	return self->priv->digit;
}

void
mcus_seven_segment_display_set_digit (MCUSSevenSegmentDisplay *self, guint digit)
{
	g_return_if_fail (/*digit >= 0 && */digit <= 9);
	g_return_if_fail (MCUS_IS_SEVEN_SEGMENT_DISPLAY (self));

	self->priv->digit = digit;

	/* Preserve the state of the point */
	self->priv->segments = segment_digit_map[digit] | (self->priv->segments & POINT_ACTIVE);

	/* Update the accessible description */
	update_accessible_description (self);

	/* Ensure we're redrawn */
	gtk_widget_queue_draw (GTK_WIDGET (self));
}

gboolean
mcus_seven_segment_display_get_segment (MCUSSevenSegmentDisplay *self, MCUSSevenSegmentDisplaySegment segment)
{
	g_return_val_if_fail (MCUS_IS_SEVEN_SEGMENT_DISPLAY (self), FALSE);

	return self->priv->segments & (1 << segment);
}

void
mcus_seven_segment_display_set_segment (MCUSSevenSegmentDisplay *self, MCUSSevenSegmentDisplaySegment segment, gboolean enabled)
{
	g_return_if_fail (MCUS_IS_SEVEN_SEGMENT_DISPLAY (self));

	if (enabled == TRUE)
		self->priv->segments |= 1 << segment;
	else
		self->priv->segments &= ~(1 << segment);
	self->priv->digit = -1;

	/* Update the accessible description */
	update_accessible_description (self);

	/* Ensure we're redrawn */
	gtk_widget_queue_draw (GTK_WIDGET (self));
}

/* Accessibility stuff */
static AtkObject *
mcus_seven_segment_display_accessible_new (GObject *object)
{
	AtkObject *accessible;

	g_return_val_if_fail (GTK_IS_WIDGET (object), NULL);

	accessible = g_object_new (mcus_seven_segment_display_accessible_get_type (), NULL);
	atk_object_initialize (accessible, object);

	return accessible;
}

static void
mcus_seven_segment_display_accessible_factory_class_init (AtkObjectFactoryClass *klass)
{
	klass->create_accessible = mcus_seven_segment_display_accessible_new;
	klass->get_accessible_type = mcus_seven_segment_display_accessible_get_type;
}

static GType
mcus_seven_segment_display_accessible_factory_get_type (void)
{
	static GType type = 0;

	if (!type) {
		const GTypeInfo tinfo = {
			sizeof (AtkObjectFactoryClass),
			NULL,           /* base_init */
			NULL,           /* base_finalize */
			(GClassInitFunc) mcus_seven_segment_display_accessible_factory_class_init,
			NULL,           /* class_finalize */
			NULL,           /* class_data */
			sizeof (AtkObjectFactory),
			0,             /* n_preallocs */
			NULL, NULL
		};

		type = g_type_register_static (ATK_TYPE_OBJECT_FACTORY, _("MCUSSevenSegmentDisplayAccessibleFactory"), &tinfo, 0);
	}

	return type;
}

static AtkObjectClass *a11y_parent_class = NULL;

static void
mcus_seven_segment_display_accessible_initialize (AtkObject *accessible, gpointer widget)
{
	atk_object_set_name (accessible, _("Seven-Segment Display"));
	atk_object_set_description (accessible, _("Simulates a hardware seven-segment display"));

	a11y_parent_class->initialize (accessible, widget);
}

static void
mcus_seven_segment_display_accessible_class_init (AtkObjectClass *klass)
{
	a11y_parent_class = g_type_class_peek_parent (klass);
	klass->initialize = mcus_seven_segment_display_accessible_initialize;
}

static void
mcus_seven_segment_display_accessible_image_get_size (AtkImage *image, gint *width, gint *height)
{
	GtkWidget *widget = GTK_ACCESSIBLE (image)->widget;

	if (!widget) {
		*width = *height = 0;
	} else {
		GtkAllocation allocation;
		gtk_widget_get_allocation (widget, &allocation);

		*width = allocation.width;
		*height = allocation.height;
	}
}

static const gchar *
mcus_seven_segment_display_accessible_image_get_description (AtkImage *image)
{
	MCUSSevenSegmentDisplay *self = MCUS_SEVEN_SEGMENT_DISPLAY (GTK_ACCESSIBLE (image)->widget);
	return self->priv->description;
}

static void
mcus_seven_segment_display_accessible_image_interface_init (AtkImageIface *iface)
{
	iface->get_image_size = mcus_seven_segment_display_accessible_image_get_size;
	iface->get_image_description = mcus_seven_segment_display_accessible_image_get_description;
}

static GType
mcus_seven_segment_display_accessible_get_type (void)
{
	static GType type = 0;

	/* Action interface
	   Name etc. ... */
	if (G_UNLIKELY (type == 0)) {
		const GInterfaceInfo atk_image_info = {
			(GInterfaceInitFunc) mcus_seven_segment_display_accessible_image_interface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL
		};
		GType parent_atk_type;
		GTypeInfo tinfo = { 0 };
		GTypeQuery query;
		AtkObjectFactory *factory;

		if ((type = g_type_from_name ("MCUSSevenSegmentDisplayAccessible")))
			return type;

		factory = atk_registry_get_factory (atk_get_default_registry (), GTK_TYPE_IMAGE);
		if (!factory)
			return G_TYPE_INVALID;

		parent_atk_type = atk_object_factory_get_accessible_type (factory);
		if (!parent_atk_type)
			return G_TYPE_INVALID;

		/* Figure out the size of the class and instance we are deriving from */
		g_type_query (parent_atk_type, &query);

		tinfo.class_init = (GClassInitFunc) mcus_seven_segment_display_accessible_class_init;
		tinfo.class_size = query.class_size;
		tinfo.instance_size = query.instance_size;

		/* Register the type */
		type = g_type_register_static (parent_atk_type, "MCUSSevenSegmentDisplayAccessible", &tinfo, 0);

		g_type_add_interface_static (type, ATK_TYPE_IMAGE, &atk_image_info);
	}

	return type;
}

static AtkObject *
mcus_seven_segment_display_get_accessible (GtkWidget *widget)
{
	static gboolean first_time = TRUE;

	if (first_time) {
		AtkObjectFactory *factory;
		AtkRegistry *registry;
		GType derived_type, derived_atk_type;

		/* Figure out whether accessibility is enabled by looking at the type of the accessible object which would be created for
		 * the parent type of MCUSLED. */
		derived_type = g_type_parent (MCUS_TYPE_SEVEN_SEGMENT_DISPLAY);

		registry = atk_get_default_registry ();
		factory = atk_registry_get_factory (registry, derived_type);
		derived_atk_type = atk_object_factory_get_accessible_type (factory);
		if (g_type_is_a (derived_atk_type, GTK_TYPE_ACCESSIBLE)) {
			atk_registry_set_factory_type (registry, MCUS_TYPE_SEVEN_SEGMENT_DISPLAY,
			                               mcus_seven_segment_display_accessible_factory_get_type ());
		}

		first_time = FALSE;
	}

	return GTK_WIDGET_CLASS (mcus_seven_segment_display_parent_class)->get_accessible (widget);
}
