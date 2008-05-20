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

#include "parser.h"
#include "main.h"

void
mw_about_activate_cb (GtkAction *self, gpointer user_data)
{
	MCUSParser *parser;
	GError *error = NULL;

	mcus_print_debug_data ();

	parser = mcus_parser_new ();
	mcus_parser_parse (parser, "\n\
MOVI S1, 05\n\
MOVI S0, 00\n\
loop:\n\
INC S0\n\
OUT Q, S0\n\
MOV S0, S2\n\
EOR S2, S1\n\
JNZ loop", &error);
	if (error != NULL)
		g_error (error->message);

	mcus_parser_compile (parser, &error);
	if (error != NULL)
		g_error (error->message);

	mcus_print_debug_data ();

	while (mcus->zero_flag == FALSE) {
		mcus_iterate_simulation ();
		mcus_print_debug_data ();
	}
}
