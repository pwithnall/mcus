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

#ifndef MCUS_LED_H
#define MCUS_LED_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MCUS_TYPE_LED		(mcus_led_get_type ())
#define MCUS_LED(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), MCUS_TYPE_LED, MCUSLED))
#define MCUS_LED_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), MCUS_TYPE_LED, MCUSLEDClass))
#define MCUS_IS_LED(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), MCUS_TYPE_LED))
#define MCUS_IS_LED_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), MCUS_TYPE_LED))
#define MCUS_LED_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), MCUS_TYPE_LED, MCUSLEDClass))

typedef struct _MCUSLEDPrivate	MCUSLEDPrivate;

typedef struct {
	GtkWidget parent;
	MCUSLEDPrivate *priv;
} MCUSLED;

typedef struct {
	GtkWidgetClass parent;
} MCUSLEDClass;

G_MODULE_EXPORT GType mcus_led_get_type (void);
MCUSLED *mcus_led_new (void);

gboolean mcus_led_get_enabled (MCUSLED *self);
void mcus_led_set_enabled (MCUSLED *self, gboolean enabled);

G_END_DECLS

#endif /* !MCUS_LED_H */
