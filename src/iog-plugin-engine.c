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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include "iog-plugin-engine.h"
#include "iog-plugin.h"
#include "iog-module.h"
#include "iog-debug.h"
#include "iog-application.h"
#include "iog-config-keys.h"
#include "iog-util.h"

#include <glib/gi18n.h>
#include <glib.h>
#include <gio/gio.h>

#ifdef ENABLE_PYTHON
#include "iog-python-module.h"
#endif

#define USER_IOG_PLUGINS_LOCATION "plugins/"

#define PLUGIN_EXT	".iog-plugin"

typedef enum {
	IOG_PLUGIN_LOADER_C,
	IOG_PLUGIN_LOADER_PY,
} IogPluginLoader;

struct _IogPluginInfo
{
	gchar             *file;

	gchar             *location;
	IogPluginLoader    loader;
	GTypeModule       *module;

	gchar             *name;
	gchar             *desc;
	gchar             *icon_name;
	gchar            **authors;
	gchar             *copyright;
	gchar             *website;

	IogPlugin         *plugin;

	gint               active : 1;

	/* A plugin is unavailable if it is not possible to activate it
	   due to an error loading the plugin module (e.g. for Python plugins
	   when the interpreter has not been correctly initializated) */
	gint               available : 1;
};

static void	iog_plugin_engine_active_plugins_changed (GSettings *settings,
							  gchar *key,
							  gpointer user_data);

static GList *iog_plugins_list = NULL;

static GSettings *iog_plugin_engine_settings = NULL;

static GSList *active_plugins = NULL;

static void
iog_plugin_info_free (IogPluginInfo *info)
{
	if (info->plugin != NULL) {
	       	iog_debug_message (DEBUG_PLUGINS, "Unref plugin %s", info->name);

		g_object_unref (info->plugin);
	}

	g_free (info->file);
	g_free (info->location);
	g_free (info->name);
	g_free (info->desc);
	g_free (info->icon_name);
	g_free (info->website);
	g_free (info->copyright);
	g_strfreev (info->authors);

	g_free (info);
}

