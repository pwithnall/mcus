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

#ifndef MCUS_INPUT_PORT_H
#define MCUS_INPUT_PORT_H

G_BEGIN_DECLS

#define MCUS_IO_ERROR (mcus_io_error_quark ())
GQuark mcus_io_error_quark (void) G_GNUC_CONST;

enum {
	MCUS_IO_ERROR_INPUT
};

gboolean mcus_input_port_read_entry (GError **error);
void mcus_input_port_update_entry (void);
void mcus_input_port_read_check_buttons (void);
void mcus_input_port_update_check_buttons (void);

G_END_DECLS

#endif /* !MCUS_INPUT_PORT_H */
