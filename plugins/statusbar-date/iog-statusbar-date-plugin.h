/* Statusbar Date -- Shows the EXIF date in IOG's statusbar
 *
 * Copyright (C) 2008 The Free Software Foundation
 *
 * Author: Claudio Saavedra  <csaavedra@gnome.org>
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

#ifndef __IOG_STATUSBAR_DATE_PLUGIN_H__
#define __IOG_STATUSBAR_DATE_PLUGIN_H__

#include <glib.h>
#include <glib-object.h>
#include <iog-plugin.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define IOG_TYPE_STATUSBAR_DATE_PLUGIN \
	(iog_statusbar_date_plugin_get_type())
#define IOG_STATUSBAR_DATE_PLUGIN(o) \
	(G_TYPE_CHECK_INSTANCE_CAST((o), IOG_TYPE_STATUSBAR_DATE_PLUGIN, IogStatusbarDatePlugin))
#define IOG_STATUSBAR_DATE_PLUGIN_CLASS(k) \
	G_TYPE_CHECK_CLASS_CAST((k), IOG_TYPE_STATUSBAR_DATE_PLUGIN, IogStatusbarDatePluginClass))
#define IOG_IS_STATUSBAR_DATE_PLUGIN(o) \
	(G_TYPE_CHECK_INSTANCE_TYPE((o), IOG_TYPE_STATUSBAR_DATE_PLUGIN))
#define IOG_IS_STATUSBAR_DATE_PLUGIN_CLASS(k) \
	(G_TYPE_CHECK_CLASS_TYPE((k), IOG_TYPE_STATUSBAR_DATE_PLUGIN))
#define IOG_STATUSBAR_DATE_PLUGIN_GET_CLASS(o) \
	(G_TYPE_INSTANCE_GET_CLASS((o), IOG_TYPE_STATUSBAR_DATE_PLUGIN, IogStatusbarDatePluginClass))

/* Private structure type */
typedef struct _IogStatusbarDatePluginPrivate IogStatusbarDatePluginPrivate;

/*
 * Main object structure
 */
typedef struct _IogStatusbarDatePlugin IogStatusbarDatePlugin;

struct _IogStatusbarDatePlugin {
	IogPlugin parent_instance;
};

/*
 * Class definition
 */
typedef struct _IogStatusbarDatePluginClass	IogStatusbarDatePluginClass;

struct _IogStatusbarDatePluginClass {
	IogPluginClass parent_class;
};

/*
 * Public methods
 */
GType iog_statusbar_date_plugin_get_type(void) G_GNUC_CONST;

/* All the plugins must implement this function */
G_MODULE_EXPORT GType register_iog_plugin(GTypeModule* module);

G_END_DECLS

#endif /* __IOG_STATUSBAR_DATE_PLUGIN_H__ */
