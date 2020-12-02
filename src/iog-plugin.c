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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "iog-plugin.h"

G_DEFINE_TYPE (IogPlugin, iog_plugin, G_TYPE_OBJECT)

static void
dummy (IogPlugin *plugin, IogWindow *window)
{
}

static GtkWidget *
create_configure_dialog	(IogPlugin *plugin)
{
	return NULL;
}

static gboolean
is_configurable (IogPlugin *plugin)
{
	return (IOG_PLUGIN_GET_CLASS (plugin)->create_configure_dialog !=
		create_configure_dialog);
}

static void
iog_plugin_class_init (IogPluginClass *klass)
{
	klass->activate = dummy;
	klass->deactivate = dummy;
	klass->update_ui = dummy;

	klass->create_configure_dialog = create_configure_dialog;
	klass->is_configurable = is_configurable;
}

static void
iog_plugin_init (IogPlugin *plugin)
{
}

void
iog_plugin_activate (IogPlugin *plugin, IogWindow *window)
{
	g_return_if_fail (IOG_IS_PLUGIN (plugin));
	g_return_if_fail (IOG_IS_WINDOW (window));

	IOG_PLUGIN_GET_CLASS (plugin)->activate (plugin, window);
}

void
iog_plugin_deactivate (IogPlugin *plugin, IogWindow *window)
{
	g_return_if_fail (IOG_IS_PLUGIN (plugin));
	g_return_if_fail (IOG_IS_WINDOW (window));

	IOG_PLUGIN_GET_CLASS (plugin)->deactivate (plugin, window);
}

void
iog_plugin_update_ui (IogPlugin *plugin, IogWindow *window)
{
	g_return_if_fail (IOG_IS_PLUGIN (plugin));
	g_return_if_fail (IOG_IS_WINDOW (window));

	IOG_PLUGIN_GET_CLASS (plugin)->update_ui (plugin, window);
}

gboolean
iog_plugin_is_configurable (IogPlugin *plugin)
{
	g_return_val_if_fail (IOG_IS_PLUGIN (plugin), FALSE);

	return IOG_PLUGIN_GET_CLASS (plugin)->is_configurable (plugin);
}

GtkWidget *
iog_plugin_create_configure_dialog (IogPlugin *plugin)
{
	g_return_val_if_fail (IOG_IS_PLUGIN (plugin), NULL);

	return IOG_PLUGIN_GET_CLASS (plugin)->create_configure_dialog (plugin);
}
