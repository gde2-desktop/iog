/* Eye Of Gde2 - IOG Plugin Engine
 *
 * Copyright (C) 2007 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 *
 * Based on gedit code (gedit/gedit-plugins-engine.h) by:
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

#ifndef __IOG_PLUGIN_ENGINE_H__
#define __IOG_PLUGIN_ENGINE_H__

#include "iog-window.h"

#include <glib.h>

typedef struct _IogPluginInfo IogPluginInfo;

G_GNUC_INTERNAL
gboolean	 iog_plugin_engine_init 		(void);

G_GNUC_INTERNAL
void		 iog_plugin_engine_shutdown 		(void);

G_GNUC_INTERNAL
void		 iog_plugin_engine_garbage_collect	(void);

G_GNUC_INTERNAL
const GList	*iog_plugin_engine_get_plugins_list 	(void);

G_GNUC_INTERNAL
gboolean 	 iog_plugin_engine_activate_plugin 	(IogPluginInfo *info);

G_GNUC_INTERNAL
gboolean 	 iog_plugin_engine_deactivate_plugin	(IogPluginInfo *info);

G_GNUC_INTERNAL
gboolean 	 iog_plugin_engine_plugin_is_active 	(IogPluginInfo *info);

G_GNUC_INTERNAL
gboolean 	 iog_plugin_engine_plugin_is_available	(IogPluginInfo *info);

G_GNUC_INTERNAL
gboolean	 iog_plugin_engine_plugin_is_configurable
			       				(IogPluginInfo *info);

G_GNUC_INTERNAL
void	 	 iog_plugin_engine_configure_plugin	(IogPluginInfo *info,
			       			 	 GtkWindow     *parent);

G_GNUC_INTERNAL
void		 iog_plugin_engine_update_plugins_ui	(IogWindow     *window,
			       			 	 gboolean       new_window);

G_GNUC_INTERNAL
const gchar	*iog_plugin_engine_get_plugin_name	(IogPluginInfo *info);

G_GNUC_INTERNAL
const gchar	*iog_plugin_engine_get_plugin_description
			       				(IogPluginInfo *info);

G_GNUC_INTERNAL
const gchar	*iog_plugin_engine_get_plugin_icon_name (IogPluginInfo *info);

G_GNUC_INTERNAL
const gchar    **iog_plugin_engine_get_plugin_authors   (IogPluginInfo *info);

G_GNUC_INTERNAL
const gchar	*iog_plugin_engine_get_plugin_website   (IogPluginInfo *info);

G_GNUC_INTERNAL
const gchar	*iog_plugin_engine_get_plugin_copyright (IogPluginInfo *info);

#endif  /* __IOG_PLUGIN_ENGINE_H__ */
