/* Eye Of Gde2 - IOG Module
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "iog-module.h"
#include "iog-debug.h"

#include <gmodule.h>

#define IOG_MODULE_GET_PRIVATE(object) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((object), IOG_TYPE_MODULE, IogModulePrivate))

G_DEFINE_TYPE (IogModule, iog_module, G_TYPE_TYPE_MODULE);

typedef GType (*IogModuleRegisterFunc) (GTypeModule *);

static gboolean
iog_module_load (GTypeModule *gmodule)
{
	IogModule *module = IOG_MODULE (gmodule);
	IogModuleRegisterFunc register_func;

	iog_debug_message (DEBUG_PLUGINS, "Loading %s", module->path);

	module->library = g_module_open (module->path, 0);

	if (module->library == NULL) {
		g_warning ("%s", g_module_error());

		return FALSE;
	}

	/* Extract symbols from the lib */
	if (!g_module_symbol (module->library,
			      "register_iog_plugin",
			      (void *) &register_func)) {
		g_warning ("%s", g_module_error());
		g_module_close (module->library);

		return FALSE;
	}

	/* Symbol can still be NULL even though g_module_symbol
	 * returned TRUE */
	if (register_func == NULL) {
		g_warning ("Symbol 'register_iog_plugin' should not be NULL");
		g_module_close (module->library);

		return FALSE;
	}

	module->type = register_func (gmodule);

	if (module->type == 0) {
		g_warning ("Invalid iog plugin contained by module %s", module->path);
		return FALSE;
	}

	return TRUE;
}

static void
iog_module_unload (GTypeModule *gmodule)
{
	IogModule *module = IOG_MODULE (gmodule);

	iog_debug_message (DEBUG_PLUGINS, "Unloading %s", module->path);

	g_module_close (module->library);

	module->library = NULL;
	module->type = 0;
}

const gchar *
iog_module_get_path (IogModule *module)
{
	g_return_val_if_fail (IOG_IS_MODULE (module), NULL);

	return module->path;
}

GObject *
iog_module_new_object (IogModule *module)
{
	iog_debug_message (DEBUG_PLUGINS, "Creating object of type %s", g_type_name (module->type));

	if (module->type == 0) {
		return NULL;
	}

	return g_object_new (module->type, NULL);
}

static void
iog_module_init (IogModule *module)
{
	iog_debug_message (DEBUG_PLUGINS, "IogModule %p initialising", module);
}

static void
iog_module_finalize (GObject *object)
{
	IogModule *module = IOG_MODULE (object);

	iog_debug_message (DEBUG_PLUGINS, "IogModule %p finalising", module);

	g_free (module->path);

	G_OBJECT_CLASS (iog_module_parent_class)->finalize (object);
}

static void
iog_module_class_init (IogModuleClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);
	GTypeModuleClass *module_class = G_TYPE_MODULE_CLASS (class);

	object_class->finalize = iog_module_finalize;

	module_class->load = iog_module_load;
	module_class->unload = iog_module_unload;
}

IogModule *
iog_module_new (const gchar *path)
{
	IogModule *module;

	if (path == NULL || path[0] == '\0') {
		return NULL;
	}

	module = g_object_new (IOG_TYPE_MODULE, NULL);

	g_type_module_set_name (G_TYPE_MODULE (module), path);

	module->path = g_strdup (path);

	return module;
}
