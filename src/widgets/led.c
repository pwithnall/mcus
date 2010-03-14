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
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <atk/atk.h>
#include <math.h>
#include <string.h>

#include "led.h"

#define MINIMUM_SIZE 30

static void mcus_led_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void mcus_led_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void mcus_led_realize (GtkWidget *widget);
static void mcus_led_size_request (GtkWidget *widget, GtkRequisition *requisition);
static void mcus_led_size_allocate (GtkWidget *widget, GtkAllocation *allocation);
static gint mcus_led_expose_event (GtkWidget *widget, GdkEventExpose *event);
static AtkObject *mcus_led_get_accessible (GtkWidget *widget);
static GType mcus_led_accessible_get_type (void) G_GNUC_CONST;

struct _MCUSLEDPrivate {
	gboolean enabled;
	gdouble render_size;
};

enum {
	PROP_ENABLED = 1
};

G_DEFINE_TYPE (MCUSLED, mcus_led, GTK_TYPE_WIDGET)

MCUSLED *
mcus_led_new (void)
{
	return g_object_new (MCUS_TYPE_LED, NULL);
}

static void
mcus_led_class_init (MCUSLEDClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	g_type_class_add_private (klass, sizeof (MCUSLEDPrivate));

	gobject_class->get_property = mcus_led_get_property;
	gobject_class->set_property = mcus_led_set_property;

	widget_class->realize = mcus_led_realize;
	widget_class->size_request = mcus_led_size_request;
	widget_class->size_allocate = mcus_led_size_allocate;
	widget_class->expose_event = mcus_led_expose_event;
	widget_class->get_accessible = mcus_led_get_accessible;

	g_object_class_install_property (gobject_class, PROP_ENABLED,
				g_param_spec_boolean ("enabled",
					"Enabled", "Whether the LED is enabled.",
					FALSE,
					G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
mcus_led_init (MCUSLED *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, MCUS_TYPE_LED, MCUSLEDPrivate);
}

static void
mcus_led_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	MCUSLEDPrivate *priv = MCUS_LED (object)->priv;

	switch (property_id) {
		case PROP_ENABLED:
			g_value_set_boolean (value, priv->enabled);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
mcus_led_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_ENABLED:
			mcus_led_set_enabled (MCUS_LED (object), g_value_get_boolean (value));
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
mcus_led_realize (GtkWidget *widget)
{
	MCUSLED *self;
	GdkWindowAttr attributes;
	gint attr_mask;

	g_return_if_fail (MCUS_IS_LED (widget));

	/* Mark the widget as realised */
	GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
	self = MCUS_LED (widget);

	/* Create the attribute object */
	attributes.x = widget->allocation.x;
	attributes.y = widget->allocation.y;
	attributes.width = widget->allocation.width;
	attributes.height = widget->allocation.height;
	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.event_mask = GDK_EXPOSURE_MASK | GDK_STRUCTURE_MASK | GDK_PROPERTY_CHANGE_MASK | GDK_VISIBILITY_NOTIFY_MASK;
	attributes.visual = gtk_widget_get_visual (widget);
	attributes.colormap = gtk_widget_get_colormap (widget);

	/* Create a new GdkWindow for the widget */
	attr_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
	widget->window = gdk_window_new (widget->parent->window, &attributes, attr_mask);
	gdk_window_set_user_data (widget->window, self);

	/* Attach a style to the window and draw a background colour */
	widget->style = gtk_style_attach (widget->style, widget->window);
	gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);
	gdk_window_show (widget->window);
}

static void
mcus_led_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
	g_return_if_fail (requisition != NULL);
	g_return_if_fail (MCUS_IS_LED (widget));

	requisition->width = MINIMUM_SIZE;
	requisition->height = MINIMUM_SIZE;
}

static void
mcus_led_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	MCUSLEDPrivate *priv;

	g_return_if_fail (allocation != NULL);
	g_return_if_fail (MCUS_IS_LED (widget));

	widget->allocation = *allocation;
	priv = MCUS_LED (widget)->priv;

	/* Sort out sizes, ratios, etc. */
	if (allocation->height < allocation->width)
		priv->render_size = allocation->height;
	else
		priv->render_size = allocation->width;

	/* Ensure the borders aren't clipped */
	priv->render_size -= widget->style->xthickness * 2.0;

	if (GTK_WIDGET_REALIZED (widget))
		gdk_window_move_resize (widget->window, allocation->x, allocation->y, allocation->width, allocation->height);
}