static IogPluginInfo *
iog_plugin_engine_load (const gchar *file)
{
	IogPluginInfo *info;
	GKeyFile *plugin_file = NULL;
	gchar *str;

	g_return_val_if_fail (file != NULL, NULL);

	iog_debug_message (DEBUG_PLUGINS, "Loading plugin: %s", file);

	info = g_new0 (IogPluginInfo, 1);
	info->file = g_strdup (file);

	plugin_file = g_key_file_new ();

	if (!g_key_file_load_from_file (plugin_file, file, G_KEY_FILE_NONE, NULL)) {
		g_warning ("Bad plugin file: %s", file);

		goto error;
	}

	if (!g_key_file_has_key (plugin_file,
			   	 "Iog Plugin",
				 "IAge",
				 NULL))	{
		iog_debug_message (DEBUG_PLUGINS,
				   "IAge key does not exist in file: %s", file);

		goto error;
	}

	/* Check IAge=2 */
	if (g_key_file_get_integer (plugin_file,
				    "Iog Plugin",
				    "IAge",
				    NULL) != 2) {
		iog_debug_message (DEBUG_PLUGINS,
				   "Wrong IAge in file: %s", file);

		goto error;
	}

	/* Get Location */
	str = g_key_file_get_string (plugin_file,
				     "Iog Plugin",
				     "Module",
				     NULL);

	if ((str != NULL) && (*str != '\0')) {
		info->location = str;
	} else {
		g_warning ("Could not find 'Module' in %s", file);

		goto error;
	}

	/* Get the loader for this plugin */
	str = g_key_file_get_string (plugin_file,
				     "Iog Plugin",
				     "Loader",
				     NULL);

	if (str && strcmp(str, "python") == 0) {
		info->loader = IOG_PLUGIN_LOADER_PY;

#ifndef ENABLE_PYTHON
		g_warning ("Cannot load Python plugin '%s' since iog was not "
			   "compiled with Python support.", file);

		goto error;
#endif

	} else {
		info->loader = IOG_PLUGIN_LOADER_C;
	}

	g_free (str);

	/* Get Name */
	str = g_key_file_get_locale_string (plugin_file,
					    "Iog Plugin",
					    "Name",
					    NULL, NULL);
	if (str) {
		info->name = str;
	} else {
		g_warning ("Could not find 'Name' in %s", file);

		goto error;
	}

	/* Get Description */
	str = g_key_file_get_locale_string (plugin_file,
					    "Iog Plugin",
					    "Description",
					    NULL, NULL);
	if (str) {
		info->desc = str;
	} else {
		iog_debug_message (DEBUG_PLUGINS, "Could not find 'Description' in %s", file);
 	}

	/* Get Icon */
	str = g_key_file_get_locale_string (plugin_file,
					    "Iog Plugin",
					    "Icon",
					    NULL, NULL);
	if (str) {
		info->icon_name = str;
	} else {
		iog_debug_message (DEBUG_PLUGINS, "Could not find 'Icon' in %s, "
						  "using 'iog-plugin'", file);
	}

	/* Get Authors */
	info->authors = g_key_file_get_string_list (plugin_file,
						    "Iog Plugin",
						    "Authors",
						    NULL,
						    NULL);

	if (info->authors == NULL)
		iog_debug_message (DEBUG_PLUGINS, "Could not find 'Authors' in %s", file);


	/* Get Copyright */
	str = g_key_file_get_string (plugin_file,
				     "Iog Plugin",
				     "Copyright",
				     NULL);
	if (str) {
		info->copyright = str;
	} else {
		iog_debug_message (DEBUG_PLUGINS, "Could not find 'Copyright' in %s", file);
	}

	/* Get Website */
	str = g_key_file_get_string (plugin_file,
				     "Iog Plugin",
				     "Website",
				     NULL);
	if (str) {
		info->website = str;
	} else {
		iog_debug_message (DEBUG_PLUGINS, "Could not find 'Website' in %s", file);
	}

	g_key_file_free (plugin_file);

	/* If we know nothing about the availability of the plugin,
	   set it as available */
	info->available = TRUE;

	return info;

error:
	g_free (info->file);
	g_free (info->location);
	g_free (info->name);
	g_free (info);
	g_key_file_free (plugin_file);

	return NULL;
}

static gint
compare_plugin_info (IogPluginInfo *info1,
		     IogPluginInfo *info2)
{
	return strcmp (info1->location, info2->location);
}

static void
iog_plugin_engine_load_dir (const gchar *dir)
{
	GError *error = NULL;
	GDir *d;
	const gchar *dirent;

	if (!g_file_test (dir, G_FILE_TEST_IS_DIR)) {
		return;
	}

	g_return_if_fail (iog_plugin_engine_settings != NULL);

	iog_debug_message (DEBUG_PLUGINS, "DIR: %s", dir);

	d = g_dir_open (dir, 0, &error);

	if (!d)	{
		g_warning ("%s", error->message);
		g_error_free (error);

		return;
	}

	while ((dirent = g_dir_read_name (d))) {
		if (g_str_has_suffix (dirent, PLUGIN_EXT)) {
			gchar *plugin_file;
			IogPluginInfo *info;

			plugin_file = g_build_filename (dir, dirent, NULL);
			info = iog_plugin_engine_load (plugin_file);
			g_free (plugin_file);

			if (info == NULL)
				continue;

			/* If a plugin with this name has already been loaded
			 * drop this one (user plugins override system plugins) */
			if (g_list_find_custom (iog_plugins_list,
						info,
						(GCompareFunc)compare_plugin_info) != NULL) {
				g_warning ("Two or more plugins named '%s'. "
					   "Only the first will be considered.\n",
					   info->location);

				iog_plugin_info_free (info);

				continue;
			}

			/* Actually, the plugin will be activated when reactivate_all
			 * will be called for the first time. */
			info->active = (g_slist_find_custom (active_plugins,
							     info->location,
							     (GCompareFunc)strcmp) != NULL);

			iog_plugins_list = g_list_prepend (iog_plugins_list, info);

			iog_debug_message (DEBUG_PLUGINS, "Plugin %s loaded", info->name);
		}
	}

	iog_plugins_list = g_list_reverse (iog_plugins_list);

	g_dir_close (d);
}

