/* Eye Of Gde2 - IOG Plugin Manager
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

#ifndef __IOG_PLUGIN_MANAGER_H__
#define __IOG_PLUGIN_MANAGER_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _IogPluginManager IogPluginManager;
typedef struct _IogPluginManagerClass IogPluginManagerClass;
typedef struct _IogPluginManagerPrivate IogPluginManagerPrivate;

#define IOG_TYPE_PLUGIN_MANAGER              (iog_plugin_manager_get_type())
#define IOG_PLUGIN_MANAGER(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), IOG_TYPE_PLUGIN_MANAGER, IogPluginManager))
#define IOG_PLUGIN_MANAGER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass),  IOG_TYPE_PLUGIN_MANAGER, IogPluginManagerClass))
#define IOG_IS_PLUGIN_MANAGER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), IOG_TYPE_PLUGIN_MANAGER))
#define IOG_IS_PLUGIN_MANAGER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), IOG_TYPE_PLUGIN_MANAGER))
#define IOG_PLUGIN_MANAGER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj),  IOG_TYPE_PLUGIN_MANAGER, IogPluginManagerClass))

struct _IogPluginManager {
	GtkVBox vbox;

	IogPluginManagerPrivate *priv;
};

struct _IogPluginManagerClass {
	GtkVBoxClass parent_class;
};

G_GNUC_INTERNAL
GType		 iog_plugin_manager_get_type	(void) G_GNUC_CONST;

G_GNUC_INTERNAL
GtkWidget	*iog_plugin_manager_new		(void);

G_END_DECLS

#endif  /* __IOG_PLUGIN_MANAGER_H__  */