static gint
mcus_led_expose_event (GtkWidget *widget, GdkEventExpose *event)
{
	cairo_t *cr;
	MCUSLEDPrivate *priv;
	GdkColor fill, stroke;

	g_return_val_if_fail (event != NULL, FALSE);
	g_return_val_if_fail (MCUS_IS_LED (widget), FALSE);

	/* Compress exposes */
	if (event->count > 0)
		return TRUE;

	priv = MCUS_LED (widget)->priv;

	/* Clear the area first */
	gdk_window_clear_area (widget->window, event->area.x, event->area.y, event->area.width, event->area.height);

	/* Prepare our custom colours */
	fill.red = 29555; /* Tango's medium "chameleon" --- 73d216 */
	fill.green = 53970;
	fill.blue = 5654;
	stroke.red = 34952; /* Tango's lightest "aluminium" --- 888a85 */
	stroke.green = 35466;
	stroke.blue = 34181;

	/* Draw! */
	cr = gdk_cairo_create (widget->window);

	/* Clip to the exposed area */
	cairo_rectangle (cr, event->area.x, event->area.y, event->area.width, event->area.height);
	cairo_clip (cr);

	cairo_set_line_width (cr, widget->style->xthickness);

	/* Draw the LED */
	cairo_arc (cr,
		   widget->allocation.width / 2.0,
		   widget->allocation.height / 2.0,
		   priv->render_size / 2.0,
		   0, 2 * M_PI);
	gdk_cairo_set_source_color (cr, priv->enabled ? &fill : &stroke);
	cairo_fill_preserve (cr);
	gdk_cairo_set_source_color (cr, &stroke);
	cairo_stroke (cr);

	cairo_destroy (cr);

	return TRUE;
}

gboolean
mcus_led_get_enabled (MCUSLED *self)
{
	g_return_val_if_fail (MCUS_IS_LED (self), -1);

	return self->priv->enabled;
}

void
mcus_led_set_enabled (MCUSLED *self, gboolean enabled)
{
	g_return_if_fail (MCUS_IS_LED (self));

	self->priv->enabled = enabled ? TRUE : FALSE;

	/* Ensure we're redrawn */
	gtk_widget_queue_draw (GTK_WIDGET (self));
}

/* Accessibility stuff */
static AtkObject *
mcus_led_accessible_new (GObject *object)
{
	AtkObject *accessible;

	g_return_val_if_fail (GTK_IS_WIDGET (object), NULL);

	accessible = g_object_new (mcus_led_accessible_get_type (), NULL);
	atk_object_initialize (accessible, object);

	return accessible;
}

static void
mcus_led_accessible_factory_class_init (AtkObjectFactoryClass *klass)
{
	klass->create_accessible = mcus_led_accessible_new;
	klass->get_accessible_type = mcus_led_accessible_get_type;
}

static GType
mcus_led_accessible_factory_get_type (void)
{
	static GType type = 0;

	if (!type) {
		const GTypeInfo tinfo = {
			sizeof (AtkObjectFactoryClass),
			NULL,           /* base_init */
			NULL,           /* base_finalize */
			(GClassInitFunc) mcus_led_accessible_factory_class_init,
			NULL,           /* class_finalize */
			NULL,           /* class_data */
			sizeof (AtkObjectFactory),
			0,             /* n_preallocs */
			NULL, NULL
		};

		type = g_type_register_static (ATK_TYPE_OBJECT_FACTORY, _("MCUSLEDAccessibleFactory"), &tinfo, 0);
	}

	return type;
}

