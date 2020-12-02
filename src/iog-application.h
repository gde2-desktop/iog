/* Eye Of Gde2 - Application Facade
 *
 * Copyright (C) 2006 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 *
 * Based on evince code (shell/ev-application.h) by:
 * 	- Martin Kretzschmar <martink@gnome.org>
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

#ifndef __IOG_APPLICATION_H__
#define __IOG_APPLICATION_H__

#include "iog-window.h"
#include "egg-toolbars-model.h"

#ifdef HAVE_DBUS
#include "totem-scrsaver.h"
#endif

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _IogApplication IogApplication;
typedef struct _IogApplicationClass IogApplicationClass;
typedef struct _IogApplicationPrivate IogApplicationPrivate;

#define IOG_TYPE_APPLICATION            (iog_application_get_type ())
#define IOG_APPLICATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), IOG_TYPE_APPLICATION, IogApplication))
#define IOG_APPLICATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  IOG_TYPE_APPLICATION, IogApplicationClass))
#define IOG_IS_APPLICATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), IOG_TYPE_APPLICATION))
#define IOG_IS_APPLICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  IOG_TYPE_APPLICATION))
#define IOG_APPLICATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  IOG_TYPE_APPLICATION, IogApplicationClass))

#define IOG_APP				(iog_application_get_instance ())

struct _IogApplication {
	GObject base_instance;

	EggToolbarsModel *toolbars_model;
	gchar            *toolbars_file;
#ifdef HAVE_DBUS
	TotemScrsaver    *scr_saver;
#endif
};

struct _IogApplicationClass {
	GObjectClass parent_class;
};

GType	          iog_application_get_type	      (void) G_GNUC_CONST;

IogApplication   *iog_application_get_instance        (void);

#ifdef HAVE_DBUS
gboolean          iog_application_register_service    (IogApplication *application);
#endif

void	          iog_application_shutdown	      (IogApplication   *application);

gboolean          iog_application_open_window         (IogApplication   *application,
						       guint             timestamp,
						       IogStartupFlags   flags,
						       GError          **error);

gboolean          iog_application_open_uri_list      (IogApplication   *application,
						      GSList           *uri_list,
						      guint             timestamp,
						      IogStartupFlags   flags,
						      GError          **error);

gboolean          iog_application_open_file_list     (IogApplication  *application,
						      GSList          *file_list,
						      guint           timestamp,
						      IogStartupFlags flags,
						      GError         **error);

#ifdef HAVE_DBUS
gboolean          iog_application_open_uris           (IogApplication *application,
						       gchar         **uris,
						       guint           timestamp,
						       IogStartupFlags flags,
						       GError        **error);
#endif

GList		 *iog_application_get_windows	      (IogApplication   *application);

EggToolbarsModel *iog_application_get_toolbars_model  (IogApplication *application);

void              iog_application_save_toolbars_model (IogApplication *application);

void		  iog_application_reset_toolbars_model (IogApplication *app);

#ifdef HAVE_DBUS
void              iog_application_screensaver_enable  (IogApplication *application);

void              iog_application_screensaver_disable (IogApplication *application);
#endif

G_END_DECLS

#endif /* __IOG_APPLICATION_H__ */
