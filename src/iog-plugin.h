/* Eye Of Gde2 - IOG Plugin
 *
 * Copyright (C) 2007 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 *
 * Based on gedit code (gedit/gedit-module.c) by:
 * 	- Paolo Maggi <paolo@gnome.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef __IOG_PLUGIN_H__
#define __IOG_PLUGIN_H__

#include "iog-window.h"
#include "iog-debug.h"

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _IogPlugin IogPlugin;
typedef struct _IogPluginClass IogPluginClass;
typedef struct _IogPluginPrivate IogPluginPrivate;

#define IOG_TYPE_PLUGIN            (iog_plugin_get_type())
#define IOG_PLUGIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), IOG_TYPE_PLUGIN, IogPlugin))
#define IOG_PLUGIN_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST((obj), IOG_TYPE_PLUGIN, IogPlugin const))
#define IOG_PLUGIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  IOG_TYPE_PLUGIN, IogPluginClass))
#define IOG_IS_PLUGIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), IOG_TYPE_PLUGIN))
#define IOG_IS_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), IOG_TYPE_PLUGIN))
#define IOG_PLUGIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  IOG_TYPE_PLUGIN, IogPluginClass))

struct _IogPlugin {
	GObject parent;
};

struct _IogPluginClass  {
	GObjectClass parent_class;

	void 		(*activate)		(IogPlugin *plugin,
						 IogWindow *window);

	void 		(*deactivate)		(IogPlugin *plugin,
						 IogWindow *window);

	void 		(*update_ui)		(IogPlugin *plugin,
						 IogWindow *window);

	GtkWidget 	*(*create_configure_dialog)
						(IogPlugin *plugin);

	/* Plugins should not override this, it's handled automatically
 	 * by the IogPluginClass */
	gboolean 	(*is_configurable)
						(IogPlugin *plugin);

	/* Padding for future expansion */
	void		(*_iog_reserved1)	(void);
	void		(*_iog_reserved2)	(void);
	void		(*_iog_reserved3)	(void);
	void		(*_iog_reserved4)	(void);
};

GType 		 iog_plugin_get_type 		(void) G_GNUC_CONST;

void 		 iog_plugin_activate		(IogPlugin *plugin,
						 IogWindow *window);

void 		 iog_plugin_deactivate	        (IogPlugin *plugin,
						 IogWindow *window);

void 		 iog_plugin_update_ui		(IogPlugin *plugin,
						 IogWindow *window);

gboolean	 iog_plugin_is_configurable	(IogPlugin *plugin);

GtkWidget	*iog_plugin_create_configure_dialog
						(IogPlugin *plugin);

/*
 * Utility macro used to register plugins
 *
 * use: IOG_PLUGIN_REGISTER_TYPE_WITH_CODE(PluginName, plugin_name, CODE)
 */
#define IOG_PLUGIN_REGISTER_TYPE_WITH_CODE(PluginName, plugin_name, CODE)	\
										\
static GType plugin_name##_type = 0;						\
										\
GType										\
plugin_name##_get_type (void)							\
{										\
	return plugin_name##_type;						\
}										\
										\
static void     plugin_name##_init              (PluginName        *self);	\
static void     plugin_name##_class_init        (PluginName##Class *klass);	\
static gpointer plugin_name##_parent_class = NULL;				\
static void     plugin_name##_class_intern_init (gpointer klass)		\
{										\
	plugin_name##_parent_class = g_type_class_peek_parent (klass);		\
	plugin_name##_class_init ((PluginName##Class *) klass);			\
}										\
										\
G_MODULE_EXPORT GType								\
register_iog_plugin (GTypeModule *module)					\
{										\
	static const GTypeInfo our_info =					\
	{									\
		sizeof (PluginName##Class),					\
		NULL, /* base_init */						\
		NULL, /* base_finalize */					\
		(GClassInitFunc) plugin_name##_class_intern_init,		\
		NULL,								\
		NULL, /* class_data */						\
		sizeof (PluginName),						\
		0, /* n_preallocs */						\
		(GInstanceInitFunc) plugin_name##_init				\
	};									\
										\
	iog_debug_message (DEBUG_PLUGINS, "Registering " #PluginName);	\
										\
	/* Initialise the i18n stuff */						\
	bindtextdomain (GETTEXT_PACKAGE, IOG_LOCALEDIR);			\
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");			\
										\
	plugin_name##_type = g_type_module_register_type (module,		\
					    IOG_TYPE_PLUGIN,			\
					    #PluginName,			\
					    &our_info,				\
					    0);					\
										\
	CODE									\
										\
	return plugin_name##_type;						\
}

/*
 * Utility macro used to register plugins
 *
 * use: IOG_PLUGIN_REGISTER_TYPE(PluginName, plugin_name)
 */
#define IOG_PLUGIN_REGISTER_TYPE(PluginName, plugin_name)			\
	IOG_PLUGIN_REGISTER_TYPE_WITH_CODE(PluginName, plugin_name, ;)

/*
 * Utility macro used to register gobject types in plugins with additional code
 *
 * use: IOG_PLUGIN_DEFINE_TYPE_WITH_CODE(ObjectName, object_name, PARENT_TYPE, CODE)
 */
#define IOG_PLUGIN_DEFINE_TYPE_WITH_CODE(ObjectName, object_name, PARENT_TYPE, CODE)	\
										\
static GType g_define_type_id = 0;						\
										\
GType										\
object_name##_get_type (void)							\
{										\
	return g_define_type_id;						\
}										\
										\
static void     object_name##_init              (ObjectName        *self);	\
static void     object_name##_class_init        (ObjectName##Class *klass);	\
static gpointer object_name##_parent_class = NULL;				\
static void     object_name##_class_intern_init (gpointer klass)		\
{										\
	object_name##_parent_class = g_type_class_peek_parent (klass);		\
	object_name##_class_init ((ObjectName##Class *) klass);			\
}										\
										\
GType										\
object_name##_register_type (GTypeModule *module)				\
{										\
	static const GTypeInfo our_info =					\
	{									\
		sizeof (ObjectName##Class),					\
		NULL, /* base_init */						\
		NULL, /* base_finalize */					\
		(GClassInitFunc) object_name##_class_intern_init,		\
		NULL,								\
		NULL, /* class_data */						\
		sizeof (ObjectName),						\
		0, /* n_preallocs */						\
		(GInstanceInitFunc) object_name##_init				\
	};									\
										\
	iog_debug_message (DEBUG_PLUGINS, "Registering " #ObjectName);	\
										\
	g_define_type_id = g_type_module_register_type (module,			\
					   	        PARENT_TYPE,		\
					                #ObjectName,		\
					                &our_info,		\
					                0);			\
										\
	CODE									\
										\
	return g_define_type_id;						\
}

/*
 * Utility macro used to register gobject types in plugins
 *
 * use: IOG_PLUGIN_DEFINE_TYPE(ObjectName, object_name, PARENT_TYPE)
 */
#define IOG_PLUGIN_DEFINE_TYPE(ObjectName, object_name, PARENT_TYPE)		\
	IOG_PLUGIN_DEFINE_TYPE_WITH_CODE(ObjectName, object_name, PARENT_TYPE, ;)

G_END_DECLS

#endif  /* __IOG_PLUGIN_H__ */
