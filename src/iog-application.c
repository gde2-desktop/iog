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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "iog-image.h"
#include "iog-session.h"
#include "iog-window.h"
#include "iog-application.h"
#include "iog-util.h"

#ifdef HAVE_DBUS
#include "totem-scrsaver.h"
#endif

#include <string.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#ifdef HAVE_DBUS
#include "iog-application-service.h"
#include <dbus/dbus-glib-bindings.h>

#define APPLICATION_SERVICE_NAME "org.gde2.iog.ApplicationService"
#endif

static void iog_application_load_accelerators (void);
static void iog_application_save_accelerators (void);

#define IOG_APPLICATION_GET_PRIVATE(object) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((object), IOG_TYPE_APPLICATION, IogApplicationPrivate))

G_DEFINE_TYPE (IogApplication, iog_application, G_TYPE_OBJECT);

#ifdef HAVE_DBUS

/**
 * iog_application_register_service:
 * @application: An #IogApplication.
 *
 * Registers #IogApplication<!-- -->'s DBus service, to allow
 * remote calls. If the DBus service is already registered,
 * or there is any other connection error, returns %FALSE.
 *
 * Returns: %TRUE if the service was registered succesfully. %FALSE
 * otherwise.
 **/
gboolean
iog_application_register_service (IogApplication *application)
{
	static DBusGConnection *connection = NULL;
	DBusGProxy *driver_proxy;
	GError *err = NULL;
	guint request_name_result;

	if (connection) {
		g_warning ("Service already registered.");
		return FALSE;
	}

	connection = dbus_g_bus_get (DBUS_BUS_STARTER, &err);

	if (connection == NULL) {
		g_warning ("Service registration failed.");
		g_error_free (err);

		return FALSE;
	}

	driver_proxy = dbus_g_proxy_new_for_name (connection,
						  DBUS_SERVICE_DBUS,
						  DBUS_PATH_DBUS,
						  DBUS_INTERFACE_DBUS);

	if (!org_freedesktop_DBus_request_name (driver_proxy,
                                        	APPLICATION_SERVICE_NAME,
						DBUS_NAME_FLAG_DO_NOT_QUEUE,
						&request_name_result, &err)) {
		g_warning ("Service registration failed.");
		g_clear_error (&err);
	}

	g_object_unref (driver_proxy);

	if (request_name_result == DBUS_REQUEST_NAME_REPLY_EXISTS) {
		return FALSE;
	}

	dbus_g_object_type_install_info (IOG_TYPE_APPLICATION,
					 &dbus_glib_iog_application_object_info);

	dbus_g_connection_register_g_object (connection,
					     "/org/gde2/iog/Iog",
                                             G_OBJECT (application));

        application->scr_saver = totem_scrsaver_new ();
        g_object_set (application->scr_saver,
		      "reason", _("Running in fullscreen mode"),
		      NULL);

	return TRUE;
}
#endif /* ENABLE_DBUS */

static void
iog_application_class_init (IogApplicationClass *iog_application_class)
{
}

static void
iog_application_init (IogApplication *iog_application)
{
	const gchar *dot_dir = iog_util_dot_dir ();

	iog_session_init (iog_application);

	iog_application->toolbars_model = egg_toolbars_model_new ();

	egg_toolbars_model_load_names (iog_application->toolbars_model,
				       IOG_DATA_DIR "/iog-toolbar.xml");

	if (G_LIKELY (dot_dir != NULL))
		iog_application->toolbars_file = g_build_filename
			(dot_dir, "iog_toolbar.xml", NULL);

	if (!dot_dir || !egg_toolbars_model_load_toolbars (iog_application->toolbars_model,
					       iog_application->toolbars_file)) {

		egg_toolbars_model_load_toolbars (iog_application->toolbars_model,
						  IOG_DATA_DIR "/iog-toolbar.xml");
	}

	egg_toolbars_model_set_flags (iog_application->toolbars_model, 0,
				      EGG_TB_MODEL_NOT_REMOVABLE);

	iog_application_load_accelerators ();
}

