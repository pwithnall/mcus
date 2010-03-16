/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * MCUS
 * Copyright (C) Philip Withnall 2010 <philip@tecnocode.co.uk>
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
#include <gtk/gtk.h>
#include <atk/atk.h>
#include <math.h>
#include <string.h>

#include "byte-array.h"

#define MAX_WIDTH_BYTES 16 /* maximum number of bytes to display on a line */

static void mcus_byte_array_dispose (GObject *object);
static void mcus_byte_array_finalize (GObject *object);
static void mcus_byte_array_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void mcus_byte_array_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void mcus_byte_array_size_request (GtkWidget *widget, GtkRequisition *requisition);
static gint mcus_byte_array_expose_event (GtkWidget *widget, GdkEventExpose *event);
static gboolean mcus_byte_array_query_tooltip (GtkWidget *widget, gint x, gint y, gboolean keyboard_tooltip, GtkTooltip *tooltip);
static void ensure_layout (MCUSByteArray *self);
static gint get_width_pu (MCUSByteArray *self);
static void get_layout_location (MCUSByteArray *self, gint *xp, gint *yp);

struct _MCUSByteArrayPrivate {
	const guchar *array;
	guint array_length;
	guint display_length;
	gint highlight_byte;

	PangoLayout *layout;
	PangoAttrList *attr_list;
	PangoAttribute *highlight_attr; /* owned by @attr_list */
	PangoFontDescription *font_desc;
	guint width_chars; /* number of characters in one line of the array */
	gchar *text;
};

enum {
	PROP_ARRAY = 1,
	PROP_ARRAY_LENGTH,
	PROP_DISPLAY_LENGTH,
	PROP_HIGHLIGHT_BYTE
};

G_DEFINE_TYPE (MCUSByteArray, mcus_byte_array, GTK_TYPE_MISC)