static void
iog_plugin_engine_load_all (void)
{
	gchar *pdir;

	pdir = g_build_filename (iog_util_dot_dir (),
                                 USER_IOG_PLUGINS_LOCATION, NULL);

	/* Load user's plugins */
	iog_plugin_engine_load_dir (pdir);

	g_free (pdir);

	/* Load system plugins */
	iog_plugin_engine_load_dir (IOG_PLUGIN_DIR "/");
}

static void
iog_plugin_engine_get_active_plugins (void)
{
	gchar **array;
	gint i;

	active_plugins = NULL;
	array = g_settings_get_strv (iog_plugin_engine_settings, IOG_CONF_PLUGINS_ACTIVE_PLUGINS);
	if (array != NULL) {
		for (i = 0; array[i]; i++) {
			active_plugins = g_slist_append (active_plugins, g_strdup (array[i]));
		}
	}
	g_strfreev (array);
}

gboolean
iog_plugin_engine_init (void)
{
	iog_debug (DEBUG_PLUGINS);

	g_return_val_if_fail (iog_plugins_list == NULL, FALSE);

	if (!g_module_supported ()) {
		g_warning ("iog is not able to initialize the plugins engine.");

		return FALSE;
	}

	iog_plugin_engine_settings = g_settings_new (IOG_CONF_PLUGINS);

	g_signal_connect (iog_plugin_engine_settings,
			  "changed::" IOG_CONF_PLUGINS_ACTIVE_PLUGINS,
			  G_CALLBACK (iog_plugin_engine_active_plugins_changed),
			  NULL);

	iog_plugin_engine_get_active_plugins ();

	iog_plugin_engine_load_all ();

	return TRUE;
}

void
iog_plugin_engine_garbage_collect (void)
{
#ifdef ENABLE_PYTHON
	iog_python_garbage_collect ();
#endif
}

void
iog_plugin_engine_shutdown (void)
{
	GList *pl;

	iog_debug (DEBUG_PLUGINS);

#ifdef ENABLE_PYTHON
	/* Note: that this may cause finalization of objects (typically
	 * the IogWindow) by running the garbage collector. Since some
	 * of the plugin may have installed callbacks upon object
	 * finalization (typically they need to free the WindowData)
	 * it must run before we get rid of the plugins.
	 */
	iog_python_shutdown ();
#endif

	g_return_if_fail (iog_plugin_engine_settings != NULL);

	for (pl = iog_plugins_list; pl; pl = pl->next) {
		IogPluginInfo *info = (IogPluginInfo*) pl->data;

		iog_plugin_info_free (info);
	}

	g_slist_foreach (active_plugins, (GFunc)g_free, NULL);
	g_slist_free (active_plugins);

	active_plugins = NULL;

	g_list_free (iog_plugins_list);
	iog_plugins_list = NULL;

	g_object_unref (iog_plugin_engine_settings);
	iog_plugin_engine_settings = NULL;
}

const GList *
iog_plugin_engine_get_plugins_list (void)
{
	iog_debug (DEBUG_PLUGINS);

	return iog_plugins_list;
}

