/* Eye Of Gde2 - Main Window
 *
 * Copyright (C) 2007 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 *
 * Based on gedit code (gedit/gedit-module.c) by:
 * 	- Paolo Maggi <paolo@gnome.org>
 *      - Marco Pesenti Gritti <marco@gnome.org>
 *      - Christian Persch <chpe@gnome.org>
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

#ifndef IOG_MODULE_H
#define IOG_MODULE_H

#include <glib-object.h>
#include <gmodule.h>

G_BEGIN_DECLS

typedef struct _IogModule IogModule;
typedef struct _IogModuleClass IogModuleClass;
typedef struct _IogModulePrivate IogModulePrivate;

#define IOG_TYPE_MODULE            (iog_module_get_type ())
#define IOG_MODULE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), IOG_TYPE_MODULE, IogModule))
#define IOG_MODULE_CLASS(klass)	   (G_TYPE_CHECK_CLASS_CAST ((klass),  IOG_TYPE_MODULE, IogModuleClass))
#define IOG_IS_MODULE(obj)	   (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IOG_TYPE_MODULE))
#define IOG_IS_MODULE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj),    IOG_TYPE_MODULE))
#define IOG_MODULE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),   IOG_TYPE_MODULE, IogModuleClass))

struct _IogModule {
	GTypeModule parent_instance;

	GModule *library;
	gchar   *path;
	GType    type;
};

struct _IogModuleClass {
	GTypeModuleClass parent_class;
};

G_GNUC_INTERNAL
GType		 iog_module_get_type	(void) G_GNUC_CONST;

G_GNUC_INTERNAL
IogModule	*iog_module_new		(const gchar *path);

G_GNUC_INTERNAL
const gchar	*iog_module_get_path	(IogModule *module);

G_GNUC_INTERNAL
GObject		*iog_module_new_object	(IogModule *module);

G_END_DECLS

#endif