static void
mcus_byte_array_class_init (MCUSByteArrayClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	g_type_class_add_private (klass, sizeof (MCUSByteArrayPrivate));

	gobject_class->dispose = mcus_byte_array_dispose;
	gobject_class->finalize = mcus_byte_array_finalize;
	gobject_class->get_property = mcus_byte_array_get_property;
	gobject_class->set_property = mcus_byte_array_set_property;

	widget_class->size_request = mcus_byte_array_size_request;
	widget_class->expose_event = mcus_byte_array_expose_event;
	widget_class->query_tooltip = mcus_byte_array_query_tooltip;

	g_object_class_install_property (gobject_class, PROP_ARRAY,
				g_param_spec_pointer ("array",
					"Array", "The shared memory byte array to render.",
					G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (gobject_class, PROP_ARRAY_LENGTH,
				g_param_spec_uint ("array-length",
					"Array Length", "The length of the byte array, in bytes.",
					1, G_MAXUINT, 8,
					G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (gobject_class, PROP_DISPLAY_LENGTH,
				g_param_spec_uint ("display-length",
					"Display Length", "The number of bytes to display before ellipsizing.",
					0, G_MAXUINT, 8,
					G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (gobject_class, PROP_HIGHLIGHT_BYTE,
				g_param_spec_int ("highlight-byte",
					"Highlight Byte", "The zero-based index of a byte to highlight, or -1 to not highlight any bytes.",
					-1, G_MAXINT, -1,
					G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
mcus_byte_array_init (MCUSByteArray *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, MCUS_TYPE_BYTE_ARRAY, MCUSByteArrayPrivate);

	/* We don't have a window of our own; we use our parent's */
	gtk_widget_set_has_window (GTK_WIDGET (self), FALSE);

	/* Tooltip support */
	gtk_widget_set_has_tooltip (GTK_WIDGET (self), TRUE);
}

static void
mcus_byte_array_dispose (GObject *object)
{
	MCUSByteArrayPrivate *priv = MCUS_BYTE_ARRAY (object)->priv;

	if (priv->layout != NULL)
		g_object_unref (priv->layout);
	priv->layout = NULL;

	if (priv->attr_list != NULL)
		pango_attr_list_unref (priv->attr_list);
	priv->attr_list = NULL;

	/* Chain up to the parent class */
	G_OBJECT_CLASS (mcus_byte_array_parent_class)->dispose (object);
}

static void
mcus_byte_array_finalize (GObject *object)
{
	MCUSByteArrayPrivate *priv = MCUS_BYTE_ARRAY (object)->priv;

	g_free (priv->text);
	pango_font_description_free (priv->font_desc);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (mcus_byte_array_parent_class)->finalize (object);
}

static void
mcus_byte_array_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	MCUSByteArrayPrivate *priv = MCUS_BYTE_ARRAY (object)->priv;

	switch (property_id) {
		case PROP_ARRAY:
			g_value_set_pointer (value, (gpointer) priv->array);
			break;
		case PROP_ARRAY_LENGTH:
			g_value_set_uint (value, priv->array_length);
			break;
		case PROP_DISPLAY_LENGTH:
			g_value_set_uint (value, priv->display_length);
			break;
		case PROP_HIGHLIGHT_BYTE:
			g_value_set_int (value, priv->highlight_byte);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
mcus_byte_array_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	MCUSByteArray *self = MCUS_BYTE_ARRAY (object);

	switch (property_id) {
		case PROP_ARRAY:
			mcus_byte_array_set_array (self, g_value_get_pointer (value), self->priv->array_length);
			break;
		case PROP_ARRAY_LENGTH:
			mcus_byte_array_set_array (self, self->priv->array, g_value_get_uint (value));
			break;
		case PROP_DISPLAY_LENGTH:
			mcus_byte_array_set_display_length (self, g_value_get_uint (value));
			break;
		case PROP_HIGHLIGHT_BYTE:
			mcus_byte_array_set_highlight_byte (self, g_value_get_int (value));
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
mcus_byte_array_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
	MCUSByteArrayPrivate *priv = MCUS_BYTE_ARRAY (widget)->priv;
	PangoRectangle logical_rect;
	gint width, xpad, ypad;

	ensure_layout (MCUS_BYTE_ARRAY (widget));

	width = get_width_pu (MCUS_BYTE_ARRAY (widget));
	if (width == 0) {
		requisition->width = requisition->height = 0;
		return;
	}

	gtk_misc_get_padding (GTK_MISC (widget), &xpad, &ypad);
	pango_layout_set_width (priv->layout, width);
	pango_layout_get_extents (priv->layout, NULL, &logical_rect);

	requisition->width = PANGO_PIXELS (width) + 2 * xpad;
	requisition->height = PANGO_PIXELS (logical_rect.height) + 2 * ypad;
}

static gint
mcus_byte_array_expose_event (GtkWidget *widget, GdkEventExpose *event)
{
	gint x, y;
	GtkAllocation allocation;
	MCUSByteArrayPrivate *priv = MCUS_BYTE_ARRAY (widget)->priv;

	/* Don't draw if there's nothing to draw */
	if (priv->display_length == 0 || priv->array == NULL)
		return FALSE;

	ensure_layout (MCUS_BYTE_ARRAY (widget));
	get_layout_location (MCUS_BYTE_ARRAY (widget), &x, &y);

	gtk_widget_get_allocation (widget, &allocation);
	gtk_paint_layout (gtk_widget_get_style (widget), gtk_widget_get_window (widget), gtk_widget_get_state (widget), FALSE,
	                  &(event->area), widget, "label", x, y, priv->layout);

	return FALSE;
}

static gboolean
mcus_byte_array_query_tooltip (GtkWidget *widget, gint x, gint y, gboolean keyboard_tooltip, GtkTooltip *tooltip)
{
	MCUSByteArray *self = MCUS_BYTE_ARRAY (widget);
	gchar *text;
	guint byte_index;
	gint char_index, layout_x, layout_y;
	GtkAllocation allocation;

	/* Don't display tooltips on keyboard focus */
	if (keyboard_tooltip == TRUE)
		return FALSE;

	ensure_layout (self);
	get_layout_location (self, &layout_x, &layout_y);
	gtk_widget_get_allocation (widget, &allocation);

	/* Work out a character index into the layout's text underneath the cursor, or bail out if there isn't one */
	if (pango_layout_xy_to_index (self->priv->layout,
	                              (x - layout_x + allocation.x) * PANGO_SCALE,
	                              (y - layout_y + allocation.y) * PANGO_SCALE,
	                              &char_index, NULL) == FALSE) {
		return FALSE;
	}

	/* From the character index, we can calculate which byte the user's hovering over and display that in the tooltip */
	if (char_index < 0)
		return FALSE;
	byte_index = (guint) char_index / 3;

	if (byte_index >= self->priv->display_length)
		text = g_strdup (_("Rest of data not shown."));
	else
		text = g_strdup_printf (_("Address: %02X"), byte_index);
	gtk_tooltip_set_text (tooltip, text);
	g_free (text);

	return TRUE;
}

static void
ensure_layout (MCUSByteArray *self)
{
	MCUSByteArrayPrivate *priv = self->priv;
	GtkStyle *style;

	if (priv->layout != NULL)
		return;

	/* Create the layout */
	priv->layout = gtk_widget_create_pango_layout (GTK_WIDGET (self), priv->text);

	/* Set the font description */
	style = gtk_widget_get_style (GTK_WIDGET (self));
	priv->font_desc = pango_font_description_copy (style->font_desc);
	pango_font_description_set_family_static (priv->font_desc, "monospace");
	pango_layout_set_font_description (priv->layout, priv->font_desc);

	/* Set various properties */
	pango_layout_set_alignment (priv->layout,
	                            (gtk_widget_get_direction (GTK_WIDGET (self)) == GTK_TEXT_DIR_RTL) ? PANGO_ALIGN_RIGHT : PANGO_ALIGN_LEFT);
	pango_layout_set_ellipsize (priv->layout, PANGO_ELLIPSIZE_NONE);
	pango_layout_set_single_paragraph_mode (priv->layout, FALSE);
	pango_layout_set_width (priv->layout, -1);

	/* Create the attributes */
	priv->attr_list = pango_attr_list_new ();
	priv->highlight_attr = pango_attr_weight_new (PANGO_WEIGHT_BOLD);
	pango_attr_list_insert (priv->attr_list, priv->highlight_attr); /* @attr_list assumes ownership of @highlight_attr */
}

/* Get the width of the array in Pango units */
static gint
get_width_pu (MCUSByteArray *self)
{
	MCUSByteArrayPrivate *priv = self->priv;
	PangoContext *context;
	PangoFontMetrics *metrics;
	gint digit_width;

	context = pango_layout_get_context (priv->layout);
	metrics = pango_context_get_metrics (context, priv->font_desc, pango_context_get_language (context));
	digit_width = pango_font_metrics_get_approximate_digit_width (metrics);
	pango_font_metrics_unref (metrics);

	return digit_width * priv->width_chars;
}

static void
get_layout_location (MCUSByteArray *self, gint *xp, gint *yp)
{
	MCUSByteArrayPrivate *priv = self->priv;
	gfloat xalign, yalign;
	gint required_width, x, y, xpad, ypad;
	PangoRectangle logical;
	GtkAllocation allocation;
	GtkRequisition requisition;

	gtk_misc_get_alignment (GTK_MISC (self), &xalign, &yalign);
	gtk_misc_get_padding (GTK_MISC (self), &xpad, &ypad);
	gtk_widget_get_allocation (GTK_WIDGET (self), &allocation);
	gtk_widget_get_child_requisition (GTK_WIDGET (self), &requisition);
	pango_layout_get_pixel_extents (priv->layout, NULL, &logical);

	/* X coordinate */
	if (gtk_widget_get_direction (GTK_WIDGET (self)) != GTK_TEXT_DIR_LTR)
		xalign = 1.0 - xalign;

	required_width = MIN (PANGO_PIXELS (pango_layout_get_width (priv->layout)), logical.width) + 2 * xpad;
	x = floor (allocation.x + (gint) xpad + xalign * (allocation.width - required_width));

	if (gtk_widget_get_direction (GTK_WIDGET (self)) == GTK_TEXT_DIR_LTR)
		x = MAX (x, allocation.x + xpad);
	else
		x = MIN (x, allocation.x + allocation.width - xpad);
	x -= logical.x;

	/* Y coordinate */
	y = floor (allocation.y + (gint) ypad + MAX (((allocation.height - requisition.height) * yalign), 0));

	/* Return the values */
	if (xp)
		*xp = x;
	if (yp)
		*yp = y;
}

MCUSByteArray *
mcus_byte_array_new (const guchar *array, guint array_length)
{
	return g_object_new (MCUS_TYPE_BYTE_ARRAY,
	                     "array", array,
	                     "array-length", array_length,
	                     NULL);
}

void
mcus_byte_array_update (MCUSByteArray *self)
{
	guint text_length, i;
	gchar *f;
	MCUSByteArrayPrivate *priv = self->priv;

	/* Free everything if there's nothing to display */
	if (priv->display_length == 0 || priv->array == NULL) {
		g_free (priv->text);
		priv->text = g_strdup ("");
		goto update_layout;
	}

	/* Update the text; two hex characters per byte, plus a space, newline or nul */
	text_length = priv->display_length * 3;

	/* Add a extra character (three bytes) for ellipsisation */
	if (priv->display_length < priv->array_length)
		text_length += 3;

	/* Allocate storage */
	g_free (priv->text);
	priv->text = g_malloc (sizeof (gchar) * text_length);

	/* Set the text */
	f = priv->text;
	for (i = 0; i < MIN (priv->display_length, priv->array_length); i++) {
		g_sprintf (f, "%02X", priv->array[i]);
		*(f + 2) = ' ';
		f += 3;

		if (G_UNLIKELY (i % MAX_WIDTH_BYTES == MAX_WIDTH_BYTES - 1))
			*(f - 1) = '\n';
	}

	/* Ellipsise or terminate */
	if (priv->display_length < priv->array_length)
		g_sprintf (f, "…"); /* adds \0 automatically */
	else
		*(f - 1) = '\0';

update_layout:
	/* Update the layout */
	ensure_layout (self);
	pango_layout_set_text (priv->layout, priv->text, -1);

	gtk_widget_queue_draw (GTK_WIDGET (self));
}

const guchar *
mcus_byte_array_get_array (MCUSByteArray *self, guint *array_length)
{
	g_return_val_if_fail (MCUS_IS_BYTE_ARRAY (self), NULL);

	if (array_length != NULL)
		*array_length = self->priv->array_length;
	return self->priv->array;
}

void
mcus_byte_array_set_array (MCUSByteArray *self, const guchar *array, guint array_length)
{
	g_return_if_fail (MCUS_IS_BYTE_ARRAY (self));
	g_return_if_fail (array_length >= 1);

	self->priv->array = array;
	self->priv->array_length = array_length;

	mcus_byte_array_update (self);
	gtk_widget_queue_resize (GTK_WIDGET (self));

	g_object_freeze_notify (G_OBJECT (self));
	g_object_notify (G_OBJECT (self), "array");
	g_object_notify (G_OBJECT (self), "array-length");
	g_object_thaw_notify (G_OBJECT (self));
}

guint
mcus_byte_array_get_display_length (MCUSByteArray *self)
{
	g_return_val_if_fail (MCUS_IS_BYTE_ARRAY (self), 0);
	return self->priv->display_length;
}

void
mcus_byte_array_set_display_length (MCUSByteArray *self, guint display_length)
{
	MCUSByteArrayPrivate *priv = self->priv;

	g_return_if_fail (MCUS_IS_BYTE_ARRAY (self));

	/* Set the display length */
	priv->display_length = display_length;

	/* Update various cached values; two characters per byte, plus spaces between them all */
	priv->width_chars = MIN (priv->display_length, MAX_WIDTH_BYTES) * 3 - 1;

	/* If the data's being ellipsised to less than one line, ensure the ellipsis stays on that line */
	if (priv->display_length < MAX_WIDTH_BYTES && priv->display_length < priv->array_length)
		priv->width_chars = priv->display_length * 3 + 1;

	mcus_byte_array_update (self);
	gtk_widget_queue_resize (GTK_WIDGET (self));

	g_object_notify (G_OBJECT (self), "display-length");
}

gint
mcus_byte_array_get_highlight_byte (MCUSByteArray *self)
{
	g_return_val_if_fail (MCUS_IS_BYTE_ARRAY (self), -1);
	return self->priv->highlight_byte;
}

void
mcus_byte_array_set_highlight_byte (MCUSByteArray *self, gint highlight_byte)
{
	MCUSByteArrayPrivate *priv = self->priv;

	g_return_if_fail (MCUS_IS_BYTE_ARRAY (self));
	g_return_if_fail (highlight_byte >= -1);

	ensure_layout (self);

	priv->highlight_byte = highlight_byte;

	/* Update the Pango attribute which actually highlights the byte */
	if (highlight_byte == -1) {
		pango_layout_set_attributes (priv->layout, NULL);
	} else {
		priv->highlight_attr->start_index = highlight_byte * 3;
		priv->highlight_attr->end_index = highlight_byte * 3 + 2;
		pango_layout_set_attributes (priv->layout, priv->attr_list);
	}

	gtk_widget_queue_draw (GTK_WIDGET (self));

	g_object_notify (G_OBJECT (self), "highlight-byte");
}