static gboolean
load_plugin_module (IogPluginInfo *info)
{
	gchar *path;
	gchar *dirname;

	iog_debug (DEBUG_PLUGINS);

	g_return_val_if_fail (info != NULL, FALSE);
	g_return_val_if_fail (info->file != NULL, FALSE);
	g_return_val_if_fail (info->location != NULL, FALSE);
	g_return_val_if_fail (info->plugin == NULL, FALSE);
	g_return_val_if_fail (info->available, FALSE);

	switch (info->loader) {
	case IOG_PLUGIN_LOADER_C:
		dirname = g_path_get_dirname (info->file);
		g_return_val_if_fail (dirname != NULL, FALSE);

		path = g_module_build_path (dirname, info->location);

		g_free (dirname);

		g_return_val_if_fail (path != NULL, FALSE);

		info->module = G_TYPE_MODULE (iog_module_new (path));

		g_free (path);

		break;

#ifdef ENABLE_PYTHON
	case IOG_PLUGIN_LOADER_PY:
	{
		gchar *dir;

		if (!iog_python_init ()) {
			/* Mark plugin as unavailable and fails */
			info->available = FALSE;

			g_warning ("Cannot load Python plugin '%s' since iog "
			           "was not able to initialize the Python interpreter.",
			           info->name);

			return FALSE;
		}

		g_return_val_if_fail ((info->location != NULL) &&
		                      (info->location[0] != '\0'),
		                      FALSE);

		dir = g_path_get_dirname (info->file);

		info->module = G_TYPE_MODULE (
				iog_python_module_new (dir, info->location));

		g_free (dir);

		break;
	}
#endif
	default:
		g_return_val_if_reached (FALSE);
	}

	if (!g_type_module_use (info->module)) {
		switch (info->loader) {
		case IOG_PLUGIN_LOADER_C:
			g_warning ("Cannot load plugin '%s' since file '%s' cannot be read.",
				   info->name,
				   iog_module_get_path (IOG_MODULE (info->module)));
			break;

		case IOG_PLUGIN_LOADER_PY:
			g_warning ("Cannot load Python plugin '%s' since file '%s' cannot be read.",
				   info->name,
				   info->location);
			break;

		default:
			g_return_val_if_reached (FALSE);
		}

		g_object_unref (G_OBJECT (info->module));

		info->module = NULL;

		/* Mark plugin as unavailable and fails */
		info->available = FALSE;

		return FALSE;
	}

	switch (info->loader) {
	case IOG_PLUGIN_LOADER_C:
		info->plugin =
			IOG_PLUGIN (iog_module_new_object (IOG_MODULE (info->module)));
		break;

#ifdef ENABLE_PYTHON
	case IOG_PLUGIN_LOADER_PY:
		info->plugin =
			IOG_PLUGIN (iog_python_module_new_object (IOG_PYTHON_MODULE (info->module)));
		break;
#endif

	default:
		g_return_val_if_reached (FALSE);
	}

	g_type_module_unuse (info->module);

	iog_debug_message (DEBUG_PLUGINS, "End");

	return TRUE;
}

static gboolean
iog_plugin_engine_activate_plugin_real (IogPluginInfo *info)
{
	gboolean res = TRUE;

	/* Plugin is not available, don't try to activate/load it */
	if (!info->available) {
		return FALSE;
	}

	if (info->plugin == NULL)
		res = load_plugin_module (info);

	if (res) {
		const GList *wins = iog_application_get_windows (IOG_APP);

		while (wins != NULL) {
			iog_plugin_activate (info->plugin,
					     IOG_WINDOW (wins->data));

			wins = g_list_next (wins);
		}
	} else {
		g_warning ("Error activating plugin '%s'", info->name);
	}

	return res;
}

gboolean
iog_plugin_engine_activate_plugin (IogPluginInfo *info)
{
	iog_debug (DEBUG_PLUGINS);

	g_return_val_if_fail (info != NULL, FALSE);

	if (!info->available)
		return FALSE;

	if (info->active)
		return TRUE;

	if (iog_plugin_engine_activate_plugin_real (info)) {
		GSList *list;

		/* Update plugin state */
		info->active = TRUE;

		list = active_plugins;

		while (list != NULL) {
			if (strcmp (info->location, (gchar *)list->data) == 0) {
				g_warning ("Plugin '%s' is already active.", info->name);

				return TRUE;
			}

			list = g_slist_next (list);
		}

		active_plugins = g_slist_insert_sorted (active_plugins,
						        g_strdup (info->location),
						        (GCompareFunc)strcmp);

		GArray *array;
		GSList *l;
		array = g_array_new (TRUE, TRUE, sizeof (gchar *));
		for (l = active_plugins; l; l = l->next) {
			array = g_array_append_val (array, l->data);
		}
		g_settings_set_strv (iog_plugin_engine_settings, IOG_CONF_PLUGINS_ACTIVE_PLUGINS,
				(const gchar **) array->data);
		g_array_free (array, TRUE);

		return TRUE;
	}

	return FALSE;
}

