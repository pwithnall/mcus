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

#ifndef MCUS_SEVEN_SEGMENT_DISPLAY_H
#define MCUS_SEVEN_SEGMENT_DISPLAY_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef enum {
	SEGMENT_A = 0,
	SEGMENT_B,
	SEGMENT_C,
	SEGMENT_D,
	SEGMENT_E,
	SEGMENT_F,
	SEGMENT_G,
	SEGMENT_POINT,
	MAX_SEGMENTS
} MCUSSevenSegmentDisplaySegment;

#define MCUS_TYPE_SEVEN_SEGMENT_DISPLAY		(mcus_seven_segment_display_get_type ())
#define MCUS_SEVEN_SEGMENT_DISPLAY(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), MCUS_TYPE_SEVEN_SEGMENT_DISPLAY, MCUSSevenSegmentDisplay))
#define MCUS_SEVEN_SEGMENT_DISPLAY_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), MCUS_TYPE_SEVEN_SEGMENT_DISPLAY, MCUSSevenSegmentDisplayClass))
#define MCUS_IS_SEVEN_SEGMENT_DISPLAY(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), MCUS_TYPE_SEVEN_SEGMENT_DISPLAY))
#define MCUS_IS_SEVEN_SEGMENT_DISPLAY_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), MCUS_TYPE_SEVEN_SEGMENT_DISPLAY))
#define MCUS_SEVEN_SEGMENT_DISPLAY_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), MCUS_TYPE_SEVEN_SEGMENT_DISPLAY, MCUSSevenSegmentDisplayClass))

typedef struct _MCUSSevenSegmentDisplayPrivate	MCUSSevenSegmentDisplayPrivate;

typedef struct {
	GtkWidget parent;
	MCUSSevenSegmentDisplayPrivate *priv;
} MCUSSevenSegmentDisplay;

typedef struct {
	GtkWidgetClass parent;
} MCUSSevenSegmentDisplayClass;

G_MODULE_EXPORT GType mcus_seven_segment_display_get_type (void);
MCUSSevenSegmentDisplay *mcus_seven_segment_display_new (void);

guint8 mcus_seven_segment_display_get_segment_mask (MCUSSevenSegmentDisplay *self);
void mcus_seven_segment_display_set_segment_mask (MCUSSevenSegmentDisplay *self, guint8 segment_mask);
gint mcus_seven_segment_display_get_digit (MCUSSevenSegmentDisplay *self);
void mcus_seven_segment_display_set_digit (MCUSSevenSegmentDisplay *self, guint digit);
gboolean mcus_seven_segment_display_get_segment (MCUSSevenSegmentDisplay *self, MCUSSevenSegmentDisplaySegment segment);
void mcus_seven_segment_display_set_segment (MCUSSevenSegmentDisplay *self, MCUSSevenSegmentDisplaySegment segment, gboolean enabled);

G_END_DECLS

#endif /* !MCUS_SEVEN_SEGMENT_DISPLAY_H */
