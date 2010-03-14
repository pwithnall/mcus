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

#ifndef MCUS_SIMULATION_H
#define MCUS_SIMULATION_H

G_BEGIN_DECLS

#define MCUS_SIMULATION_ERROR		(mcus_simulation_error_quark ())

enum {
	MCUS_SIMULATION_ERROR_MEMORY_OVERFLOW,
	MCUS_SIMULATION_ERROR_STACK_OVERFLOW,
	MCUS_SIMULATION_ERROR_STACK_UNDERFLOW,
	MCUS_SIMULATION_ERROR_INVALID_OPCODE
};

GQuark mcus_simulation_error_quark (void) G_GNUC_CONST;

void mcus_simulation_init (void);
gboolean mcus_simulation_iterate (GError **error);
void mcus_simulation_finalise (void);
void mcus_simulation_update_ui (void);

G_END_DECLS

#endif /* !MCUS_SIMULATION_H */