static void
iog_plugin_engine_deactivate_plugin_real (IogPluginInfo *info)
{
	const GList *wins = iog_application_get_windows (IOG_APP);

	while (wins != NULL) {
		iog_plugin_deactivate (info->plugin,
				       IOG_WINDOW (wins->data));

		wins = g_list_next (wins);
	}
}

gboolean
iog_plugin_engine_deactivate_plugin (IogPluginInfo *info)
{
	gboolean res;
	GSList *list;

	iog_debug (DEBUG_PLUGINS);

	g_return_val_if_fail (info != NULL, FALSE);

	if (!info->active || !info->available)
		return TRUE;

	iog_plugin_engine_deactivate_plugin_real (info);

	/* Update plugin state */
	info->active = FALSE;

	list = active_plugins;
	res = (list == NULL);

	while (list != NULL) {
		if (strcmp (info->location, (gchar *)list->data) == 0) {
			g_free (list->data);
			active_plugins = g_slist_delete_link (active_plugins, list);
			list = NULL;
			res = TRUE;
		} else {
			list = g_slist_next (list);
		}
	}

	if (!res) {
		g_warning ("Plugin '%s' is already deactivated.", info->name);

		return TRUE;
	}

	GArray *array;
	GSList *l;
	array = g_array_new (TRUE, TRUE, sizeof (gchar *));
	for (l = active_plugins; l; l = l->next) {
		array = g_array_append_val (array, l->data);
	}
	g_settings_set_strv (iog_plugin_engine_settings, IOG_CONF_PLUGINS_ACTIVE_PLUGINS,
			(const gchar **) array->data);
	g_array_free (array, TRUE);

	return TRUE;
}

gboolean
iog_plugin_engine_plugin_is_active (IogPluginInfo *info)
{
	g_return_val_if_fail (info != NULL, FALSE);

	return (info->available && info->active);
}

gboolean
iog_plugin_engine_plugin_is_available (IogPluginInfo *info)
{
	g_return_val_if_fail (info != NULL, FALSE);

	return (info->available != FALSE);
}

static void
reactivate_all (IogWindow *window)
{
	GList *pl;

	iog_debug (DEBUG_PLUGINS);

	for (pl = iog_plugins_list; pl; pl = pl->next) {
		gboolean res = TRUE;

		IogPluginInfo *info = (IogPluginInfo*)pl->data;

		/* If plugin is not available, don't try to activate/load it */
		if (info->available && info->active) {
			if (info->plugin == NULL)
				res = load_plugin_module (info);

			if (res)
				iog_plugin_activate (info->plugin,
						     window);
		}
	}

	iog_debug_message (DEBUG_PLUGINS, "End");
}

void
iog_plugin_engine_update_plugins_ui (IogWindow *window,
				     gboolean   new_window)
{
	GList *pl;

	iog_debug (DEBUG_PLUGINS);

	g_return_if_fail (IOG_IS_WINDOW (window));

	if (new_window)
		reactivate_all (window);

	/* Updated ui of all the plugins that implement update_ui */
	for (pl = iog_plugins_list; pl; pl = pl->next) {
		IogPluginInfo *info = (IogPluginInfo*)pl->data;

		if (!info->available || !info->active)
			continue;

	       	iog_debug_message (DEBUG_PLUGINS, "Updating UI of %s", info->name);

		iog_plugin_update_ui (info->plugin, window);
	}
}