/**
 * iog_application_get_instance:
 *
 * Returns a singleton instance of #IogApplication currently running.
 * If not running yet, it will create one.
 *
 * Returns: (transfer none): a running #IogApplication.
 **/
IogApplication *
iog_application_get_instance (void)
{
	static IogApplication *instance;

	if (!instance) {
		instance = IOG_APPLICATION (g_object_new (IOG_TYPE_APPLICATION, NULL));
	}

	return instance;
}

static IogWindow *
iog_application_get_empty_window (IogApplication *application)
{
	IogWindow *empty_window = NULL;
	GList *windows;
	GList *l;

	g_return_val_if_fail (IOG_IS_APPLICATION (application), NULL);

	windows = iog_application_get_windows (application);

	for (l = windows; l != NULL; l = l->next) {
		IogWindow *window = IOG_WINDOW (l->data);

		if (iog_window_is_empty (window)) {
			empty_window = window;
			break;
		}
	}

	g_list_free (windows);

	return empty_window;
}

/**
 * iog_application_open_window:
 * @application: An #IogApplication.
 * @timestamp: The timestamp of the user interaction which triggered this call
 * (see gtk_window_present_with_time()).
 * @flags: A set of #IogStartupFlags influencing a new windows' state.
 * @error: Return location for a #GError, or NULL to ignore errors.
 *
 * Opens and presents an empty #IogWindow to the user. If there is
 * an empty window already open, this will be used. Otherwise, a
 * new one will be instantiated.
 *
 * Returns: %FALSE if @application is invalid, %TRUE otherwise
 **/
gboolean
iog_application_open_window (IogApplication  *application,
			     guint32         timestamp,
			     IogStartupFlags flags,
			     GError        **error)
{
	GtkWidget *new_window = NULL;

	new_window = GTK_WIDGET (iog_application_get_empty_window (application));

	if (new_window == NULL) {
		new_window = iog_window_new (flags);
	}

	g_return_val_if_fail (IOG_IS_APPLICATION (application), FALSE);

	gtk_window_present_with_time (GTK_WINDOW (new_window),
				      timestamp);

	return TRUE;
}

static IogWindow *
iog_application_get_file_window (IogApplication *application, GFile *file)
{
	IogWindow *file_window = NULL;
	GList *windows;
	GList *l;

	g_return_val_if_fail (file != NULL, NULL);
	g_return_val_if_fail (IOG_IS_APPLICATION (application), NULL);

	windows = gtk_window_list_toplevels ();

	for (l = windows; l != NULL; l = l->next) {
		if (IOG_IS_WINDOW (l->data)) {
			IogWindow *window = IOG_WINDOW (l->data);

			if (!iog_window_is_empty (window)) {
				IogImage *image = iog_window_get_image (window);
				GFile *window_file;

				window_file = iog_image_get_file (image);
				if (g_file_equal (window_file, file)) {
					file_window = window;
					break;
				}
			}
		}
	}

	g_list_free (windows);

	return file_window;
}

static void
iog_application_show_window (IogWindow *window, gpointer user_data)
{
	guint32 timestamp = GPOINTER_TO_UINT (user_data);
	
	/* set the proper interaction time on the window.
	 * Fall back to roundtripping to the X server when we
	 * don't have the timestamp, e.g. when launched from
	 * terminal. We also need to make sure that the window
	 * has been realized otherwise it will not work. lame.
	 */
	if (!gtk_widget_get_realized (GTK_WIDGET (window)))
		gtk_widget_realize (GTK_WIDGET (window));
	
	if (timestamp <= 0)
		timestamp = gdk_x11_get_server_time (gtk_widget_get_window (GTK_WIDGET (window)));
	
	gdk_x11_window_set_user_time (gtk_widget_get_window (GTK_WIDGET (window)),
				                  timestamp);
		      
	gtk_window_present (GTK_WINDOW (window));
}

