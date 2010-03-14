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

#ifndef MCUS_COMPILER_H
#define MCUS_COMPILER_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define MCUS_TYPE_COMPILER		(mcus_compiler_get_type ())
#define MCUS_COMPILER(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), MCUS_TYPE_COMPILER, MCUSCompiler))
#define MCUS_COMPILER_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), MCUS_TYPE_COMPILER, MCUSCompilerClass))
#define MCUS_IS_COMPILER(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), MCUS_TYPE_COMPILER))
#define MCUS_IS_COMPILER_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), MCUS_TYPE_COMPILER))
#define MCUS_COMPILER_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), MCUS_TYPE_COMPILER, MCUSCompilerClass))

#define MCUS_COMPILER_ERROR		(mcus_compiler_error_quark ())

enum {
	MCUS_COMPILER_ERROR_INVALID_LABEL,
	MCUS_COMPILER_ERROR_INVALID_LABEL_DELIMITATION,
	MCUS_COMPILER_ERROR_INVALID_MNEMONIC,
	MCUS_COMPILER_ERROR_INVALID_OPERAND,
	MCUS_COMPILER_ERROR_INVALID_OPERAND_TYPE,
	MCUS_COMPILER_ERROR_UNRESOLVABLE_LABEL,
	MCUS_COMPILER_ERROR_MEMORY_OVERFLOW,
	MCUS_COMPILER_ERROR_INVALID_LOOKUP_TABLE,
	MCUS_COMPILER_ERROR_INVALID_CONSTANT,
	MCUS_COMPILER_ERROR_DUPLICATE_LABEL,
	MCUS_COMPILER_ERROR_DUPLICATE_LOOKUP_TABLE
};

typedef struct _MCUSCompilerPrivate	MCUSCompilerPrivate;

typedef struct {
	GObject parent;
	MCUSCompilerPrivate *priv;
} MCUSCompiler;

typedef struct {
	GObjectClass parent;
} MCUSCompilerClass;

GType mcus_compiler_get_type (void) G_GNUC_CONST;
GQuark mcus_compiler_error_quark (void) G_GNUC_CONST;

MCUSCompiler *mcus_compiler_new (void) G_GNUC_WARN_UNUSED_RESULT;
gboolean mcus_compiler_parse (MCUSCompiler *self, const gchar *code, GError **error);
gboolean mcus_compiler_compile (MCUSCompiler *self, GError **error);
guint mcus_compiler_get_offset (MCUSCompiler *self);
void mcus_compiler_get_error_location (MCUSCompiler *self, guint *start, guint *end);

G_END_DECLS

#endif /* !MCUS_COMPILER_H */
