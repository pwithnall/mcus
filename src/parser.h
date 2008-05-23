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

#ifndef MCUS_PARSER_H
#define MCUS_PARSER_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define MCUS_TYPE_PARSER		(mcus_parser_get_type ())
#define MCUS_PARSER(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), MCUS_TYPE_PARSER, MCUSParser))
#define MCUS_PARSER_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), MCUS_TYPE_PARSER, MCUSParserClass))
#define MCUS_IS_PARSER(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), MCUS_TYPE_PARSER))
#define MCUS_IS_PARSER_CLASS(k)		(G_TYPE_CHECK_CLASS_TYPE ((k), MCUS_TYPE_PARSER))
#define MCUS_PARSER_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), MCUS_TYPE_PARSER, MCUSParserClass))

#define MCUS_PARSER_ERROR		(mcus_parser_error_quark ())
#define PARSER_ERROR_CONTEXT_LENGTH	4

enum {
	MCUS_PARSER_ERROR_INVALID_LABEL,
	MCUS_PARSER_ERROR_INVALID_INSTRUCTION,
	MCUS_PARSER_ERROR_INVALID_OPERAND,
	MCUS_PARSER_ERROR_INVALID_OPERAND_TYPE,
	MCUS_PARSER_ERROR_UNRESOLVABLE_LABEL,
	MCUS_PARSER_ERROR_MEMORY_OVERFLOW
};

typedef struct _MCUSParserPrivate	MCUSParserPrivate;

typedef struct {
	GObject parent;
	MCUSParserPrivate *priv;
} MCUSParser;

typedef struct {
	GObjectClass parent;
} MCUSParserClass;

GType mcus_parser_get_type (void);
GQuark mcus_parser_error_quark (void);

MCUSParser *mcus_parser_new (void);
gboolean mcus_parser_parse (MCUSParser *self, const gchar *code, GError **error);
gboolean mcus_parser_compile (MCUSParser *self, GError **error);
guint mcus_parser_get_offset (MCUSParser *self);

G_END_DECLS

#endif /* !MCUS_PARSER_H */