gboolean
iog_plugin_engine_plugin_is_configurable (IogPluginInfo *info) {
	iog_debug (DEBUG_PLUGINS);

	g_return_val_if_fail (info != NULL, FALSE);

	if ((info->plugin == NULL) || !info->active || !info->available)
		return FALSE;

	return iog_plugin_is_configurable (info->plugin);
}

void
iog_plugin_engine_configure_plugin (IogPluginInfo *info,
				    GtkWindow     *parent)
{
	GtkWidget *conf_dlg;

	GtkWindowGroup *wg;

	iog_debug (DEBUG_PLUGINS);

	g_return_if_fail (info != NULL);

	conf_dlg = iog_plugin_create_configure_dialog (info->plugin);

	g_return_if_fail (conf_dlg != NULL);

	gtk_window_set_transient_for (GTK_WINDOW (conf_dlg),
				      parent);

	// Will return a default group if no group is set
	wg = gtk_window_get_group (parent);

	// For now assign a dedicated window group if it is 
	// the default one until we know if this is really needed
	if (wg == gtk_window_get_group (NULL)) {
		wg = gtk_window_group_new ();
		gtk_window_group_add_window (wg, parent);
	}

	gtk_window_group_add_window (wg,
				     GTK_WINDOW (conf_dlg));

	gtk_window_set_modal (GTK_WINDOW (conf_dlg), TRUE);

	gtk_widget_show (conf_dlg);
}

static void
iog_plugin_engine_active_plugins_changed (GSettings *settings,
					  gchar *key,
					  gpointer user_data)
{
	GList *pl;
	gboolean to_activate;

	iog_debug (DEBUG_PLUGINS);

	g_return_if_fail (settings != NULL);
	g_return_if_fail (key != NULL);

	iog_plugin_engine_get_active_plugins ();

	for (pl = iog_plugins_list; pl; pl = pl->next) {
		IogPluginInfo *info = (IogPluginInfo*)pl->data;

		if (!info->available)
			continue;

		to_activate = (g_slist_find_custom (active_plugins,
						    info->location,
						    (GCompareFunc)strcmp) != NULL);

		if (!info->active && to_activate) {
			/* Activate plugin */
			if (iog_plugin_engine_activate_plugin_real (info))
				/* Update plugin state */
				info->active = TRUE;
		} else {
			if (info->active && !to_activate) {
				iog_plugin_engine_deactivate_plugin_real (info);

				/* Update plugin state */
				info->active = FALSE;
			}
		}
	}
}

const gchar *
iog_plugin_engine_get_plugin_name (IogPluginInfo *info)
{
	g_return_val_if_fail (info != NULL, NULL);

	return info->name;
}

const gchar *
iog_plugin_engine_get_plugin_description (IogPluginInfo *info)
{
	g_return_val_if_fail (info != NULL, NULL);

	return info->desc;
}

const gchar *
iog_plugin_engine_get_plugin_icon_name (IogPluginInfo *info)
{
	g_return_val_if_fail (info != NULL, NULL);

	/* Use the iog-plugin icon as a default if the plugin does not
	   have its own */
	if (info->icon_name != NULL &&
	    gtk_icon_theme_has_icon (gtk_icon_theme_get_default (),
	    			     info->icon_name))
		return info->icon_name;
	else
		return "iog-plugin";
}

const gchar **
iog_plugin_engine_get_plugin_authors (IogPluginInfo *info)
{
	g_return_val_if_fail (info != NULL, (const gchar **)NULL);

	return (const gchar **) info->authors;
}

const gchar *
iog_plugin_engine_get_plugin_website (IogPluginInfo *info)
{
	g_return_val_if_fail (info != NULL, NULL);

	return info->website;
}

const gchar *
iog_plugin_engine_get_plugin_copyright (IogPluginInfo *info)
{
	g_return_val_if_fail (info != NULL, NULL);

	return info->copyright;
}
