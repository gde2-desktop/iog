/* Eye Of Gde2 - Python Module
 *
 * Copyright (C) 2007 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 *
 * Based on gedit code (gedit/gedit-python-module.h) by:
 * 	- Raphael Slinckx <raphael@slinckx.net>
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

#ifndef __IOG_PYTHON_MODULE_H__
#define __IOG_PYTHON_MODULE_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define IOG_TYPE_PYTHON_MODULE		  (iog_python_module_get_type ())
#define IOG_PYTHON_MODULE(obj)		  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IOG_TYPE_PYTHON_MODULE, IogPythonModule))
#define IOG_PYTHON_MODULE_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), IOG_TYPE_PYTHON_MODULE, IogPythonModuleClass))
#define IOG_IS_PYTHON_MODULE(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IOG_TYPE_PYTHON_MODULE))
#define IOG_IS_PYTHON_MODULE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), IOG_TYPE_PYTHON_MODULE))
#define IOG_PYTHON_MODULE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), IOG_TYPE_PYTHON_MODULE, IogPythonModuleClass))

typedef struct _IogPythonModule	IogPythonModule;
typedef struct _IogPythonModuleClass IogPythonModuleClass;
typedef struct _IogPythonModulePrivate IogPythonModulePrivate;

struct _IogPythonModuleClass {
	GTypeModuleClass parent_class;
};

struct _IogPythonModule {
	GTypeModule parent_instance;
};

G_GNUC_INTERNAL
GType			 iog_python_module_get_type		(void) G_GNUC_CONST;

G_GNUC_INTERNAL
IogPythonModule		*iog_python_module_new			(const gchar* path,
								 const gchar *module);

G_GNUC_INTERNAL
GObject			*iog_python_module_new_object		(IogPythonModule *module);

G_GNUC_INTERNAL
gboolean		iog_python_init				(void);

G_GNUC_INTERNAL
void			iog_python_shutdown			(void);

G_GNUC_INTERNAL
void			iog_python_garbage_collect		(void);

G_END_DECLS

#endif /* __IOG_PYTHON_MODULE_H__ */
