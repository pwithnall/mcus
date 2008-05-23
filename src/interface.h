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

#ifndef MCUS_INTERFACE_H
#define MCUS_INTERFACE_H

G_BEGIN_DECLS

GtkWidget *mcus_create_interface (void);
void mcus_interface_error (const gchar *message, GtkWidget *parent_window);
void mcus_print_debug_data (void);
void mcus_update_ui (void);
void mcus_read_input_port (void);
void mcus_read_analogue_input (void);

G_END_DECLS

#endif /* MCUS_INTERFACE_H */
