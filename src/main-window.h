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

#ifndef MCUS_MAIN_WINDOW_H
#define MCUS_MAIN_WINDOW_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

enum {
	MCUS_IO_ERROR_INPUT
};

GQuark mcus_io_error_quark (void) G_GNUC_CONST;
#define MCUS_IO_ERROR (mcus_io_error_quark ())

#define MCUS_TYPE_MAIN_WINDOW		(mcus_main_window_get_type ())
#define MCUS_MAIN_WINDOW(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), MCUS_TYPE_MAIN_WINDOW, MCUSMainWindow))
#define MCUS_MAIN_WINDOW_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), MCUS_TYPE_MAIN_WINDOW, MCUSMainWindowClass))
#define MCUS_IS_MAIN_WINDOW(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), MCUS_TYPE_MAIN_WINDOW))
#define MCUS_IS_MAIN_WINDOW_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), MCUS_TYPE_MAIN_WINDOW))
#define MCUS_MAIN_WINDOW_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), MCUS_TYPE_MAIN_WINDOW, MCUSMainWindowClass))

typedef struct _MCUSMainWindowPrivate	MCUSMainWindowPrivate;

typedef struct {
	GtkWindow parent;
	MCUSMainWindowPrivate *priv;
} MCUSMainWindow;

typedef struct {
	GtkWindowClass parent;
} MCUSMainWindowClass;

G_MODULE_EXPORT GType mcus_main_window_get_type (void) G_GNUC_CONST;

GtkWindow *mcus_main_window_new (void) G_GNUC_WARN_UNUSED_RESULT;

void mcus_main_window_new_program (MCUSMainWindow *self);
void mcus_main_window_open_program (MCUSMainWindow *self);
void mcus_main_window_save_program (MCUSMainWindow *self);
gboolean mcus_main_window_save_program_as (MCUSMainWindow *self);
void mcus_main_window_open_file (MCUSMainWindow *self, const gchar *filename);
gboolean mcus_main_window_quit (MCUSMainWindow *self);

G_END_DECLS

#endif /* !MCUS_MAIN_WINDOW_H */