/**
 * iog_application_open_file_list:
 * @application: An #IogApplication.
 * @file_list: (element-type GFile): A list of #GFile<!-- -->s
 * @timestamp: The timestamp of the user interaction which triggered this call
 * (see gtk_window_present_with_time()).
 * @flags: A set of #IogStartupFlags influencing a new windows' state.
 * @error: Return location for a #GError, or NULL to ignore errors.
 *
 * Opens a list of files in a #IogWindow. If an #IogWindow displaying the first
 * image in the list is already open, this will be used. Otherwise, an empty
 * #IogWindow is used, either already existing or newly created.
 *
 * Returns: Currently always %TRUE.
 **/
gboolean
iog_application_open_file_list (IogApplication  *application,
				GSList          *file_list,
				guint           timestamp,
				IogStartupFlags flags,
				GError         **error)
{
	IogWindow *new_window = NULL;

	if (file_list != NULL)
		new_window = iog_application_get_file_window (application,
							      (GFile *) file_list->data);

	if (new_window != NULL) {
		gtk_window_present_with_time (GTK_WINDOW (new_window),
					      timestamp);
		return TRUE;
	}

	new_window = iog_application_get_empty_window (application);

	if (new_window == NULL) {
		new_window = IOG_WINDOW (iog_window_new (flags));
	}

	g_signal_connect (new_window,
			  "prepared",
			  G_CALLBACK (iog_application_show_window),
			  GUINT_TO_POINTER (timestamp));

	iog_window_open_file_list (new_window, file_list);

	return TRUE;
}

/**
 * iog_application_open_uri_list:
 * @application: An #IogApplication.
 * @uri_list: (element-type utf8): A list of URIs.
 * @timestamp: The timestamp of the user interaction which triggered this call
 * (see gtk_window_present_with_time()).
 * @flags: A set of #IogStartupFlags influencing a new windows' state.
 * @error: Return location for a #GError, or NULL to ignore errors.
 *
 * Opens a list of images, from a list of URIs. See
 * iog_application_open_file_list() for details.
 *
 * Returns: Currently always %TRUE.
 **/
gboolean
iog_application_open_uri_list (IogApplication  *application,
 			       GSList          *uri_list,
 			       guint           timestamp,
 			       IogStartupFlags flags,
 			       GError         **error)
{
 	GSList *file_list = NULL;

 	g_return_val_if_fail (IOG_IS_APPLICATION (application), FALSE);

 	file_list = iog_util_string_list_to_file_list (uri_list);

 	return iog_application_open_file_list (application,
					       file_list,
					       timestamp,
					       flags,
					       error);
}

#ifdef HAVE_DBUS
/**
 * iog_application_open_uris:
 * @application: an #IogApplication
 * @uris:  A #GList of URI strings.
 * @timestamp: The timestamp of the user interaction which triggered this call
 * (see gtk_window_present_with_time()).
 * @flags: A set of #IogStartupFlags influencing a new windows' state.
 * @error: Return location for a #GError, or NULL to ignore errors.
 *
 * Opens a list of images, from a list of URI strings. See
 * iog_application_open_file_list() for details.
 *
 * Returns: Currently always %TRUE.
 **/
gboolean
iog_application_open_uris (IogApplication  *application,
 			   gchar          **uris,
 			   guint           timestamp,
 			   IogStartupFlags flags,
 			   GError        **error)
{
 	GSList *file_list = NULL;

 	file_list = iog_util_strings_to_file_list (uris);

 	return iog_application_open_file_list (application, file_list, timestamp,
						    flags, error);
}
#endif

/**
 * iog_application_shutdown:
 * @application: An #IogApplication.
 *
 * Takes care of shutting down the Eye of GDE2, and quits.
 **/
void
iog_application_shutdown (IogApplication *application)
{
	g_return_if_fail (IOG_IS_APPLICATION (application));

	if (application->toolbars_model) {
		g_object_unref (application->toolbars_model);
		application->toolbars_model = NULL;

		g_free (application->toolbars_file);
		application->toolbars_file = NULL;
	}

	iog_application_save_accelerators ();

	g_object_unref (application);

	gtk_main_quit ();
}

