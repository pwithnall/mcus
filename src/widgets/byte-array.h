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

#ifndef MCUS_BYTE_ARRAY_H
#define MCUS_BYTE_ARRAY_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MCUS_TYPE_BYTE_ARRAY		(mcus_byte_array_get_type ())
#define MCUS_BYTE_ARRAY(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), MCUS_TYPE_BYTE_ARRAY, MCUSByteArray))
#define MCUS_BYTE_ARRAY_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), MCUS_TYPE_BYTE_ARRAY, MCUSByteArrayClass))
#define MCUS_IS_BYTE_ARRAY(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), MCUS_TYPE_BYTE_ARRAY))
#define MCUS_IS_BYTE_ARRAY_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), MCUS_TYPE_BYTE_ARRAY))
#define MCUS_BYTE_ARRAY_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), MCUS_TYPE_BYTE_ARRAY, MCUSByteArrayClass))

typedef struct _MCUSByteArrayPrivate	MCUSByteArrayPrivate;

typedef struct {
	GtkMisc parent;
	MCUSByteArrayPrivate *priv;
} MCUSByteArray;

typedef struct {
	GtkMiscClass parent;
} MCUSByteArrayClass;

G_MODULE_EXPORT GType mcus_byte_array_get_type (void) G_GNUC_CONST;
MCUSByteArray *mcus_byte_array_new (const guchar *array, guint array_length) G_GNUC_WARN_UNUSED_RESULT;

void mcus_byte_array_update (MCUSByteArray *self);

const guchar *mcus_byte_array_get_array (MCUSByteArray *self, guint *array_length);
void mcus_byte_array_set_array (MCUSByteArray *self, const guchar *array, guint array_length);
guint mcus_byte_array_get_display_length (MCUSByteArray *self);
void mcus_byte_array_set_display_length (MCUSByteArray *self, guint display_length);
gint mcus_byte_array_get_highlight_byte (MCUSByteArray *self);
void mcus_byte_array_set_highlight_byte (MCUSByteArray *self, gint highlight_byte);

G_END_DECLS

#endif /* !MCUS_BYTE_ARRAY_H */
