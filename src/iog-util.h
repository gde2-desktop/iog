/* Eye Of Gde2 - General Utilities
 *
 * Copyright (C) 2006 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 *
 * Based on code by:
 *	- Jens Finke <jens@gnome.org>
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

#ifndef __IOG_UTIL_H__
#define __IOG_UTIL_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

G_GNUC_INTERNAL
void     iog_util_show_help                  (const gchar *section,
					      GtkWindow   *parent);

G_GNUC_INTERNAL
gchar   *iog_util_make_valid_utf8            (const gchar *name);

G_GNUC_INTERNAL
GSList  *iog_util_parse_uri_string_list_to_file_list (const gchar *uri_list);

G_GNUC_INTERNAL
GSList  *iog_util_string_list_to_file_list    (GSList *string_list);

#ifdef HAVE_DBUS
G_GNUC_INTERNAL
GSList  *iog_util_strings_to_file_list        (gchar **strings);
#endif

G_GNUC_INTERNAL
GSList  *iog_util_string_array_to_list       (const gchar **files,
	 				      gboolean create_uri);

G_GNUC_INTERNAL
gchar  **iog_util_string_array_make_absolute (gchar **files);

G_GNUC_INTERNAL
gboolean iog_util_launch_desktop_file        (const gchar *filename,
					      guint32      user_time);

G_GNUC_INTERNAL
const    gchar *iog_util_dot_dir             (void);

G_GNUC_INTERNAL
char *  iog_util_filename_get_extension      (const char * filename_with_extension);

G_GNUC_INTERNAL
gboolean iog_util_file_is_persistent (GFile *file);

G_END_DECLS

#endif /* __IOG_UTIL_H__ */