static AtkObjectClass *a11y_parent_class = NULL;

static void
mcus_led_accessible_initialize (AtkObject *accessible, gpointer widget)
{
	atk_object_set_name (accessible, _("Single LED widget"));
	atk_object_set_description (accessible, _("Provides visual Boolean output"));

	a11y_parent_class->initialize (accessible, widget);
}

static void
mcus_led_accessible_class_init (AtkObjectClass *klass)
{
	a11y_parent_class = g_type_class_peek_parent (klass);
	klass->initialize = mcus_led_accessible_initialize;
}

static void
mcus_led_accessible_image_get_size (AtkImage *image, gint *width, gint *height)
{
	GtkWidget *widget = GTK_ACCESSIBLE (image)->widget;
	if (!widget) {
		*width = *height = 0;
	} else {
		*width = widget->allocation.width;
		*height = widget->allocation.height;
	}
}

static const gchar *
mcus_led_accessible_image_get_description (AtkImage *image)
{
	MCUSLED *self = MCUS_LED (GTK_ACCESSIBLE (image)->widget);
	return self->priv->enabled ? _("LED on") : _("LED off");
}

static void
mcus_led_accessible_image_interface_init (AtkImageIface *iface)
{
	iface->get_image_size = mcus_led_accessible_image_get_size;
	iface->get_image_description = mcus_led_accessible_image_get_description;
}

static GType
mcus_led_accessible_get_type (void)
{
	static GType type = 0;

	/* Action interface
	   Name etc. ... */
	if (G_UNLIKELY (type == 0)) {
		const GInterfaceInfo atk_image_info = {
			(GInterfaceInitFunc) mcus_led_accessible_image_interface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL
		};
		GType parent_atk_type;
		GTypeInfo tinfo = { 0 };
		GTypeQuery query;
		AtkObjectFactory *factory;

		if ((type = g_type_from_name ("MCUSLEDAccessible")))
			return type;

		factory = atk_registry_get_factory (atk_get_default_registry (), GTK_TYPE_IMAGE);
		if (!factory)
			return G_TYPE_INVALID;

		parent_atk_type = atk_object_factory_get_accessible_type (factory);
		if (!parent_atk_type)
			return G_TYPE_INVALID;

		/* Figure out the size of the class and instance we are deriving from */
		g_type_query (parent_atk_type, &query);

		tinfo.class_init = (GClassInitFunc) mcus_led_accessible_class_init;
		tinfo.class_size = query.class_size;
		tinfo.instance_size = query.instance_size;

		/* Register the type */
		type = g_type_register_static (parent_atk_type, "MCUSLEDAccessible", &tinfo, 0);

		g_type_add_interface_static (type, ATK_TYPE_IMAGE, &atk_image_info);
	}

	return type;
}

static AtkObject *
mcus_led_get_accessible (GtkWidget *widget)
{
	static gboolean first_time = TRUE;

	if (first_time) {
		AtkObjectFactory *factory;
		AtkRegistry *registry;
		GType derived_type, derived_atk_type;

		/* Figure out whether accessibility is enabled by looking at the type of the accessible object which would be created for
		 * the parent type of MCUSLED. */
		derived_type = g_type_parent (MCUS_TYPE_LED);

		registry = atk_get_default_registry ();
		factory = atk_registry_get_factory (registry, derived_type);
		derived_atk_type = atk_object_factory_get_accessible_type (factory);
		if (g_type_is_a (derived_atk_type, GTK_TYPE_ACCESSIBLE))
			atk_registry_set_factory_type (registry, MCUS_TYPE_LED, mcus_led_accessible_factory_get_type ());
		first_time = FALSE;
	}

	return GTK_WIDGET_CLASS (mcus_led_parent_class)->get_accessible (widget);
}