/**
 * iog_application_get_windows:
 * @application: An #IogApplication.
 *
 * Gets the list of existing #IogApplication<!-- -->s. The windows
 * in this list are not individually referenced, you need to keep
 * your own references if you want to perform actions that may destroy
 * them.
 *
 * Returns: (element-type IogWindow) (transfer container): A new list of #IogWindow<!-- -->s.
 **/
GList *
iog_application_get_windows (IogApplication *application)
{
	GList *l, *toplevels;
	GList *windows = NULL;

	g_return_val_if_fail (IOG_IS_APPLICATION (application), NULL);

	toplevels = gtk_window_list_toplevels ();

	for (l = toplevels; l != NULL; l = l->next) {
		if (IOG_IS_WINDOW (l->data)) {
			windows = g_list_append (windows, l->data);
		}
	}

	g_list_free (toplevels);

	return windows;
}

/**
 * iog_application_get_toolbars_model:
 * @application: An #IogApplication.
 *
 * Retrieves the #EggToolbarsModel for the toolbar in #IogApplication.
 *
 * Returns: (transfer none): An #EggToolbarsModel.
 **/
EggToolbarsModel *
iog_application_get_toolbars_model (IogApplication *application)
{
	g_return_val_if_fail (IOG_IS_APPLICATION (application), NULL);

	return application->toolbars_model;
}

/**
 * iog_application_save_toolbars_model:
 * @application: An #IogApplication.
 *
 * Causes the saving of the model of the toolbar in #IogApplication to a file.
 **/
void
iog_application_save_toolbars_model (IogApplication *application)
{
	if (G_LIKELY(application->toolbars_file != NULL))
        	egg_toolbars_model_save_toolbars (application->toolbars_model,
				 	          application->toolbars_file,
						  "1.0");
}

/**
 * iog_application_reset_toolbars_model:
 * @app: an #IogApplication
 *
 * Restores the toolbars model to the defaults.
 **/
void
iog_application_reset_toolbars_model (IogApplication *app)
{
	g_return_if_fail (IOG_IS_APPLICATION (app));

	g_object_unref (app->toolbars_model);

	app->toolbars_model = egg_toolbars_model_new ();

	egg_toolbars_model_load_names (app->toolbars_model,
				       IOG_DATA_DIR "/iog-toolbar.xml");
	egg_toolbars_model_load_toolbars (app->toolbars_model,
					  IOG_DATA_DIR "/iog-toolbar.xml");
	egg_toolbars_model_set_flags (app->toolbars_model, 0,
				      EGG_TB_MODEL_NOT_REMOVABLE);
}

#ifdef HAVE_DBUS
/**
 * iog_application_screensaver_enable:
 * @application: an #IogApplication.
 *
 * Enables the screensaver. Usually necessary after a call to
 * iog_application_screensaver_disable().
 **/
void
iog_application_screensaver_enable (IogApplication *application)
{
        if (application->scr_saver)
                totem_scrsaver_enable (application->scr_saver);
}

/**
 * iog_application_screensaver_disable:
 * @application: an #IogApplication.
 *
 * Disables the screensaver. Useful when the application is in fullscreen or
 * similar mode.
 **/
void
iog_application_screensaver_disable (IogApplication *application)
{
        if (application->scr_saver)
                totem_scrsaver_disable (application->scr_saver);
}
#endif

static void
iog_application_load_accelerators (void)
{
		gchar* accelfile = g_build_filename(g_get_user_config_dir(), "gde2", "accels", "iog", NULL);

	/* gtk_accel_map_load does nothing if the file does not exist */
	gtk_accel_map_load (accelfile);
	g_free (accelfile);
}

static void
iog_application_save_accelerators (void)
{
		gchar* accelfile = g_build_filename(g_get_user_config_dir(), "gde2", "accels", "iog", NULL);

	gtk_accel_map_save (accelfile);
	g_free (accelfile);
}
