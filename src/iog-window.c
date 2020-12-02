/* Eye Of Gde2 - Main Window
 *
 * Copyright (C) 2000-2008 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 *
 * Based on code by:
 * 	- Federico Mena-Quintero <federico@gnu.org>
 *	- Jens Finke <jens@gnome.org>
 * Based on evince code (shell/ev-window.c) by:
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

#include <math.h>

#include "iog-window.h"
#include "iog-scroll-view.h"
#include "iog-debug.h"
#include "iog-file-chooser.h"
#include "iog-thumb-view.h"
#include "iog-list-store.h"
#include "iog-sidebar.h"
#include "iog-statusbar.h"
#include "iog-preferences-dialog.h"
#include "iog-properties-dialog.h"
#include "iog-print.h"
#include "iog-error-message-area.h"
#include "iog-application.h"
#include "iog-thumb-nav.h"
#include "iog-config-keys.h"
#include "iog-job-queue.h"
#include "iog-jobs.h"
#include "iog-util.h"
#include "iog-save-as-dialog-helper.h"
#include "iog-plugin-engine.h"
#include "iog-close-confirmation-dialog.h"
#include "iog-clipboard-handler.h"

#include "iog-enum-types.h"

#include "egg-toolbar-editor.h"
#include "egg-editable-toolbar.h"
#include "egg-toolbars-model.h"

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gdk/gdkkeysyms.h>
#include <gio/gdesktopappinfo.h>
#include <gtk/gtk.h>

#if HAVE_LCMS
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif
#include <lcms2.h>
#endif

#define GDE2_DESKTOP_USE_UNSTABLE_API
#include <libgde2-desktop/gde2-desktop-utils.h>

#define IOG_WINDOW_GET_PRIVATE(object) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((object), IOG_TYPE_WINDOW, IogWindowPrivate))

G_DEFINE_TYPE (IogWindow, iog_window, GTK_TYPE_WINDOW);

#define IOG_WINDOW_MIN_WIDTH  440
#define IOG_WINDOW_MIN_HEIGHT 350

#define IOG_WINDOW_DEFAULT_WIDTH  540
#define IOG_WINDOW_DEFAULT_HEIGHT 450

#define IOG_WINDOW_FULLSCREEN_TIMEOUT 5 * 1000
#define IOG_WINDOW_FULLSCREEN_POPUP_THRESHOLD 5

#define IOG_RECENT_FILES_GROUP  "Graphics"
#define IOG_RECENT_FILES_APP_NAME "Eye of GDE2 Image Viewer"
#define IOG_RECENT_FILES_LIMIT  5

#define IOG_WALLPAPER_FILENAME "iog-wallpaper"

#define is_rtl (gtk_widget_get_default_direction () == GTK_TEXT_DIR_RTL)

#if GTK_CHECK_VERSION (3, 2, 0)
#define gtk_hbox_new(X,Y) gtk_box_new(GTK_ORIENTATION_HORIZONTAL,Y)
#define gtk_vbox_new(X,Y) gtk_box_new(GTK_ORIENTATION_VERTICAL,Y)
#endif

typedef enum {
	IOG_WINDOW_STATUS_UNKNOWN,
	IOG_WINDOW_STATUS_INIT,
	IOG_WINDOW_STATUS_NORMAL
} IogWindowStatus;

enum {
	PROP_0,
	PROP_COLLECTION_POS,
	PROP_COLLECTION_RESIZABLE,
	PROP_STARTUP_FLAGS
};

enum {
	SIGNAL_PREPARED,
	SIGNAL_LAST
};

static guint signals[SIGNAL_LAST] = { 0 };

struct _IogWindowPrivate {
	GSettings           *view_settings;
	GSettings           *ui_settings;
	GSettings           *fullscreen_settings;
	GSettings           *lockdown_settings;

	IogListStore        *store;
	IogImage            *image;
	IogWindowMode        mode;
	IogWindowStatus      status;

	GtkUIManager        *ui_mgr;
	GtkWidget           *box;
	GtkWidget           *layout;
	GtkWidget           *cbox;
	GtkWidget           *view;
	GtkWidget           *sidebar;
	GtkWidget           *thumbview;
	GtkWidget           *statusbar;
	GtkWidget           *nav;
	GtkWidget           *message_area;
	GtkWidget           *toolbar;
	GObject             *properties_dlg;

	GtkActionGroup      *actions_window;
	GtkActionGroup      *actions_image;
	GtkActionGroup      *actions_collection;
	GtkActionGroup      *actions_recent;

	GtkWidget           *fullscreen_popup;
	GSource             *fullscreen_timeout_source;

	gboolean             slideshow_random;
	gboolean             slideshow_loop;
	gint                 slideshow_switch_timeout;
	GSource             *slideshow_switch_source;

	guint                recent_menu_id;

	IogJob              *load_job;
	IogJob              *transform_job;
	IogJob              *save_job;
	GFile               *last_save_as_folder;
	IogJob              *copy_job;

	guint                image_info_message_cid;
	guint                tip_message_cid;
	guint                copy_file_cid;

	IogStartupFlags      flags;
	GSList              *file_list;

	IogWindowCollectionPos collection_position;
	gboolean             collection_resizable;

	GtkActionGroup      *actions_open_with;
	guint                open_with_menu_id;

	gboolean             save_disabled;
	gboolean             needs_reload_confirmation;

	GtkPageSetup        *page_setup;

#ifdef HAVE_LCMS
	cmsHPROFILE         *display_profile;
#endif
};

static void iog_window_cmd_fullscreen (GtkAction *action, gpointer user_data);
static void iog_window_run_fullscreen (IogWindow *window, gboolean slideshow);
static void iog_window_cmd_slideshow (GtkAction *action, gpointer user_data);
static void iog_window_cmd_pause_slideshow (GtkAction *action, gpointer user_data);
static void iog_window_stop_fullscreen (IogWindow *window, gboolean slideshow);
static void iog_job_load_cb (IogJobLoad *job, gpointer data);
static void iog_job_save_progress_cb (IogJobSave *job, float progress, gpointer data);
static void iog_job_progress_cb (IogJobLoad *job, float progress, gpointer data);
static void iog_job_transform_cb (IogJobTransform *job, gpointer data);
static void fullscreen_set_timeout (IogWindow *window);
static void fullscreen_clear_timeout (IogWindow *window);
static void update_action_groups_state (IogWindow *window);
static void open_with_launch_application_cb (GtkAction *action, gpointer callback_data);
static void iog_window_update_openwith_menu (IogWindow *window, IogImage *image);
static void iog_window_list_store_image_added (GtkTreeModel *tree_model,
					       GtkTreePath  *path,
					       GtkTreeIter  *iter,
					       gpointer      user_data);
static void iog_window_list_store_image_removed (GtkTreeModel *tree_model,
                 				 GtkTreePath  *path,
						 gpointer      user_data);
static void iog_window_set_wallpaper (IogWindow *window, const gchar *filename, const gchar *visible_filename);
static gboolean iog_window_save_images (IogWindow *window, GList *images);
static void iog_window_finish_saving (IogWindow *window);
static GAppInfo *get_appinfo_for_editor (IogWindow *window);

static GQuark
iog_window_error_quark (void)
{
	static GQuark q = 0;

	if (q == 0)
		q = g_quark_from_static_string ("iog-window-error-quark");

	return q;
}

static void
iog_window_set_collection_mode (IogWindow *window, IogWindowCollectionPos position, gboolean resizable)
{
	IogWindowPrivate *priv;
	GtkWidget *hpaned;
	IogThumbNavMode mode = IOG_THUMB_NAV_MODE_ONE_ROW;

	iog_debug (DEBUG_PREFERENCES);

	g_return_if_fail (IOG_IS_WINDOW (window));

	priv = window->priv;

	if (priv->collection_position == position &&
	    priv->collection_resizable == resizable)
		return;

	priv->collection_position = position;
	priv->collection_resizable = resizable;

	hpaned = gtk_widget_get_parent (priv->sidebar);

	g_object_ref (hpaned);
	g_object_ref (priv->nav);

	gtk_container_remove (GTK_CONTAINER (priv->layout), hpaned);
	gtk_container_remove (GTK_CONTAINER (priv->layout), priv->nav);

	gtk_widget_destroy (priv->layout);

	switch (position) {
	case IOG_WINDOW_COLLECTION_POS_BOTTOM:
	case IOG_WINDOW_COLLECTION_POS_TOP:
		if (resizable) {
			mode = IOG_THUMB_NAV_MODE_MULTIPLE_ROWS;

#if GTK_CHECK_VERSION (3, 2, 0)
			priv->layout = gtk_paned_new (GTK_ORIENTATION_VERTICAL);
#else
			priv->layout = gtk_vpaned_new ();
#endif

			if (position == IOG_WINDOW_COLLECTION_POS_BOTTOM) {
				gtk_paned_pack1 (GTK_PANED (priv->layout), hpaned, TRUE, FALSE);
				gtk_paned_pack2 (GTK_PANED (priv->layout), priv->nav, FALSE, TRUE);
			} else {
				gtk_paned_pack1 (GTK_PANED (priv->layout), priv->nav, FALSE, TRUE);
				gtk_paned_pack2 (GTK_PANED (priv->layout), hpaned, TRUE, FALSE);
			}
		} else {
			mode = IOG_THUMB_NAV_MODE_ONE_ROW;

			priv->layout = gtk_vbox_new (FALSE, 2);

			if (position == IOG_WINDOW_COLLECTION_POS_BOTTOM) {
				gtk_box_pack_start (GTK_BOX (priv->layout), hpaned, TRUE, TRUE, 0);
				gtk_box_pack_start (GTK_BOX (priv->layout), priv->nav, FALSE, FALSE, 0);
			} else {
				gtk_box_pack_start (GTK_BOX (priv->layout), priv->nav, FALSE, FALSE, 0);
				gtk_box_pack_start (GTK_BOX (priv->layout), hpaned, TRUE, TRUE, 0);
			}
		}
		break;

	case IOG_WINDOW_COLLECTION_POS_LEFT:
	case IOG_WINDOW_COLLECTION_POS_RIGHT:
		if (resizable) {
			mode = IOG_THUMB_NAV_MODE_MULTIPLE_COLUMNS;

#if GTK_CHECK_VERSION (3, 2, 0)
			priv->layout = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);
#else
			priv->layout = gtk_hpaned_new ();
#endif

			if (position == IOG_WINDOW_COLLECTION_POS_LEFT) {
				gtk_paned_pack1 (GTK_PANED (priv->layout), priv->nav, FALSE, TRUE);
				gtk_paned_pack2 (GTK_PANED (priv->layout), hpaned, TRUE, FALSE);
			} else {
				gtk_paned_pack1 (GTK_PANED (priv->layout), hpaned, TRUE, FALSE);
				gtk_paned_pack2 (GTK_PANED (priv->layout), priv->nav, FALSE, TRUE);
			}
		} else {
			mode = IOG_THUMB_NAV_MODE_ONE_COLUMN;

			priv->layout = gtk_hbox_new (FALSE, 2);

			if (position == IOG_WINDOW_COLLECTION_POS_LEFT) {
				gtk_box_pack_start (GTK_BOX (priv->layout), priv->nav, FALSE, FALSE, 0);
				gtk_box_pack_start (GTK_BOX (priv->layout), hpaned, TRUE, TRUE, 0);
			} else {
				gtk_box_pack_start (GTK_BOX (priv->layout), hpaned, TRUE, TRUE, 0);
				gtk_box_pack_start (GTK_BOX (priv->layout), priv->nav, FALSE, FALSE, 0);
			}
		}

		break;
	}

	gtk_box_pack_end (GTK_BOX (priv->cbox), priv->layout, TRUE, TRUE, 0);

	iog_thumb_nav_set_mode (IOG_THUMB_NAV (priv->nav), mode);

	if (priv->mode != IOG_WINDOW_MODE_UNKNOWN) {
		update_action_groups_state (window);
	}
}

static void
iog_window_can_save_changed_cb (GSettings *settings, gchar *key, gpointer user_data)
{
	IogWindowPrivate *priv;
	IogWindow *window;
	gboolean save_disabled = FALSE;
	GtkAction *action_save, *action_save_as;

	iog_debug (DEBUG_PREFERENCES);

	g_return_if_fail (IOG_IS_WINDOW (user_data));

	window = IOG_WINDOW (user_data);
	priv = IOG_WINDOW (user_data)->priv;

	save_disabled = g_settings_get_boolean (settings, key);

	priv->save_disabled = save_disabled;

	action_save =
		gtk_action_group_get_action (priv->actions_image, "ImageSave");
	action_save_as =
		gtk_action_group_get_action (priv->actions_image, "ImageSaveAs");

	if (priv->save_disabled) {
		gtk_action_set_sensitive (action_save, FALSE);
		gtk_action_set_sensitive (action_save_as, FALSE);
	} else {
		IogImage *image = iog_window_get_image (window);

		if (IOG_IS_IMAGE (image)) {
			gtk_action_set_sensitive (action_save,
						  iog_image_is_modified (image));

			gtk_action_set_sensitive (action_save_as, TRUE);
		}
	}
}

#ifdef HAVE_LCMS
static cmsHPROFILE *
iog_window_get_display_profile (GdkScreen *screen)
{
	Display *dpy;
	Atom icc_atom, type;
	int format;
	gulong nitems;
	gulong bytes_after;
	gulong length;
	guchar *str;
	int result;
	cmsHPROFILE *profile;
	char *atom_name;

	dpy = GDK_DISPLAY_XDISPLAY (gdk_screen_get_display (screen));

	if (gdk_screen_get_number (screen) > 0)
		atom_name = g_strdup_printf ("_ICC_PROFILE_%d", gdk_screen_get_number (screen));
	else
		atom_name = g_strdup ("_ICC_PROFILE");

	icc_atom = gdk_x11_get_xatom_by_name_for_display (gdk_screen_get_display (screen), atom_name);

	g_free (atom_name);

	result = XGetWindowProperty (dpy,
				     GDK_WINDOW_XID (gdk_screen_get_root_window (screen)),
				     icc_atom,
				     0,
				     G_MAXLONG,
				     False,
				     XA_CARDINAL,
				     &type,
				     &format,
				     &nitems,
				     &bytes_after,
                                     (guchar **)&str);

	/* TODO: handle bytes_after != 0 */

	if ((result == Success) && (type == XA_CARDINAL) && (nitems > 0)) {
		switch (format)
		{
			case 8:
				length = nitems;
				break;
			case 16:
				length = sizeof(short) * nitems;
				break;
			case 32:
				length = sizeof(long) * nitems;
				break;
			default:
				iog_debug_message (DEBUG_LCMS, "Unable to read profile, not correcting");

				XFree (str);
				return NULL;
		}

		profile = cmsOpenProfileFromMem (str, length);

		if (G_UNLIKELY (profile == NULL)) {
			iog_debug_message (DEBUG_LCMS,
					   "Invalid display profile, "
					   "not correcting");
		}

		XFree (str);
	} else {
		profile = NULL;
		iog_debug_message (DEBUG_LCMS, "No profile, not correcting");
	}

	return profile;
}
#endif

static void
update_image_pos (IogWindow *window)
{
	IogWindowPrivate *priv;
	gint pos = -1, n_images = 0;

	priv = window->priv;

	n_images = iog_list_store_length (IOG_LIST_STORE (priv->store));

	if (n_images > 0) {
		pos = iog_list_store_get_pos_by_image (IOG_LIST_STORE (priv->store),
						       priv->image);
	}
	/* Images: (image pos) / (n_total_images) */
	iog_statusbar_set_image_number (IOG_STATUSBAR (priv->statusbar),
					pos + 1,
					n_images);

}

static void
update_status_bar (IogWindow *window)
{
	IogWindowPrivate *priv;
	char *str = NULL;

	g_return_if_fail (IOG_IS_WINDOW (window));

	iog_debug (DEBUG_WINDOW);

	priv = window->priv;

	if (priv->image != NULL &&
	    iog_image_has_data (priv->image, IOG_IMAGE_DATA_DIMENSION)) {
		int zoom, width, height;
		goffset bytes = 0;

		zoom = floor (100 * iog_scroll_view_get_zoom (IOG_SCROLL_VIEW (priv->view)) + 0.5);

		iog_image_get_size (priv->image, &width, &height);

		bytes = iog_image_get_bytes (priv->image);

		if ((width > 0) && (height > 0)) {
			char *size_string;

				size_string = g_format_size (bytes);

			/* Translators: This is the string displayed in the statusbar
			 * The tokens are from left to right:
			 * - image width
			 * - image height
			 * - image size in bytes
			 * - zoom in percent */
			str = g_strdup_printf (ngettext("%i × %i pixel  %s    %i%%",
							"%i × %i pixels  %s    %i%%", height),
						width,
						height,
						size_string,
						zoom);

			g_free (size_string);
		}

		update_image_pos (window);
	}

	gtk_statusbar_pop (GTK_STATUSBAR (priv->statusbar),
			   priv->image_info_message_cid);

	gtk_statusbar_push (GTK_STATUSBAR (priv->statusbar),
			    priv->image_info_message_cid, str ? str : "");

	g_free (str);
}

static void
iog_window_set_message_area (IogWindow *window,
		             GtkWidget *message_area)
{
	if (window->priv->message_area == message_area)
		return;

	if (window->priv->message_area != NULL)
		gtk_widget_destroy (window->priv->message_area);

	window->priv->message_area = message_area;

	if (message_area == NULL) return;

	gtk_box_pack_start (GTK_BOX (window->priv->cbox),
			    window->priv->message_area,
			    FALSE,
			    FALSE,
			    0);

	g_object_add_weak_pointer (G_OBJECT (window->priv->message_area),
				   (void *) &window->priv->message_area);
}

static void
update_action_groups_state (IogWindow *window)
{
	IogWindowPrivate *priv;
	GtkAction *action_collection;
	GtkAction *action_sidebar;
	GtkAction *action_fscreen;
	GtkAction *action_sshow;
	GtkAction *action_print;
	gboolean print_disabled = FALSE;
	gboolean show_image_collection = FALSE;
	gint n_images = 0;

	g_return_if_fail (IOG_IS_WINDOW (window));

	iog_debug (DEBUG_WINDOW);

	priv = window->priv;

	action_collection =
		gtk_action_group_get_action (priv->actions_window,
					     "ViewImageCollection");

	action_sidebar =
		gtk_action_group_get_action (priv->actions_window,
					     "ViewSidebar");

	action_fscreen =
		gtk_action_group_get_action (priv->actions_image,
					     "ViewFullscreen");

	action_sshow =
		gtk_action_group_get_action (priv->actions_collection,
					     "ViewSlideshow");

	action_print =
		gtk_action_group_get_action (priv->actions_image,
					     "ImagePrint");

	g_assert (action_collection != NULL);
	g_assert (action_sidebar != NULL);
	g_assert (action_fscreen != NULL);
	g_assert (action_sshow != NULL);
	g_assert (action_print != NULL);

	if (priv->store != NULL) {
		n_images = iog_list_store_length (IOG_LIST_STORE (priv->store));
	}

	if (n_images == 0) {
		gtk_widget_hide (priv->layout);

		gtk_action_group_set_sensitive (priv->actions_window,      TRUE);
		gtk_action_group_set_sensitive (priv->actions_image,       FALSE);
		gtk_action_group_set_sensitive (priv->actions_collection,  FALSE);

		gtk_action_set_sensitive (action_fscreen, FALSE);
		gtk_action_set_sensitive (action_sshow,   FALSE);

		/* If there are no images on model, initialization
 		   stops here. */
		if (priv->status == IOG_WINDOW_STATUS_INIT) {
			priv->status = IOG_WINDOW_STATUS_NORMAL;
		}
	} else {
		if (priv->flags & IOG_STARTUP_DISABLE_COLLECTION) {
			g_settings_set_boolean (priv->ui_settings, IOG_CONF_UI_IMAGE_COLLECTION, FALSE);

			show_image_collection = FALSE;
		} else {
			show_image_collection =
				g_settings_get_boolean (priv->ui_settings, IOG_CONF_UI_IMAGE_COLLECTION);
		}

		show_image_collection = show_image_collection &&
					n_images > 1 &&
					priv->mode != IOG_WINDOW_MODE_SLIDESHOW;

		gtk_widget_show (priv->layout);

		if (show_image_collection)
			gtk_widget_show (priv->nav);

		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action_collection),
					      show_image_collection);

		gtk_action_group_set_sensitive (priv->actions_window, TRUE);
		gtk_action_group_set_sensitive (priv->actions_image,  TRUE);

		gtk_action_set_sensitive (action_fscreen, TRUE);

		if (n_images == 1) {
			gtk_action_group_set_sensitive (priv->actions_collection,  FALSE);
			gtk_action_set_sensitive (action_collection, FALSE);
			gtk_action_set_sensitive (action_sshow, FALSE);
		} else {
			gtk_action_group_set_sensitive (priv->actions_collection,  TRUE);
			gtk_action_set_sensitive (action_sshow, TRUE);
		}

		if (show_image_collection)
			gtk_widget_grab_focus (priv->thumbview);
		else
			gtk_widget_grab_focus (priv->view);
	}

	print_disabled = g_settings_get_boolean (priv->lockdown_settings,
						IOG_CONF_LOCKDOWN_CAN_PRINT);

	if (print_disabled) {
		gtk_action_set_sensitive (action_print, FALSE);
	}

	if (iog_sidebar_is_empty (IOG_SIDEBAR (priv->sidebar))) {
		gtk_action_set_sensitive (action_sidebar, FALSE);
		gtk_widget_hide (priv->sidebar);
	}
}

static void
update_selection_ui_visibility (IogWindow *window)
{
	IogWindowPrivate *priv;
	GtkAction *wallpaper_action;
	gint n_selected;

	priv = window->priv;

	n_selected = iog_thumb_view_get_n_selected (IOG_THUMB_VIEW (priv->thumbview));

	wallpaper_action =
		gtk_action_group_get_action (priv->actions_image,
					     "ImageSetAsWallpaper");

	if (n_selected == 1) {
		gtk_action_set_sensitive (wallpaper_action, TRUE);
	} else {
		gtk_action_set_sensitive (wallpaper_action, FALSE);
	}
}

static gboolean
add_file_to_recent_files (GFile *file)
{
	gchar *text_uri;
	GFileInfo *file_info;
	GtkRecentData *recent_data;
	static gchar *groups[2] = { IOG_RECENT_FILES_GROUP , NULL };

	if (file == NULL) return FALSE;

	/* The password gets stripped here because ~/.recently-used.xbel is
	 * readable by everyone (chmod 644). It also makes the workaround
	 * for the bug with gtk_recent_info_get_uri_display() easier
	 * (see the comment in iog_window_update_recent_files_menu()). */
	text_uri = g_file_get_uri (file);

	if (text_uri == NULL)
		return FALSE;

	file_info = g_file_query_info (file,
				       G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
				       0, NULL, NULL);
	if (file_info == NULL)
		return FALSE;

	recent_data = g_slice_new (GtkRecentData);
	recent_data->display_name = NULL;
	recent_data->description = NULL;
	recent_data->mime_type = (gchar *) g_file_info_get_content_type (file_info);
	recent_data->app_name = IOG_RECENT_FILES_APP_NAME;
	recent_data->app_exec = g_strjoin(" ", g_get_prgname (), "%u", NULL);
	recent_data->groups = groups;
	recent_data->is_private = FALSE;

	gtk_recent_manager_add_full (gtk_recent_manager_get_default (),
				     text_uri,
				     recent_data);

	g_free (recent_data->app_exec);
	g_free (text_uri);
	g_object_unref (file_info);

	g_slice_free (GtkRecentData, recent_data);

	return FALSE;
}

static void
image_thumb_changed_cb (IogImage *image, gpointer data)
{
	IogWindow *window;
	IogWindowPrivate *priv;
	GdkPixbuf *thumb;

	g_return_if_fail (IOG_IS_WINDOW (data));

	window = IOG_WINDOW (data);
	priv = window->priv;

	thumb = iog_image_get_thumbnail (image);

	if (thumb != NULL) {
		gtk_window_set_icon (GTK_WINDOW (window), thumb);

		if (window->priv->properties_dlg != NULL) {
			iog_properties_dialog_update (IOG_PROPERTIES_DIALOG (priv->properties_dlg),
						      image);
		}

		g_object_unref (thumb);
	} else if (!gtk_widget_get_visible (window->priv->nav)) {
		gint img_pos = iog_list_store_get_pos_by_image (window->priv->store, image);
		GtkTreePath *path = gtk_tree_path_new_from_indices (img_pos,-1);
		GtkTreeIter iter;

		gtk_tree_model_get_iter (GTK_TREE_MODEL (window->priv->store), &iter, path);
		iog_list_store_thumbnail_set (window->priv->store, &iter);
		gtk_tree_path_free (path);
	}
}

static void
file_changed_info_bar_response (GtkInfoBar *info_bar,
				gint response,
				IogWindow *window)
{
	if (response == GTK_RESPONSE_YES) {
		iog_window_reload_image (window);
	}

	window->priv->needs_reload_confirmation = TRUE;

	iog_window_set_message_area (window, NULL);
}
static void
image_file_changed_cb (IogImage *img, IogWindow *window)
{
	GtkWidget *info_bar;
	gchar *text, *markup;
	GtkWidget *image;
	GtkWidget *label;
	GtkWidget *hbox;

	if (window->priv->needs_reload_confirmation == FALSE)
		return;

	if (!iog_image_is_modified (img)) {
		/* Auto-reload when image is unmodified */
		iog_window_reload_image (window);
		return;
	}

	window->priv->needs_reload_confirmation = FALSE;

	info_bar = gtk_info_bar_new_with_buttons (_("_Reload"),
						  GTK_RESPONSE_YES,
						  C_("MessageArea", "Hi_de"),
						  GTK_RESPONSE_NO, NULL);
	gtk_info_bar_set_message_type (GTK_INFO_BAR (info_bar),
				       GTK_MESSAGE_QUESTION);
	image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_QUESTION,
					  GTK_ICON_SIZE_DIALOG);
	label = gtk_label_new (NULL);

	/* The newline character is currently necessary due to a problem
	 * with the automatic line break. */
	text = g_strdup_printf (_("The image \"%s\" has been modified by an external application."
				  "\nWould you like to reload it?"), iog_image_get_caption (img));
	markup = g_markup_printf_escaped ("<b>%s</b>", text);
	gtk_label_set_markup (GTK_LABEL (label), markup);
	g_free (text);
	g_free (markup);

	hbox = gtk_hbox_new (FALSE, 8);
	gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
#if GTK_CHECK_VERSION (3, 0, 0)
	gtk_widget_set_halign (image, GTK_ALIGN_START);
	gtk_widget_set_valign (image, GTK_ALIGN_END);
#else
	gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);
#endif
	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
#if GTK_CHECK_VERSION (3, 16, 0)
	gtk_label_set_xalign (GTK_LABEL (label), 0.0);
#else
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
	gtk_box_pack_start (GTK_BOX (gtk_info_bar_get_content_area (GTK_INFO_BAR (info_bar))), hbox, TRUE, TRUE, 0);
	gtk_widget_show_all (hbox);
	gtk_widget_show (info_bar);

	iog_window_set_message_area (window, info_bar);
	g_signal_connect (info_bar, "response",
			  G_CALLBACK (file_changed_info_bar_response), window);
}

static void
iog_window_display_image (IogWindow *window, IogImage *image)
{
	IogWindowPrivate *priv;
	GFile *file;

	g_return_if_fail (IOG_IS_WINDOW (window));
	g_return_if_fail (IOG_IS_IMAGE (image));

	iog_debug (DEBUG_WINDOW);

	g_assert (iog_image_has_data (image, IOG_IMAGE_DATA_IMAGE));

	priv = window->priv;

	if (image != NULL) {
		g_signal_connect (image,
				  "thumbnail_changed",
				  G_CALLBACK (image_thumb_changed_cb),
				  window);
		g_signal_connect (image, "file-changed",
				  G_CALLBACK (image_file_changed_cb),
				  window);

		image_thumb_changed_cb (image, window);
	}

	priv->needs_reload_confirmation = TRUE;

	iog_scroll_view_set_image (IOG_SCROLL_VIEW (priv->view), image);

	gtk_window_set_title (GTK_WINDOW (window), iog_image_get_caption (image));

	update_status_bar (window);

	file = iog_image_get_file (image);
	g_idle_add_full (G_PRIORITY_LOW,
			 (GSourceFunc) add_file_to_recent_files,
			 file,
			 (GDestroyNotify) g_object_unref);

	iog_window_update_openwith_menu (window, image);
}

static void
open_with_launch_application_cb (GtkAction *action, gpointer data) {
	IogImage *image;
	GAppInfo *app;
	GFile *file;
	GList *files = NULL;

	image = IOG_IMAGE (data);
	file = iog_image_get_file (image);

	app = g_object_get_data (G_OBJECT (action), "app");
	files = g_list_append (files, file);
	g_app_info_launch (app,
			   files,
			   NULL, NULL);

	g_object_unref (file);
	g_list_free (files);
}

static void
iog_window_update_openwith_menu (IogWindow *window, IogImage *image)
{
	gboolean edit_button_active;
	GAppInfo *editor_app;
	GFile *file;
	GFileInfo *file_info;
	GList *iter;
	gchar *label, *tip;
	const gchar *mime_type;
	GtkAction *action;
	IogWindowPrivate *priv;
	GList *apps;
	guint action_id = 0;
	GIcon *app_icon;
	char *path;
	GtkWidget *menuitem;

	priv = window->priv;

	edit_button_active = FALSE;
	editor_app = get_appinfo_for_editor (window);

	file = iog_image_get_file (image);
	file_info = g_file_query_info (file,
				       G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
				       0, NULL, NULL);

	if (file_info == NULL)
		return;
	else {
		mime_type = g_file_info_get_content_type (file_info);
	}

	if (priv->open_with_menu_id != 0) {
		gtk_ui_manager_remove_ui (priv->ui_mgr, priv->open_with_menu_id);
		priv->open_with_menu_id = 0;
	}

	if (priv->actions_open_with != NULL) {
		gtk_ui_manager_remove_action_group (priv->ui_mgr, priv->actions_open_with);
		priv->actions_open_with = NULL;
	}

	if (mime_type == NULL) {
		g_object_unref (file_info);
		return;
	}

	apps = g_app_info_get_all_for_type (mime_type);

	g_object_unref (file_info);

	if (!apps)
		return;

	priv->actions_open_with = gtk_action_group_new ("OpenWithActions");
	gtk_ui_manager_insert_action_group (priv->ui_mgr, priv->actions_open_with, -1);

	priv->open_with_menu_id = gtk_ui_manager_new_merge_id (priv->ui_mgr);

	for (iter = apps; iter; iter = iter->next) {
		GAppInfo *app = iter->data;
		gchar name[64];

		if (editor_app != NULL && g_app_info_equal (editor_app, app)) {
			edit_button_active = TRUE;
		}

		/* Do not include iog itself */
		if (g_ascii_strcasecmp (g_app_info_get_executable (app),
				                g_get_prgname ()) == 0) {
			g_object_unref (app);
			continue;
		}

		g_snprintf (name, sizeof (name), "OpenWith%u", action_id++);

		label = g_strdup (g_app_info_get_name (app));
		tip = g_strdup_printf (_("Use \"%s\" to open the selected image"), g_app_info_get_name (app));

		action = gtk_action_new (name, label, tip, NULL);

		app_icon = g_app_info_get_icon (app);
		if (G_LIKELY (app_icon != NULL)) {
			g_object_ref (app_icon);
			gtk_action_set_gicon (action, app_icon);
			g_object_unref (app_icon);
		}

		g_free (label);
		g_free (tip);

		g_object_set_data_full (G_OBJECT (action), "app", app,
				                (GDestroyNotify) g_object_unref);

		g_signal_connect (action,
				          "activate",
				          G_CALLBACK (open_with_launch_application_cb),
				          image);

		gtk_action_group_add_action (priv->actions_open_with, action);
		g_object_unref (action);

		gtk_ui_manager_add_ui (priv->ui_mgr,
				        priv->open_with_menu_id,
				        "/MainMenu/Image/ImageOpenWith/Applications Placeholder",
				        name,
				        name,
				        GTK_UI_MANAGER_MENUITEM,
				        FALSE);

		gtk_ui_manager_add_ui (priv->ui_mgr,
				        priv->open_with_menu_id,
				        "/ThumbnailPopup/ImageOpenWith/Applications Placeholder",
				        name,
				        name,
				        GTK_UI_MANAGER_MENUITEM,
				        FALSE);
		gtk_ui_manager_add_ui (priv->ui_mgr,
				        priv->open_with_menu_id,
				        "/ViewPopup/ImageOpenWith/Applications Placeholder",
				        name,
				        name,
				        GTK_UI_MANAGER_MENUITEM,
				        FALSE);

		path = g_strdup_printf ("/MainMenu/Image/ImageOpenWith/Applications Placeholder/%s", name);

		menuitem = gtk_ui_manager_get_widget (priv->ui_mgr, path);

		/* Only force displaying the icon if it is an application icon */
		gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (menuitem), app_icon != NULL);

		g_free (path);

		path = g_strdup_printf ("/ThumbnailPopup/ImageOpenWith/Applications Placeholder/%s", name);

		menuitem = gtk_ui_manager_get_widget (priv->ui_mgr, path);

		/* Only force displaying the icon if it is an application icon */
		gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (menuitem), app_icon != NULL);

		g_free (path);

		path = g_strdup_printf ("/ViewPopup/ImageOpenWith/Applications Placeholder/%s", name);

		menuitem = gtk_ui_manager_get_widget (priv->ui_mgr, path);

		/* Only force displaying the icon if it is an application icon */
		gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (menuitem), app_icon != NULL);

		g_free (path);
	}

	g_list_free (apps);

	action = gtk_action_group_get_action (window->priv->actions_image,
		                                  "OpenEditor");
	if (action != NULL) {
		gtk_action_set_sensitive (action, edit_button_active);
	}
}

static void
iog_window_clear_load_job (IogWindow *window)
{
	IogWindowPrivate *priv = window->priv;

	if (priv->load_job != NULL) {
		if (!priv->load_job->finished)
			iog_job_queue_remove_job (priv->load_job);

		g_signal_handlers_disconnect_by_func (priv->load_job,
						      iog_job_progress_cb,
						      window);

		g_signal_handlers_disconnect_by_func (priv->load_job,
						      iog_job_load_cb,
						      window);

		iog_image_cancel_load (IOG_JOB_LOAD (priv->load_job)->image);

		g_object_unref (priv->load_job);
		priv->load_job = NULL;

		/* Hide statusbar */
		iog_statusbar_set_progress (IOG_STATUSBAR (priv->statusbar), 0);
	}
}

static void
iog_job_progress_cb (IogJobLoad *job, float progress, gpointer user_data)
{
	IogWindow *window;

	g_return_if_fail (IOG_IS_WINDOW (user_data));

	window = IOG_WINDOW (user_data);

	iog_statusbar_set_progress (IOG_STATUSBAR (window->priv->statusbar),
				    progress);
}

static void
iog_job_save_progress_cb (IogJobSave *job, float progress, gpointer user_data)
{
	IogWindowPrivate *priv;
	IogWindow *window;

	static IogImage *image = NULL;

	g_return_if_fail (IOG_IS_WINDOW (user_data));

	window = IOG_WINDOW (user_data);
	priv = window->priv;

	iog_statusbar_set_progress (IOG_STATUSBAR (priv->statusbar),
				    progress);

	if (image != job->current_image) {
		gchar *str_image, *status_message;
		guint n_images;

		image = job->current_image;

		n_images = g_list_length (job->images);

		str_image = iog_image_get_uri_for_display (image);

		/* Translators: This string is displayed in the statusbar
		 * while saving images. The tokens are from left to right:
		 * - the original filename
		 * - the current image's position in the queue
		 * - the total number of images queued for saving */
		status_message = g_strdup_printf (_("Saving image \"%s\" (%u/%u)"),
					          str_image,
						  job->current_pos + 1,
						  n_images);
		g_free (str_image);

		gtk_statusbar_pop (GTK_STATUSBAR (priv->statusbar),
				   priv->image_info_message_cid);

		gtk_statusbar_push (GTK_STATUSBAR (priv->statusbar),
				    priv->image_info_message_cid,
				    status_message);

		g_free (status_message);
	}

	if (progress == 1.0)
		image = NULL;
}

static void
iog_window_obtain_desired_size (IogImage  *image,
				gint       width,
				gint       height,
				IogWindow *window)
{
	GdkScreen *screen;
	GdkRectangle monitor;
	GtkAllocation allocation;
	gint final_width, final_height;
	gint screen_width, screen_height;
	gint window_width, window_height;
	gint img_width, img_height;
	gint view_width, view_height;
	gint deco_width, deco_height;

	update_action_groups_state (window);

	img_width = width;
	img_height = height;

	if (!gtk_widget_get_realized (window->priv->view)) {
		gtk_widget_realize (window->priv->view);
	}

	gtk_widget_get_allocation (window->priv->view, &allocation);
	view_width  = allocation.width;
	view_height = allocation.height;

	if (!gtk_widget_get_realized (GTK_WIDGET (window))) {
		gtk_widget_realize (GTK_WIDGET (window));
	}

	gtk_widget_get_allocation (GTK_WIDGET (window), &allocation);
	window_width  = allocation.width;
	window_height = allocation.height;

	screen = gtk_window_get_screen (GTK_WINDOW (window));

	gdk_screen_get_monitor_giogetry (screen,
			gdk_screen_get_monitor_at_window (screen,
				gtk_widget_get_window (GTK_WIDGET (window))),
			&monitor);

	screen_width  = monitor.width;
	screen_height = monitor.height;

	deco_width = window_width - view_width;
	deco_height = window_height - view_height;

	if (img_width > 0 && img_height > 0) {
		if ((img_width + deco_width > screen_width) ||
		    (img_height + deco_height > screen_height))
		{
			double factor;

			if (img_width > img_height) {
				factor = (screen_width * 0.75 - deco_width) / (double) img_width;
			} else {
				factor = (screen_height * 0.75 - deco_height) / (double) img_height;
			}

			img_width = img_width * factor;
			img_height = img_height * factor;
		}
	}

	final_width = MAX (IOG_WINDOW_MIN_WIDTH, img_width + deco_width);
	final_height = MAX (IOG_WINDOW_MIN_HEIGHT, img_height + deco_height);

	iog_debug_message (DEBUG_WINDOW, "Setting window size: %d x %d", final_width, final_height);

	gtk_window_set_default_size (GTK_WINDOW (window), final_width, final_height);

	g_signal_emit (window, signals[SIGNAL_PREPARED], 0);
}

static void
iog_window_error_message_area_response (GtkInfoBar       *message_area,
					gint              response_id,
					IogWindow        *window)
{
	if (response_id != GTK_RESPONSE_OK) {
		iog_window_set_message_area (window, NULL);

		return;
	}

	/* Trigger loading for current image again */
	iog_thumb_view_select_single (IOG_THUMB_VIEW (window->priv->thumbview),
				      IOG_THUMB_VIEW_SELECT_CURRENT);
}

static void
iog_job_load_cb (IogJobLoad *job, gpointer data)
{
	IogWindow *window;
	IogWindowPrivate *priv;
	GtkAction *action_undo, *action_save;

	g_return_if_fail (IOG_IS_WINDOW (data));

	iog_debug (DEBUG_WINDOW);

	window = IOG_WINDOW (data);
	priv = window->priv;

	iog_statusbar_set_progress (IOG_STATUSBAR (priv->statusbar), 0.0);

	gtk_statusbar_pop (GTK_STATUSBAR (window->priv->statusbar),
			   priv->image_info_message_cid);

	if (priv->image != NULL) {
		g_signal_handlers_disconnect_by_func (priv->image,
						      image_thumb_changed_cb,
						      window);
		g_signal_handlers_disconnect_by_func (priv->image,
						      image_file_changed_cb,
						      window);

		g_object_unref (priv->image);
	}

	priv->image = g_object_ref (job->image);

	if (IOG_JOB (job)->error == NULL) {
#ifdef HAVE_LCMS
		iog_image_apply_display_profile (job->image,
						 priv->display_profile);
#endif

		gtk_action_group_set_sensitive (priv->actions_image, TRUE);

		iog_window_display_image (window, job->image);
	} else {
		GtkWidget *message_area;

		message_area = iog_image_load_error_message_area_new (
					iog_image_get_caption (job->image),
					IOG_JOB (job)->error);

		g_signal_connect (message_area,
				  "response",
				  G_CALLBACK (iog_window_error_message_area_response),
				  window);

		gtk_window_set_icon (GTK_WINDOW (window), NULL);
		gtk_window_set_title (GTK_WINDOW (window),
				      iog_image_get_caption (job->image));

		iog_window_set_message_area (window, message_area);

		gtk_info_bar_set_default_response (GTK_INFO_BAR (message_area),
						   GTK_RESPONSE_CANCEL);

		gtk_widget_show (message_area);

		update_status_bar (window);

		iog_scroll_view_set_image (IOG_SCROLL_VIEW (priv->view), NULL);

        	if (window->priv->status == IOG_WINDOW_STATUS_INIT) {
			update_action_groups_state (window);

			g_signal_emit (window, signals[SIGNAL_PREPARED], 0);
		}

		gtk_action_group_set_sensitive (priv->actions_image, FALSE);
	}

	iog_window_clear_load_job (window);

	if (window->priv->status == IOG_WINDOW_STATUS_INIT) {
		window->priv->status = IOG_WINDOW_STATUS_NORMAL;

		g_signal_handlers_disconnect_by_func
			(job->image,
			 G_CALLBACK (iog_window_obtain_desired_size),
			 window);
	}

	action_save = gtk_action_group_get_action (priv->actions_image, "ImageSave");
	action_undo = gtk_action_group_get_action (priv->actions_image, "EditUndo");

	/* Set Save and Undo sensitive according to image state.
	 * Respect lockdown in case of Save.*/
	gtk_action_set_sensitive (action_save, (!priv->save_disabled && iog_image_is_modified (job->image)));
	gtk_action_set_sensitive (action_undo, iog_image_is_modified (job->image));

	g_object_unref (job->image);
}

static void
iog_window_clear_transform_job (IogWindow *window)
{
	IogWindowPrivate *priv = window->priv;

	if (priv->transform_job != NULL) {
		if (!priv->transform_job->finished)
			iog_job_queue_remove_job (priv->transform_job);

		g_signal_handlers_disconnect_by_func (priv->transform_job,
						      iog_job_transform_cb,
						      window);
		g_object_unref (priv->transform_job);
		priv->transform_job = NULL;
	}
}

static void
iog_job_transform_cb (IogJobTransform *job, gpointer data)
{
	IogWindow *window;
	GtkAction *action_undo, *action_save;
	IogImage *image;

	g_return_if_fail (IOG_IS_WINDOW (data));

	window = IOG_WINDOW (data);

	iog_window_clear_transform_job (window);

	action_undo =
		gtk_action_group_get_action (window->priv->actions_image, "EditUndo");
	action_save =
		gtk_action_group_get_action (window->priv->actions_image, "ImageSave");

	image = iog_window_get_image (window);

	gtk_action_set_sensitive (action_undo, iog_image_is_modified (image));

	if (!window->priv->save_disabled)
	{
		gtk_action_set_sensitive (action_save, iog_image_is_modified (image));
	}
}

static void
apply_transformation (IogWindow *window, IogTransform *trans)
{
	IogWindowPrivate *priv;
	GList *images;

	g_return_if_fail (IOG_IS_WINDOW (window));

	priv = window->priv;

	images = iog_thumb_view_get_selected_images (IOG_THUMB_VIEW (priv->thumbview));

	iog_window_clear_transform_job (window);

	priv->transform_job = iog_job_transform_new (images, trans);

	g_signal_connect (priv->transform_job,
			  "finished",
			  G_CALLBACK (iog_job_transform_cb),
			  window);

	g_signal_connect (priv->transform_job,
			  "progress",
			  G_CALLBACK (iog_job_progress_cb),
			  window);

	iog_job_queue_add_job (priv->transform_job);
}

static void
handle_image_selection_changed_cb (IogThumbView *thumbview, IogWindow *window)
{
	IogWindowPrivate *priv;
	IogImage *image;
	gchar *status_message;
	gchar *str_image;

	priv = window->priv;

	if (iog_list_store_length (IOG_LIST_STORE (priv->store)) == 0) {
		gtk_window_set_title (GTK_WINDOW (window),
				      g_get_application_name());
		gtk_statusbar_remove_all (GTK_STATUSBAR (priv->statusbar),
					  priv->image_info_message_cid);
		iog_scroll_view_set_image (IOG_SCROLL_VIEW (priv->view),
					   NULL);
	}

	if (iog_thumb_view_get_n_selected (IOG_THUMB_VIEW (priv->thumbview)) == 0)
		return;

	update_selection_ui_visibility (window);

	image = iog_thumb_view_get_first_selected_image (IOG_THUMB_VIEW (priv->thumbview));

	g_assert (IOG_IS_IMAGE (image));

	iog_window_clear_load_job (window);

	iog_window_set_message_area (window, NULL);

	gtk_statusbar_pop (GTK_STATUSBAR (priv->statusbar),
			   priv->image_info_message_cid);

	if (image == priv->image) {
		update_status_bar (window);
		return;
	}

	if (iog_image_has_data (image, IOG_IMAGE_DATA_IMAGE)) {
		if (priv->image != NULL)
			g_object_unref (priv->image);

		priv->image = image;
		iog_window_display_image (window, image);
		return;
	}

	if (priv->status == IOG_WINDOW_STATUS_INIT) {
		g_signal_connect (image,
				  "size-prepared",
				  G_CALLBACK (iog_window_obtain_desired_size),
				  window);
	}

	priv->load_job = iog_job_load_new (image, IOG_IMAGE_DATA_ALL);

	g_signal_connect (priv->load_job,
			  "finished",
			  G_CALLBACK (iog_job_load_cb),
			  window);

	g_signal_connect (priv->load_job,
			  "progress",
			  G_CALLBACK (iog_job_progress_cb),
			  window);

	iog_job_queue_add_job (priv->load_job);

	str_image = iog_image_get_uri_for_display (image);

	status_message = g_strdup_printf (_("Opening image \"%s\""),
				          str_image);

	g_free (str_image);

	gtk_statusbar_push (GTK_STATUSBAR (priv->statusbar),
			    priv->image_info_message_cid, status_message);

	g_free (status_message);
}

static void
view_zoom_changed_cb (GtkWidget *widget, double zoom, gpointer user_data)
{
	IogWindow *window;
	GtkAction *action_zoom_in;
	GtkAction *action_zoom_out;

	g_return_if_fail (IOG_IS_WINDOW (user_data));

	window = IOG_WINDOW (user_data);

	update_status_bar (window);

	action_zoom_in =
		gtk_action_group_get_action (window->priv->actions_image,
					     "ViewZoomIn");

	action_zoom_out =
		gtk_action_group_get_action (window->priv->actions_image,
					     "ViewZoomOut");

	gtk_action_set_sensitive (action_zoom_in,
			!iog_scroll_view_get_zoom_is_max (IOG_SCROLL_VIEW (window->priv->view)));
	gtk_action_set_sensitive (action_zoom_out,
			!iog_scroll_view_get_zoom_is_min (IOG_SCROLL_VIEW (window->priv->view)));
}

static void
iog_window_open_recent_cb (GtkAction *action, IogWindow *window)
{
	GtkRecentInfo *info;
	const gchar *uri;
	GSList *list = NULL;

	info = g_object_get_data (G_OBJECT (action), "gtk-recent-info");
	g_return_if_fail (info != NULL);

	uri = gtk_recent_info_get_uri (info);
	list = g_slist_prepend (list, g_strdup (uri));

	iog_application_open_uri_list (IOG_APP,
				       list,
				       GDK_CURRENT_TIME,
				       0,
				       NULL);

	g_slist_foreach (list, (GFunc) g_free, NULL);
	g_slist_free (list);
}

static void
file_open_dialog_response_cb (GtkWidget *chooser,
			      gint       response_id,
			      IogWindow  *ev_window)
{
	if (response_id == GTK_RESPONSE_OK) {
		GSList *uris;

		uris = gtk_file_chooser_get_uris (GTK_FILE_CHOOSER (chooser));

		iog_application_open_uri_list (IOG_APP,
					       uris,
					       GDK_CURRENT_TIME,
					       0,
					       NULL);

		g_slist_foreach (uris, (GFunc) g_free, NULL);
		g_slist_free (uris);
	}

	gtk_widget_destroy (chooser);
}

static void
iog_window_update_fullscreen_action (IogWindow *window)
{
	GtkAction *action;

	action = gtk_action_group_get_action (window->priv->actions_image,
					      "ViewFullscreen");

	g_signal_handlers_block_by_func
		(action, G_CALLBACK (iog_window_cmd_fullscreen), window);

	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
				      window->priv->mode == IOG_WINDOW_MODE_FULLSCREEN);

	g_signal_handlers_unblock_by_func
		(action, G_CALLBACK (iog_window_cmd_fullscreen), window);
}

static void
iog_window_update_slideshow_action (IogWindow *window)
{
	GtkAction *action;

	action = gtk_action_group_get_action (window->priv->actions_collection,
					      "ViewSlideshow");

	g_signal_handlers_block_by_func
		(action, G_CALLBACK (iog_window_cmd_slideshow), window);

	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
				      window->priv->mode == IOG_WINDOW_MODE_SLIDESHOW);

	g_signal_handlers_unblock_by_func
		(action, G_CALLBACK (iog_window_cmd_slideshow), window);
}

static void
iog_window_update_pause_slideshow_action (IogWindow *window)
{
	GtkAction *action;

	action = gtk_action_group_get_action (window->priv->actions_image,
					      "PauseSlideshow");

	g_signal_handlers_block_by_func
		(action, G_CALLBACK (iog_window_cmd_pause_slideshow), window);

	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
				      window->priv->mode != IOG_WINDOW_MODE_SLIDESHOW);

	g_signal_handlers_unblock_by_func
		(action, G_CALLBACK (iog_window_cmd_pause_slideshow), window);
}

static void
iog_window_update_fullscreen_popup (IogWindow *window)
{
	GtkWidget *popup = window->priv->fullscreen_popup;
	GdkRectangle screen_rect;
	GdkScreen *screen;

	g_return_if_fail (popup != NULL);

	if (gtk_widget_get_window (GTK_WIDGET (window)) == NULL) return;

	screen = gtk_widget_get_screen (GTK_WIDGET (window));

	gdk_screen_get_monitor_giogetry (screen,
			gdk_screen_get_monitor_at_window
                        (screen,
                         gtk_widget_get_window (GTK_WIDGET (window))),
                         &screen_rect);

	gtk_widget_set_size_request (popup,
				     screen_rect.width,
				     -1);

	gtk_window_move (GTK_WINDOW (popup), screen_rect.x, screen_rect.y);
}

static void
screen_size_changed_cb (GdkScreen *screen, IogWindow *window)
{
	iog_window_update_fullscreen_popup (window);
}

static gboolean
fullscreen_timeout_cb (gpointer data)
{
	IogWindow *window = IOG_WINDOW (data);

	gtk_widget_hide (window->priv->fullscreen_popup);

	iog_scroll_view_hide_cursor (IOG_SCROLL_VIEW (window->priv->view));

	fullscreen_clear_timeout (window);

	return FALSE;
}

static gboolean
slideshow_is_loop_end (IogWindow *window)
{
	IogWindowPrivate *priv = window->priv;
	IogImage *image = NULL;
	gint pos;

	image = iog_thumb_view_get_first_selected_image (IOG_THUMB_VIEW (priv->thumbview));

	pos = iog_list_store_get_pos_by_image (priv->store, image);

	return (pos == (iog_list_store_length (priv->store) - 1));
}

static gboolean
slideshow_switch_cb (gpointer data)
{
	IogWindow *window = IOG_WINDOW (data);
	IogWindowPrivate *priv = window->priv;

	iog_debug (DEBUG_WINDOW);

	if (priv->slideshow_random) {
		iog_thumb_view_select_single (IOG_THUMB_VIEW (priv->thumbview),
					      IOG_THUMB_VIEW_SELECT_RANDOM);
		return TRUE;
	}

	if (!priv->slideshow_loop && slideshow_is_loop_end (window)) {
		iog_window_stop_fullscreen (window, TRUE);
		return FALSE;
	}

	iog_thumb_view_select_single (IOG_THUMB_VIEW (priv->thumbview),
				      IOG_THUMB_VIEW_SELECT_RIGHT);

	return TRUE;
}

static void
fullscreen_clear_timeout (IogWindow *window)
{
	iog_debug (DEBUG_WINDOW);

	if (window->priv->fullscreen_timeout_source != NULL) {
		g_source_unref (window->priv->fullscreen_timeout_source);
		g_source_destroy (window->priv->fullscreen_timeout_source);
	}

	window->priv->fullscreen_timeout_source = NULL;
}

static void
fullscreen_set_timeout (IogWindow *window)
{
	GSource *source;

	iog_debug (DEBUG_WINDOW);

	fullscreen_clear_timeout (window);

	source = g_timeout_source_new (IOG_WINDOW_FULLSCREEN_TIMEOUT);
	g_source_set_callback (source, fullscreen_timeout_cb, window, NULL);

	g_source_attach (source, NULL);

	window->priv->fullscreen_timeout_source = source;

	iog_scroll_view_show_cursor (IOG_SCROLL_VIEW (window->priv->view));
}

static void
slideshow_clear_timeout (IogWindow *window)
{
	iog_debug (DEBUG_WINDOW);

	if (window->priv->slideshow_switch_source != NULL) {
		g_source_unref (window->priv->slideshow_switch_source);
		g_source_destroy (window->priv->slideshow_switch_source);
	}

	window->priv->slideshow_switch_source = NULL;
}

static void
slideshow_set_timeout (IogWindow *window)
{
	GSource *source;

	iog_debug (DEBUG_WINDOW);

	slideshow_clear_timeout (window);

	if (window->priv->slideshow_switch_timeout <= 0)
		return;

	source = g_timeout_source_new (window->priv->slideshow_switch_timeout * 1000);
	g_source_set_callback (source, slideshow_switch_cb, window, NULL);

	g_source_attach (source, NULL);

	window->priv->slideshow_switch_source = source;
}

static void
show_fullscreen_popup (IogWindow *window)
{
	iog_debug (DEBUG_WINDOW);

	if (!gtk_widget_get_visible (window->priv->fullscreen_popup)) {
		gtk_widget_show_all (GTK_WIDGET (window->priv->fullscreen_popup));
	}

	fullscreen_set_timeout (window);
}

static gboolean
fullscreen_motion_notify_cb (GtkWidget      *widget,
			     GdkEventMotion *event,
			     gpointer       user_data)
{
	IogWindow *window = IOG_WINDOW (user_data);

	iog_debug (DEBUG_WINDOW);

	if (event->y < IOG_WINDOW_FULLSCREEN_POPUP_THRESHOLD) {
		show_fullscreen_popup (window);
	} else {
		fullscreen_set_timeout (window);
	}

	return FALSE;
}

static gboolean
fullscreen_leave_notify_cb (GtkWidget *widget,
			    GdkEventCrossing *event,
			    gpointer user_data)
{
	IogWindow *window = IOG_WINDOW (user_data);

	iog_debug (DEBUG_WINDOW);

	fullscreen_clear_timeout (window);

	return FALSE;
}

static void
exit_fullscreen_button_clicked_cb (GtkWidget *button, IogWindow *window)
{
	GtkAction *action;

	iog_debug (DEBUG_WINDOW);

	if (window->priv->mode == IOG_WINDOW_MODE_SLIDESHOW) {
		action = gtk_action_group_get_action (window->priv->actions_collection,
						      "ViewSlideshow");
	} else {
		action = gtk_action_group_get_action (window->priv->actions_image,
						      "ViewFullscreen");
	}
	g_return_if_fail (action != NULL);

	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), FALSE);
}

static GtkWidget *
iog_window_get_exit_fullscreen_button (IogWindow *window)
{
	GtkWidget *button;

	button = gtk_button_new_from_stock (GTK_STOCK_LEAVE_FULLSCREEN);

	g_signal_connect (button, "clicked",
			  G_CALLBACK (exit_fullscreen_button_clicked_cb),
			  window);

	return button;
}

static GtkWidget *
iog_window_create_fullscreen_popup (IogWindow *window)
{
	GtkWidget *popup;
	GtkWidget *hbox;
	GtkWidget *button;
	GtkWidget *toolbar;
	GdkScreen *screen;

	iog_debug (DEBUG_WINDOW);

	popup = gtk_window_new (GTK_WINDOW_POPUP);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (popup), hbox);

	toolbar = gtk_ui_manager_get_widget (window->priv->ui_mgr,
					     "/FullscreenToolbar");
	g_assert (GTK_IS_WIDGET (toolbar));
	gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
	gtk_box_pack_start (GTK_BOX (hbox), toolbar, TRUE, TRUE, 0);

	button = iog_window_get_exit_fullscreen_button (window);
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

	gtk_window_set_resizable (GTK_WINDOW (popup), FALSE);

	screen = gtk_widget_get_screen (GTK_WIDGET (window));

	g_signal_connect_object (screen, "size-changed",
			         G_CALLBACK (screen_size_changed_cb),
				 window, 0);

	g_signal_connect (popup,
			  "enter-notify-event",
			  G_CALLBACK (fullscreen_leave_notify_cb),
			  window);

	gtk_window_set_screen (GTK_WINDOW (popup), screen);

	return popup;
}

static void
update_ui_visibility (IogWindow *window)
{
	IogWindowPrivate *priv;

	GtkAction *action;
	GtkWidget *menubar;

	gboolean fullscreen_mode, visible;

	g_return_if_fail (IOG_IS_WINDOW (window));

	iog_debug (DEBUG_WINDOW);

	priv = window->priv;

	fullscreen_mode = priv->mode == IOG_WINDOW_MODE_FULLSCREEN ||
			  priv->mode == IOG_WINDOW_MODE_SLIDESHOW;

	menubar = gtk_ui_manager_get_widget (priv->ui_mgr, "/MainMenu");
	g_assert (GTK_IS_WIDGET (menubar));

	visible = g_settings_get_boolean (priv->ui_settings, IOG_CONF_UI_TOOLBAR);
	visible = visible && !fullscreen_mode;

	action = gtk_ui_manager_get_action (priv->ui_mgr, "/MainMenu/View/ToolbarToggle");
	g_assert (action != NULL);
	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), visible);
	g_object_set (G_OBJECT (priv->toolbar), "visible", visible, NULL);

	visible = g_settings_get_boolean (priv->ui_settings, IOG_CONF_UI_STATUSBAR);
	visible = visible && !fullscreen_mode;

	action = gtk_ui_manager_get_action (priv->ui_mgr, "/MainMenu/View/StatusbarToggle");
	g_assert (action != NULL);
	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), visible);
	g_object_set (G_OBJECT (priv->statusbar), "visible", visible, NULL);

	if (priv->status != IOG_WINDOW_STATUS_INIT) {
		visible = g_settings_get_boolean (priv->ui_settings, IOG_CONF_UI_IMAGE_COLLECTION);
		visible = visible && priv->mode != IOG_WINDOW_MODE_SLIDESHOW;
		action = gtk_ui_manager_get_action (priv->ui_mgr, "/MainMenu/View/ImageCollectionToggle");
		g_assert (action != NULL);
		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), visible);
		if (visible) {
			gtk_widget_show (priv->nav);
		} else {
			gtk_widget_hide (priv->nav);
		}
	}

	visible = g_settings_get_boolean (priv->ui_settings, IOG_CONF_UI_SIDEBAR);
	visible = visible && !fullscreen_mode;
	action = gtk_ui_manager_get_action (priv->ui_mgr, "/MainMenu/View/SidebarToggle");
	g_assert (action != NULL);
	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), visible);
	if (visible) {
		gtk_widget_show (priv->sidebar);
	} else {
		gtk_widget_hide (priv->sidebar);
	}

	if (priv->fullscreen_popup != NULL) {
		gtk_widget_hide (priv->fullscreen_popup);
	}
}

static void
iog_window_run_fullscreen (IogWindow *window, gboolean slideshow)
{
#if GTK_CHECK_VERSION (3, 0, 0)
	static const GdkRGBA black = { 0., 0., 0., 1.};
#endif
	IogWindowPrivate *priv;
	GtkWidget *menubar;
	gboolean upscale;

	iog_debug (DEBUG_WINDOW);

	priv = window->priv;

	if (slideshow) {
		priv->mode = IOG_WINDOW_MODE_SLIDESHOW;
	} else {
		/* Stop the timer if we come from slideshowing */
		if (priv->mode == IOG_WINDOW_MODE_SLIDESHOW)
			slideshow_clear_timeout (window);

		priv->mode = IOG_WINDOW_MODE_FULLSCREEN;
	}

	if (window->priv->fullscreen_popup == NULL)
		priv->fullscreen_popup
			= iog_window_create_fullscreen_popup (window);

	update_ui_visibility (window);

	menubar = gtk_ui_manager_get_widget (priv->ui_mgr, "/MainMenu");
	g_assert (GTK_IS_WIDGET (menubar));
	gtk_widget_hide (menubar);

	g_signal_connect (priv->view,
			  "motion-notify-event",
			  G_CALLBACK (fullscreen_motion_notify_cb),
			  window);

	g_signal_connect (priv->view,
			  "leave-notify-event",
			  G_CALLBACK (fullscreen_leave_notify_cb),
			  window);

	g_signal_connect (priv->thumbview,
			  "motion-notify-event",
			  G_CALLBACK (fullscreen_motion_notify_cb),
			  window);

	g_signal_connect (priv->thumbview,
			  "leave-notify-event",
			  G_CALLBACK (fullscreen_leave_notify_cb),
			  window);

	fullscreen_set_timeout (window);

	if (slideshow) {
		priv->slideshow_random =
				g_settings_get_boolean (priv->fullscreen_settings,
						       IOG_CONF_FULLSCREEN_RANDOM);

		priv->slideshow_loop =
				g_settings_get_boolean (priv->fullscreen_settings,
						       IOG_CONF_FULLSCREEN_LOOP);

		priv->slideshow_switch_timeout =
				g_settings_get_int (priv->fullscreen_settings,
						      IOG_CONF_FULLSCREEN_SECONDS);

		slideshow_set_timeout (window);
	}

	upscale = g_settings_get_boolean (priv->fullscreen_settings,
					 IOG_CONF_FULLSCREEN_UPSCALE);

	iog_scroll_view_set_zoom_upscale (IOG_SCROLL_VIEW (priv->view),
					  upscale);

	gtk_widget_grab_focus (priv->view);

	iog_scroll_view_override_bg_color (IOG_SCROLL_VIEW (window->priv->view),
#if GTK_CHECK_VERSION (3, 0, 0)
	                                   &black);
#else
			  &(gtk_widget_get_style (GTK_WIDGET (window))->black));
#endif

#if !GTK_CHECK_VERSION (3, 0, 0)
	{
		GtkStyle *style;

		style = gtk_style_copy (gtk_widget_get_style (gtk_widget_get_parent (priv->view)));

		style->xthickness = 0;
		style->ythickness = 0;

		gtk_widget_set_style (gtk_widget_get_parent (priv->view),
				      style);

		g_object_unref (style);
	}
#endif

	gtk_window_fullscreen (GTK_WINDOW (window));
	iog_window_update_fullscreen_popup (window);

#ifdef HAVE_DBUS
	iog_application_screensaver_disable (IOG_APP);
#endif

	/* Update both actions as we could've already been in one those modes */
	iog_window_update_slideshow_action (window);
	iog_window_update_fullscreen_action (window);
	iog_window_update_pause_slideshow_action (window);
}

static void
iog_window_stop_fullscreen (IogWindow *window, gboolean slideshow)
{
	IogWindowPrivate *priv;
	GtkWidget *menubar;

	iog_debug (DEBUG_WINDOW);

	priv = window->priv;

	if (priv->mode != IOG_WINDOW_MODE_SLIDESHOW &&
	    priv->mode != IOG_WINDOW_MODE_FULLSCREEN) return;

	priv->mode = IOG_WINDOW_MODE_NORMAL;

	fullscreen_clear_timeout (window);

	if (slideshow) {
		slideshow_clear_timeout (window);
	}

	g_signal_handlers_disconnect_by_func (priv->view,
					      (gpointer) fullscreen_motion_notify_cb,
					      window);

	g_signal_handlers_disconnect_by_func (priv->view,
					      (gpointer) fullscreen_leave_notify_cb,
					      window);

	g_signal_handlers_disconnect_by_func (priv->thumbview,
					      (gpointer) fullscreen_motion_notify_cb,
					      window);

	g_signal_handlers_disconnect_by_func (priv->thumbview,
					      (gpointer) fullscreen_leave_notify_cb,
					      window);

	update_ui_visibility (window);

	menubar = gtk_ui_manager_get_widget (priv->ui_mgr, "/MainMenu");
	g_assert (GTK_IS_WIDGET (menubar));
	gtk_widget_show (menubar);

	iog_scroll_view_set_zoom_upscale (IOG_SCROLL_VIEW (priv->view), FALSE);

	iog_scroll_view_override_bg_color (IOG_SCROLL_VIEW (window->priv->view),
					   NULL);
#if !GTK_CHECK_VERSION (3, 0, 0)
	gtk_widget_set_style (gtk_widget_get_parent (window->priv->view), NULL);
#endif
	gtk_window_unfullscreen (GTK_WINDOW (window));

	if (slideshow) {
		iog_window_update_slideshow_action (window);
	} else {
		iog_window_update_fullscreen_action (window);
	}

	iog_scroll_view_show_cursor (IOG_SCROLL_VIEW (priv->view));

#ifdef HAVE_DBUS
	iog_application_screensaver_enable (IOG_APP);
#endif
}

static void
iog_window_print (IogWindow *window)
{
	GtkWidget *dialog;
	GError *error = NULL;
	GtkPrintOperation *print;
	GtkPrintOperationResult res;
	GtkPageSetup *page_setup;
	GtkPrintSettings *print_settings;
	gboolean page_setup_disabled = FALSE;

	iog_debug (DEBUG_PRINTING);

	print_settings = iog_print_get_print_settings ();

	/* Make sure the window stays valid while printing */
	g_object_ref (window);

	if (window->priv->page_setup !=NULL)
		page_setup = g_object_ref (window->priv->page_setup);
	else
		page_setup = NULL;

	print = iog_print_operation_new (window->priv->image,
					 print_settings,
					 page_setup);

	// Disable page setup options if they are locked down
	page_setup_disabled = g_settings_get_boolean (window->priv->lockdown_settings,
						      IOG_CONF_LOCKDOWN_CAN_SETUP_PAGE);
	if (page_setup_disabled)
		gtk_print_operation_set_embed_page_setup (print, FALSE);

	res = gtk_print_operation_run (print,
				       GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
				       GTK_WINDOW (window), &error);

	if (res == GTK_PRINT_OPERATION_RESULT_ERROR) {
		dialog = gtk_message_dialog_new (GTK_WINDOW (window),
						 GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_CLOSE,
						 _("Error printing file:\n%s"),
						 error->message);
		g_signal_connect (dialog, "response",
				  G_CALLBACK (gtk_widget_destroy), NULL);
		gtk_widget_show (dialog);
		g_error_free (error);
	} else if (res == GTK_PRINT_OPERATION_RESULT_APPLY) {
		GtkPageSetup *new_page_setup;
		iog_print_set_print_settings (gtk_print_operation_get_print_settings (print));
		new_page_setup = gtk_print_operation_get_default_page_setup (print);
		if (window->priv->page_setup != NULL)
			g_object_unref (window->priv->page_setup);
		window->priv->page_setup = g_object_ref (new_page_setup);
	}

	if (page_setup != NULL)
		g_object_unref (page_setup);
	g_object_unref (print_settings);
	g_object_unref (window);
}

static void
iog_window_cmd_file_open (GtkAction *action, gpointer user_data)
{
	IogWindow *window;
	IogWindowPrivate *priv;
        IogImage *current;
	GtkWidget *dlg;

	g_return_if_fail (IOG_IS_WINDOW (user_data));

	window = IOG_WINDOW (user_data);

        priv = window->priv;

	dlg = iog_file_chooser_new (GTK_FILE_CHOOSER_ACTION_OPEN);

	current = iog_thumb_view_get_first_selected_image (IOG_THUMB_VIEW (priv->thumbview));

	if (current != NULL) {
		gchar *dir_uri, *file_uri;

		file_uri = iog_image_get_uri_for_display (current);
		dir_uri = g_path_get_dirname (file_uri);

	        gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (dlg),
                                                         dir_uri);
		g_free (file_uri);
		g_free (dir_uri);
		g_object_unref (current);
	} else {
		/* If desired by the user,
		   fallback to the XDG_PICTURES_DIR (if available) */
		const gchar *pics_dir;
		gboolean use_fallback;

		use_fallback = g_settings_get_boolean (priv->ui_settings,
					   IOG_CONF_UI_FILECHOOSER_XDG_FALLBACK);
		pics_dir = g_get_user_special_dir (G_USER_DIRECTORY_PICTURES);
		if (use_fallback && pics_dir) {
			gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dlg),
							     pics_dir);
		}
	}

	g_signal_connect (dlg, "response",
			  G_CALLBACK (file_open_dialog_response_cb),
			  window);

	gtk_widget_show_all (dlg);
}

static void
iog_job_close_save_cb (IogJobSave *job, gpointer user_data)
{
	IogWindow *window = IOG_WINDOW (user_data);

	g_signal_handlers_disconnect_by_func (job,
					      iog_job_close_save_cb,
					      window);

	gtk_widget_destroy (GTK_WIDGET (window));
}

static void
close_confirmation_dialog_response_handler (IogCloseConfirmationDialog *dlg,
					    gint                        response_id,
					    IogWindow                  *window)
{
	GList *selected_images;
	IogWindowPrivate *priv;

	priv = window->priv;

	switch (response_id)
	{
		case GTK_RESPONSE_YES:
			/* save selected images */
			selected_images = iog_close_confirmation_dialog_get_selected_images (dlg);
			iog_close_confirmation_dialog_set_sensitive (dlg, FALSE);
			if (iog_window_save_images (window, selected_images)) {
				g_signal_connect (priv->save_job,
							  "finished",
							  G_CALLBACK (iog_job_close_save_cb),
							  window);

				iog_job_queue_add_job (priv->save_job);
			}

			break;

		case GTK_RESPONSE_NO:
			/* dont save */
			gtk_widget_destroy (GTK_WIDGET (window));
			break;

		default:
			/* Cancel */
			gtk_widget_destroy (GTK_WIDGET (dlg));
			break;
	}	
}

static gboolean
iog_window_unsaved_images_confirm (IogWindow *window)
{
	IogWindowPrivate *priv;
	gboolean disabled;
	GtkWidget *dialog;
	GList *list;
	IogImage *image;
	GtkTreeIter iter;

	priv = window->priv;

	disabled = g_settings_get_boolean(priv->ui_settings,
					IOG_CONF_UI_DISABLE_CLOSE_CONFIRMATION);
	disabled |= window->priv->save_disabled;
	if (disabled) {
		return FALSE;
	}

	list = NULL;
	if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->store), &iter)) {
		do {
			gtk_tree_model_get (GTK_TREE_MODEL (priv->store), &iter,
					    IOG_LIST_STORE_IOG_IMAGE, &image,
					    -1);
			if (!image)
				continue;

			if (iog_image_is_modified (image)) {
				list = g_list_prepend (list, image);
			}
		} while (gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->store), &iter));
	}		

	if (list) {
		list = g_list_reverse (list);
		dialog = iog_close_confirmation_dialog_new (GTK_WINDOW (window),
							    list);

		g_list_free (list);
		g_signal_connect (dialog,
				  "response",
				  G_CALLBACK (close_confirmation_dialog_response_handler),
				  window);
		gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);

		gtk_widget_show (dialog);
		return TRUE;

	}
	return FALSE;
}

static void
iog_window_cmd_close_window (GtkAction *action, gpointer user_data)
{
	IogWindow *window;
	IogWindowPrivate *priv;

	g_return_if_fail (IOG_IS_WINDOW (user_data));

	window = IOG_WINDOW (user_data);
	priv = window->priv;

	if (priv->save_job != NULL) {
		iog_window_finish_saving (window);
	}

	if (!iog_window_unsaved_images_confirm (window)) {
		gtk_widget_destroy (GTK_WIDGET (user_data));
	}
}

static void
iog_window_cmd_preferences (GtkAction *action, gpointer user_data)
{
	IogWindow *window;
	GObject *pref_dlg;

	g_return_if_fail (IOG_IS_WINDOW (user_data));

	window = IOG_WINDOW (user_data);

	pref_dlg = iog_preferences_dialog_get_instance (GTK_WINDOW (window));

	iog_dialog_show (IOG_DIALOG (pref_dlg));
}

#define IOG_TB_EDITOR_DLG_RESET_RESPONSE 128

static void
iog_window_cmd_edit_toolbar_cb (GtkDialog *dialog, gint response, gpointer data)
{
	IogWindow *window = IOG_WINDOW (data);

	if (response == IOG_TB_EDITOR_DLG_RESET_RESPONSE) {
		EggToolbarsModel *model;
		EggToolbarEditor *editor;

		editor = g_object_get_data (G_OBJECT (dialog),
					    "EggToolbarEditor");

		g_return_if_fail (editor != NULL);

        	egg_editable_toolbar_set_edit_mode
			(EGG_EDITABLE_TOOLBAR (window->priv->toolbar), FALSE);

		iog_application_reset_toolbars_model (IOG_APP);
		model = iog_application_get_toolbars_model (IOG_APP);
		egg_editable_toolbar_set_model
			(EGG_EDITABLE_TOOLBAR (window->priv->toolbar), model);
		egg_toolbar_editor_set_model (editor, model);

		/* Toolbar would be uneditable now otherwise */
		egg_editable_toolbar_set_edit_mode
			(EGG_EDITABLE_TOOLBAR (window->priv->toolbar), TRUE);
	} else if (response == GTK_RESPONSE_HELP) {
		iog_util_show_help ("iog-toolbareditor", NULL);
	} else {
        	egg_editable_toolbar_set_edit_mode
			(EGG_EDITABLE_TOOLBAR (window->priv->toolbar), FALSE);

		iog_application_save_toolbars_model (IOG_APP);

        	gtk_widget_destroy (GTK_WIDGET (dialog));
	}
}

static void
iog_window_cmd_edit_toolbar (GtkAction *action, gpointer *user_data)
{
	IogWindow *window;
	GtkWidget *dialog;
	GtkWidget *editor;

	g_return_if_fail (IOG_IS_WINDOW (user_data));

	window = IOG_WINDOW (user_data);

	dialog = gtk_dialog_new_with_buttons (_("Toolbar Editor"),
					      GTK_WINDOW (window),
				              GTK_DIALOG_DESTROY_WITH_PARENT,
					      _("_Reset to Default"),
					      IOG_TB_EDITOR_DLG_RESET_RESPONSE,
 					      GTK_STOCK_CLOSE,
					      GTK_RESPONSE_CLOSE,
					      GTK_STOCK_HELP,
					      GTK_RESPONSE_HELP,
					      NULL);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog),
					 GTK_RESPONSE_CLOSE);

	gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);

	gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), 2);

	gtk_window_set_default_size (GTK_WINDOW (dialog), 500, 400);

	editor = egg_toolbar_editor_new (window->priv->ui_mgr,
					 iog_application_get_toolbars_model (IOG_APP));

	gtk_container_set_border_width (GTK_CONTAINER (editor), 5);

#if GTK_CHECK_VERSION (3, 0, 0)
	// Use as much vertical space as available
	gtk_widget_set_vexpand (GTK_WIDGET (editor), TRUE);
#endif

	gtk_box_set_spacing (GTK_BOX (EGG_TOOLBAR_EDITOR (editor)), 5);

	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), editor);

	egg_editable_toolbar_set_edit_mode
		(EGG_EDITABLE_TOOLBAR (window->priv->toolbar), TRUE);

	g_object_set_data (G_OBJECT (dialog), "EggToolbarEditor", editor);

	g_signal_connect (dialog,
                          "response",
			  G_CALLBACK (iog_window_cmd_edit_toolbar_cb),
			  window);

	gtk_widget_show_all (dialog);
}

static void
iog_window_cmd_help (GtkAction *action, gpointer user_data)
{
	IogWindow *window;

	g_return_if_fail (IOG_IS_WINDOW (user_data));

	window = IOG_WINDOW (user_data);

	iog_util_show_help (NULL, GTK_WINDOW (window));
}

static void
iog_window_cmd_about (GtkAction *action, gpointer user_data)
{
	IogWindow *window;

	g_return_if_fail (IOG_IS_WINDOW (user_data));

	static const char *authors[] = {
		"Perberos <perberos@gmail.com>",
		"Steve Zesch <stevezesch2@gmail.com>",
		"Stefano Karapetsas <stefano@karapetsas.com>",
		"",
		"Claudio Saavedra <csaavedra@igalia.com> (maintainer)",
		"Felix Riemann <friemann@gnome.org> (maintainer)",
		"",
		"Lucas Rocha <lucasr@gnome.org>",
		"Tim Gerla <tim+gde2bugs@gerla.net>",
		"Philip Van Hoof <pvanhoof@gnome.org>",
                "Paolo Borelli <pborelli@katamail.com>",
		"Jens Finke <jens@triq.net>",
		"Martin Baulig <martin@home-of-linux.org>",
		"Arik Devens <arik@gnome.org>",
		"Michael Meeks <mmeeks@gnu.org>",
		"Federico Mena-Quintero <federico@gnu.org>",
		"Lutz M\xc3\xbcller <urc8@rz.uni-karlsruhe.de>",
		NULL
	};

	static const char *documenters[] = {
		"Eliot Landrum <eliot@landrum.cx>",
		"Federico Mena-Quintero <federico@gnu.org>",
		"Sun GNOME Documentation Team <gdocteam@sun.com>",
		NULL
	};

	const char *translators;

	translators = _("translator-credits");

	const char *license[] = {
		N_("This program is free software; you can redistribute it and/or modify "
		   "it under the terms of the GNU General Public License as published by "
		   "the Free Software Foundation; either version 2 of the License, or "
		   "(at your option) any later version.\n"),
		N_("This program is distributed in the hope that it will be useful, "
		   "but WITHOUT ANY WARRANTY; without even the implied warranty of "
		   "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
		   "GNU General Public License for more details.\n"),
		N_("You should have received a copy of the GNU General Public License "
		   "along with this program; if not, write to the Free Software "
		   "Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.")
	};

	char *license_trans;

	license_trans = g_strconcat (_(license[0]), "\n", _(license[1]), "\n",
				     _(license[2]), "\n", NULL);

	window = IOG_WINDOW (user_data);

	gtk_show_about_dialog (GTK_WINDOW (window),
			       "program-name", _("Eye of GDE2"),
			       "version", VERSION,
			       "copyright", "Copyright \xc2\xa9 2000-2010 Free Software Foundation, Inc.\n"
			       "Copyright \xc2\xa9 2011 Perberos\n"
			       "Copyright \xc2\xa9 2012-2016 GDE2 developers",
			       "comments",_("The GDE2 image viewer."),
			       "authors", authors,
			       "documenters", documenters,
			       "translator-credits", translators,
			       "website", "http://www.gde2-desktop.org/",
			       "logo-icon-name", "iog",
			       "wrap-license", TRUE,
			       "license", license_trans,
			       NULL);

	g_free (license_trans);
}

static void
iog_window_cmd_show_hide_bar (GtkAction *action, gpointer user_data)
{
	IogWindow *window;
	IogWindowPrivate *priv;
	gboolean visible;

	g_return_if_fail (IOG_IS_WINDOW (user_data));

	window = IOG_WINDOW (user_data);
	priv = window->priv;

	if (priv->mode != IOG_WINDOW_MODE_NORMAL &&
            priv->mode != IOG_WINDOW_MODE_FULLSCREEN) return;

	visible = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

	if (g_ascii_strcasecmp (gtk_action_get_name (action), "ViewToolbar") == 0) {
		g_object_set (G_OBJECT (priv->toolbar), "visible", visible, NULL);

		if (priv->mode == IOG_WINDOW_MODE_NORMAL)
			g_settings_set_boolean (priv->ui_settings, IOG_CONF_UI_TOOLBAR, visible);

	} else if (g_ascii_strcasecmp (gtk_action_get_name (action), "ViewStatusbar") == 0) {
		g_object_set (G_OBJECT (priv->statusbar), "visible", visible, NULL);

		if (priv->mode == IOG_WINDOW_MODE_NORMAL)
			g_settings_set_boolean (priv->ui_settings, IOG_CONF_UI_STATUSBAR, visible);

	} else if (g_ascii_strcasecmp (gtk_action_get_name (action), "ViewImageCollection") == 0) {
		if (visible) {
			/* Make sure the focus widget is realized to
			 * avoid warnings on keypress events */
			if (!gtk_widget_get_realized (window->priv->thumbview))
				gtk_widget_realize (window->priv->thumbview);

			gtk_widget_show (priv->nav);
			gtk_widget_grab_focus (priv->thumbview);
		} else {
			/* Make sure the focus widget is realized to
			 * avoid warnings on keypress events.
			 * Don't do it during init phase or the view
			 * will get a bogus allocation. */
			if (!gtk_widget_get_realized (priv->view)
			    && priv->status == IOG_WINDOW_STATUS_NORMAL)
				gtk_widget_realize (priv->view);

			gtk_widget_hide (priv->nav);

			if (gtk_widget_get_realized (priv->view))
				gtk_widget_grab_focus (priv->view);
		}
		g_settings_set_boolean (priv->ui_settings, IOG_CONF_UI_IMAGE_COLLECTION, visible);

	} else if (g_ascii_strcasecmp (gtk_action_get_name (action), "ViewSidebar") == 0) {
		if (visible) {
			gtk_widget_show (priv->sidebar);
		} else {
			gtk_widget_hide (priv->sidebar);
		}
		g_settings_set_boolean (priv->ui_settings, IOG_CONF_UI_SIDEBAR, visible);
	}
}

static void
wallpaper_info_bar_response (GtkInfoBar *bar, gint response, IogWindow *window)
{
	if (response == GTK_RESPONSE_YES) {
		GdkScreen *screen;

		screen = gtk_widget_get_screen (GTK_WIDGET (window));
		gde2_gdk_spawn_command_line_on_screen (screen,
						  "gde2-appearance-properties"
						  " --show-page=background",
						  NULL);
	}

	/* Close message area on every response */
	iog_window_set_message_area (window, NULL);
}

static void
iog_window_set_wallpaper (IogWindow *window, const gchar *filename, const gchar *visible_filename)
{
	GtkWidget *info_bar;
	GtkWidget *image;
	GtkWidget *label;
	GtkWidget *hbox;
	gchar *markup;
	gchar *text;
	gchar *basename;
	GSettings *wallpaper_settings;

	wallpaper_settings = g_settings_new (IOG_CONF_BACKGROUND_SCHEMA);
	g_settings_set_string (wallpaper_settings,
				 IOG_CONF_BACKGROUND_FILE,
				 filename);
	g_object_unref (wallpaper_settings);

	/* I18N: When setting mnemonics for these strings, watch out to not
	   clash with mnemonics from iog's menubar */
	info_bar = gtk_info_bar_new_with_buttons (_("_Open Background Preferences"),
						  GTK_RESPONSE_YES,
						  C_("MessageArea","Hi_de"),
						  GTK_RESPONSE_NO, NULL);
	gtk_info_bar_set_message_type (GTK_INFO_BAR (info_bar),
				       GTK_MESSAGE_QUESTION);

	image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_QUESTION,
					  GTK_ICON_SIZE_DIALOG);
	label = gtk_label_new (NULL);

	if (!visible_filename)
		basename = g_path_get_basename (filename);

	/* The newline character is currently necessary due to a problem
	 * with the automatic line break. */
	text = g_strdup_printf (_("The image \"%s\" has been set as Desktop Background."
				  "\nWould you like to modify its appearance?"),
				visible_filename ? visible_filename : basename);
	markup = g_markup_printf_escaped ("<b>%s</b>", text);
	gtk_label_set_markup (GTK_LABEL (label), markup);
	g_free (markup);
	g_free (text);
	if (!visible_filename)
		g_free (basename);

	hbox = gtk_hbox_new (FALSE, 8);
	gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
#if GTK_CHECK_VERSION (3, 0, 0)
	gtk_widget_set_halign (image, GTK_ALIGN_START);
	gtk_widget_set_valign (image, GTK_ALIGN_END);
#else
	gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);
#endif
	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
#if GTK_CHECK_VERSION (3, 16, 0)
	gtk_label_set_xalign (GTK_LABEL (label), 0.0);
#else
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
	gtk_box_pack_start (GTK_BOX (gtk_info_bar_get_content_area (GTK_INFO_BAR (info_bar))), hbox, TRUE, TRUE, 0);
	gtk_widget_show_all (hbox);
	gtk_widget_show (info_bar);


	iog_window_set_message_area (window, info_bar);
	gtk_info_bar_set_default_response (GTK_INFO_BAR (info_bar),
					   GTK_RESPONSE_YES);
	g_signal_connect (info_bar, "response",
			  G_CALLBACK (wallpaper_info_bar_response), window);
}

static void
iog_job_save_cb (IogJobSave *job, gpointer user_data)
{
	IogWindow *window = IOG_WINDOW (user_data);
	GtkAction *action_save;

	g_signal_handlers_disconnect_by_func (job,
					      iog_job_save_cb,
					      window);

	g_signal_handlers_disconnect_by_func (job,
					      iog_job_save_progress_cb,
					      window);

	g_object_unref (window->priv->save_job);
	window->priv->save_job = NULL;

	update_status_bar (window);
	action_save = gtk_action_group_get_action (window->priv->actions_image,
						   "ImageSave");
	gtk_action_set_sensitive (action_save, FALSE);
}

static void
iog_job_copy_cb (IogJobCopy *job, gpointer user_data)
{
	IogWindow *window = IOG_WINDOW (user_data);
	gchar *filepath, *basename, *filename, *extension;
	GtkAction *action;
	GFile *source_file, *dest_file;

	/* Create source GFile */
	basename = g_file_get_basename (job->images->data);
	filepath = g_build_filename (job->dest, basename, NULL);
	source_file = g_file_new_for_path (filepath);
	g_free (filepath);

	/* Create destination GFile */
	extension = iog_util_filename_get_extension (basename);
	filename = g_strdup_printf  ("%s.%s", IOG_WALLPAPER_FILENAME, extension);
	filepath = g_build_filename (job->dest, filename, NULL);
	dest_file = g_file_new_for_path (filepath);
	g_free (filename);
	g_free (extension);

	/* Move the file */
	g_file_move (source_file, dest_file, G_FILE_COPY_OVERWRITE,
		     NULL, NULL, NULL, NULL);

	/* Set the wallpaper */
	iog_window_set_wallpaper (window, filepath, basename);
	g_free (basename);
	g_free (filepath);

	gtk_statusbar_pop (GTK_STATUSBAR (window->priv->statusbar),
			   window->priv->copy_file_cid);
	action = gtk_action_group_get_action (window->priv->actions_image,
					      "ImageSetAsWallpaper");
	gtk_action_set_sensitive (action, TRUE);

	window->priv->copy_job = NULL;

	g_object_unref (source_file);
	g_object_unref (dest_file);
	g_object_unref (G_OBJECT (job->images->data));
	g_list_free (job->images);
	g_object_unref (job);
}

static gboolean
iog_window_save_images (IogWindow *window, GList *images)
{
	IogWindowPrivate *priv;

	priv = window->priv;

	if (window->priv->save_job != NULL)
		return FALSE;

	priv->save_job = iog_job_save_new (images);

	g_signal_connect (priv->save_job,
			  "finished",
			  G_CALLBACK (iog_job_save_cb),
			  window);

	g_signal_connect (priv->save_job,
			  "progress",
			  G_CALLBACK (iog_job_save_progress_cb),
			  window);

	return TRUE;
}

static void
iog_window_cmd_save (GtkAction *action, gpointer user_data)
{
	IogWindowPrivate *priv;
	IogWindow *window;
	GList *images;

	window = IOG_WINDOW (user_data);
	priv = window->priv;

	if (window->priv->save_job != NULL)
		return;

	images = iog_thumb_view_get_selected_images (IOG_THUMB_VIEW (priv->thumbview));

	if (iog_window_save_images (window, images)) {
		iog_job_queue_add_job (priv->save_job);
	}
}

static GFile*
iog_window_retrieve_save_as_file (IogWindow *window, IogImage *image)
{
	GtkWidget *dialog;
	GFile *save_file = NULL;
	GFile *last_dest_folder;
	gint response;

	g_assert (image != NULL);

	dialog = iog_file_chooser_new (GTK_FILE_CHOOSER_ACTION_SAVE);

	last_dest_folder = window->priv->last_save_as_folder;

	if (last_dest_folder && g_file_query_exists (last_dest_folder, NULL)) {
		gtk_file_chooser_set_current_folder_file (GTK_FILE_CHOOSER (dialog), last_dest_folder, NULL);
		gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog),
						 iog_image_get_caption (image));
	} else {
		GFile *image_file;

		image_file = iog_image_get_file (image);
		/* Setting the file will also navigate to its parent folder */
		gtk_file_chooser_set_file (GTK_FILE_CHOOSER (dialog),
					   image_file, NULL);
		g_object_unref (image_file);
	}

	response = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_hide (dialog);

	if (response == GTK_RESPONSE_OK) {
		save_file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));
		if (window->priv->last_save_as_folder)
			g_object_unref (window->priv->last_save_as_folder);
		window->priv->last_save_as_folder = g_file_get_parent (save_file);
	}
	gtk_widget_destroy (dialog);

	return save_file;
}

static void
iog_window_cmd_save_as (GtkAction *action, gpointer user_data)
{
        IogWindowPrivate *priv;
        IogWindow *window;
	GList *images;
	guint n_images;

        window = IOG_WINDOW (user_data);
	priv = window->priv;

	if (window->priv->save_job != NULL)
		return;

	images = iog_thumb_view_get_selected_images (IOG_THUMB_VIEW (priv->thumbview));
	n_images = g_list_length (images);

	if (n_images == 1) {
		GFile *file;

		file = iog_window_retrieve_save_as_file (window, images->data);

		if (!file) {
			g_list_free (images);
			return;
		}

		priv->save_job = iog_job_save_as_new (images, NULL, file);

		g_object_unref (file);
	} else if (n_images > 1) {
		GFile *base_file;
		GtkWidget *dialog;
		gchar *basedir;
		IogURIConverter *converter;

		basedir = g_get_current_dir ();
		base_file = g_file_new_for_path (basedir);
		g_free (basedir);

		dialog = iog_save_as_dialog_new (GTK_WINDOW (window),
						 images,
						 base_file);

		gtk_widget_show_all (dialog);

		if (gtk_dialog_run (GTK_DIALOG (dialog)) != GTK_RESPONSE_OK) {
			g_object_unref (base_file);
			g_list_free (images);
			gtk_widget_destroy (dialog);

			return;
		}

		converter = iog_save_as_dialog_get_converter (dialog);

		g_assert (converter != NULL);

		priv->save_job = iog_job_save_as_new (images, converter, NULL);

		gtk_widget_destroy (dialog);

		g_object_unref (converter);
		g_object_unref (base_file);
	} else {
		/* n_images = 0 -- No Image selected */
		return;
	}

	g_signal_connect (priv->save_job,
			  "finished",
			  G_CALLBACK (iog_job_save_cb),
			  window);

	g_signal_connect (priv->save_job,
			  "progress",
			  G_CALLBACK (iog_job_save_progress_cb),
			  window);

	iog_job_queue_add_job (priv->save_job);
}

static void
iog_window_cmd_print (GtkAction *action, gpointer user_data)
{
	IogWindow *window = IOG_WINDOW (user_data);

	iog_window_print (window);
}

static void
iog_window_cmd_properties (GtkAction *action, gpointer user_data)
{
	IogWindow *window = IOG_WINDOW (user_data);
	IogWindowPrivate *priv;
	GtkAction *next_image_action, *previous_image_action;

	priv = window->priv;

	next_image_action =
		gtk_action_group_get_action (priv->actions_collection,
					     "GoNext");

	previous_image_action =
		gtk_action_group_get_action (priv->actions_collection,
					     "GoPrevious");

	if (window->priv->properties_dlg == NULL) {

		window->priv->properties_dlg =
			iog_properties_dialog_new (GTK_WINDOW (window),
						   IOG_THUMB_VIEW (priv->thumbview),
						   next_image_action,
						   previous_image_action);

		iog_properties_dialog_update (IOG_PROPERTIES_DIALOG (priv->properties_dlg),
					      priv->image);
		g_settings_bind (priv->ui_settings,
				 IOG_CONF_UI_PROPSDIALOG_NETBOOK_MODE,
				 priv->properties_dlg, "netbook-mode",
				 G_SETTINGS_BIND_GET);
	}

	iog_dialog_show (IOG_DIALOG (window->priv->properties_dlg));
}

static void
iog_window_cmd_undo (GtkAction *action, gpointer user_data)
{
	g_return_if_fail (IOG_IS_WINDOW (user_data));

	apply_transformation (IOG_WINDOW (user_data), NULL);
}

static void
iog_window_cmd_flip_horizontal (GtkAction *action, gpointer user_data)
{
	g_return_if_fail (IOG_IS_WINDOW (user_data));

	apply_transformation (IOG_WINDOW (user_data),
			      iog_transform_flip_new (IOG_TRANSFORM_FLIP_HORIZONTAL));
}

static void
iog_window_cmd_flip_vertical (GtkAction *action, gpointer user_data)
{
	g_return_if_fail (IOG_IS_WINDOW (user_data));

	apply_transformation (IOG_WINDOW (user_data),
			      iog_transform_flip_new (IOG_TRANSFORM_FLIP_VERTICAL));
}

static void
iog_window_cmd_rotate_90 (GtkAction *action, gpointer user_data)
{
	g_return_if_fail (IOG_IS_WINDOW (user_data));

	apply_transformation (IOG_WINDOW (user_data),
			      iog_transform_rotate_new (90));
}

static void
iog_window_cmd_rotate_270 (GtkAction *action, gpointer user_data)
{
	g_return_if_fail (IOG_IS_WINDOW (user_data));

	apply_transformation (IOG_WINDOW (user_data),
			      iog_transform_rotate_new (270));
}

static void
iog_window_cmd_wallpaper (GtkAction *action, gpointer user_data)
{
	IogWindow *window;
	IogWindowPrivate *priv;
	IogImage *image;
	GFile *file;
	char *filename = NULL;

	g_return_if_fail (IOG_IS_WINDOW (user_data));

	window = IOG_WINDOW (user_data);
	priv = window->priv;

	/* If currently copying an image to set it as wallpaper, return. */
	if (priv->copy_job != NULL)
		return;

	image = iog_thumb_view_get_first_selected_image (IOG_THUMB_VIEW (priv->thumbview));

	g_return_if_fail (IOG_IS_IMAGE (image));

	file = iog_image_get_file (image);

	filename = g_file_get_path (file);

	/* Currently only local files can be set as wallpaper */
	if (filename == NULL || !iog_util_file_is_persistent (file))
	{
		GList *files = NULL;
		GtkAction *action;

		action = gtk_action_group_get_action (window->priv->actions_image,
						      "ImageSetAsWallpaper");
		gtk_action_set_sensitive (action, FALSE);

		priv->copy_file_cid = gtk_statusbar_get_context_id (GTK_STATUSBAR (priv->statusbar),
								    "copy_file_cid");
		gtk_statusbar_push (GTK_STATUSBAR (priv->statusbar),
				    priv->copy_file_cid,
				    _("Saving image locally…"));

		files = g_list_append (files, iog_image_get_file (image));
		priv->copy_job = iog_job_copy_new (files, g_get_user_data_dir ());
		g_signal_connect (priv->copy_job,
				  "finished",
				  G_CALLBACK (iog_job_copy_cb),
				  window);
		g_signal_connect (priv->copy_job,
				  "progress",
				  G_CALLBACK (iog_job_progress_cb),
				  window);
		iog_job_queue_add_job (priv->copy_job);

		g_object_unref (file);
		g_free (filename);
		return;
	}

	g_object_unref (file);

	iog_window_set_wallpaper (window, filename, NULL);

	g_free (filename);
}

static gboolean
iog_window_all_images_trasheable (GList *images)
{
	GFile *file;
	GFileInfo *file_info;
	GList *iter;
	IogImage *image;
	gboolean can_trash = TRUE;

	for (iter = images; iter != NULL; iter = g_list_next (iter)) {
		image = (IogImage *) iter->data;
		file = iog_image_get_file (image);
		file_info = g_file_query_info (file,
					       G_FILE_ATTRIBUTE_ACCESS_CAN_TRASH,
					       0, NULL, NULL);
		can_trash = g_file_info_get_attribute_boolean (file_info,
							       G_FILE_ATTRIBUTE_ACCESS_CAN_TRASH);

		g_object_unref (file_info);
		g_object_unref (file);

		if (can_trash == FALSE)
			break;
	}

	return can_trash;
}

static int
show_move_to_trash_confirm_dialog (IogWindow *window, GList *images, gboolean can_trash)
{
	GtkWidget *dlg;
	char *prompt;
	int response;
	int n_images;
	IogImage *image;
	static gboolean dontaskagain = FALSE;
	gboolean neverask = FALSE;
	GtkWidget* dontask_cbutton = NULL;

	/* Check if the user never wants to be bugged. */
	neverask = g_settings_get_boolean (window->priv->ui_settings,
					 IOG_CONF_UI_DISABLE_TRASH_CONFIRMATION);

	/* Assume agreement, if the user doesn't want to be
	 * asked and the trash is available */
	if (can_trash && (dontaskagain || neverask))
		return GTK_RESPONSE_OK;
	
	n_images = g_list_length (images);

	if (n_images == 1) {
		image = IOG_IMAGE (images->data);
		if (can_trash) {
			prompt = g_strdup_printf (_("Are you sure you want to move\n\"%s\" to the trash?"),
						  iog_image_get_caption (image));
		} else {
			prompt = g_strdup_printf (_("A trash for \"%s\" couldn't be found. Do you want to remove "
						    "this image permanently?"), iog_image_get_caption (image));
		}
	} else {
		if (can_trash) {
			prompt = g_strdup_printf (ngettext("Are you sure you want to move\n"
							   "the selected image to the trash?",
							   "Are you sure you want to move\n"
							   "the %d selected images to the trash?", n_images), n_images);
		} else {
			prompt = g_strdup (_("Some of the selected images can't be moved to the trash "
					     "and will be removed permanently. Are you sure you want "
					     "to proceed?"));
		}
	}

	dlg = gtk_message_dialog_new_with_markup (GTK_WINDOW (window),
						  GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						  GTK_MESSAGE_WARNING,
						  GTK_BUTTONS_NONE,
						  "<span weight=\"bold\" size=\"larger\">%s</span>",
						  prompt);
	g_free (prompt);

	gtk_dialog_add_button (GTK_DIALOG (dlg), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);

	if (can_trash) {
		gtk_dialog_add_button (GTK_DIALOG (dlg), _("Move to _Trash"), GTK_RESPONSE_OK);

		dontask_cbutton = gtk_check_button_new_with_mnemonic (_("_Do not ask again during this session"));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dontask_cbutton), FALSE);

		gtk_box_pack_end (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dlg))), dontask_cbutton, TRUE, TRUE, 0);
	} else {
		if (n_images == 1) {
			gtk_dialog_add_button (GTK_DIALOG (dlg), GTK_STOCK_DELETE, GTK_RESPONSE_OK);
		} else {
			gtk_dialog_add_button (GTK_DIALOG (dlg), GTK_STOCK_YES, GTK_RESPONSE_OK);
		}
	}

	gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);
	gtk_window_set_title (GTK_WINDOW (dlg), "");
	gtk_widget_show_all (dlg);

	response = gtk_dialog_run (GTK_DIALOG (dlg));

	/* Only update the property if the user has accepted */
	if (can_trash && response == GTK_RESPONSE_OK)
		dontaskagain = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dontask_cbutton));

	/* The checkbutton is destroyed together with the dialog */
	gtk_widget_destroy (dlg);

	return response;
}

static gboolean
move_to_trash_real (IogImage *image, GError **error)
{
	GFile *file;
	GFileInfo *file_info;
	gboolean can_trash, result;

	g_return_val_if_fail (IOG_IS_IMAGE (image), FALSE);

	file = iog_image_get_file (image);
	file_info = g_file_query_info (file,
				       G_FILE_ATTRIBUTE_ACCESS_CAN_TRASH,
				       0, NULL, NULL);
	if (file_info == NULL) {
		g_set_error (error,
			     IOG_WINDOW_ERROR,
			     IOG_WINDOW_ERROR_TRASH_NOT_FOUND,
			     _("Couldn't access trash."));
		return FALSE;
	}

	can_trash = g_file_info_get_attribute_boolean (file_info,
						       G_FILE_ATTRIBUTE_ACCESS_CAN_TRASH);
	g_object_unref (file_info);
	if (can_trash)
	{
		result = g_file_trash (file, NULL, NULL);
		if (result == FALSE) {
			g_set_error (error,
				     IOG_WINDOW_ERROR,
				     IOG_WINDOW_ERROR_TRASH_NOT_FOUND,
				     _("Couldn't access trash."));
		}
	} else {
		result = g_file_delete (file, NULL, NULL);
		if (result == FALSE) {
			g_set_error (error,
				     IOG_WINDOW_ERROR,
				     IOG_WINDOW_ERROR_IO,
				     _("Couldn't delete file"));
		}
	}

        g_object_unref (file);

	return result;
}

static void
iog_window_cmd_copy_image (GtkAction *action, gpointer user_data)
{
	GtkClipboard *clipboard;
	IogWindow *window;
	IogWindowPrivate *priv;
	IogImage *image;
	IogClipboardHandler *cbhandler;

	g_return_if_fail (IOG_IS_WINDOW (user_data));

	window = IOG_WINDOW (user_data);
	priv = window->priv;

	image = iog_thumb_view_get_first_selected_image (IOG_THUMB_VIEW (priv->thumbview));

	g_return_if_fail (IOG_IS_IMAGE (image));

	clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);

	cbhandler = iog_clipboard_handler_new (image);
	// cbhandler will self-destruct when it's not needed anymore
	iog_clipboard_handler_copy_to_clipboard (cbhandler, clipboard);

}

static void
iog_window_cmd_move_to_trash (GtkAction *action, gpointer user_data)
{
	GList *images;
	GList *it;
	IogWindowPrivate *priv;
	IogListStore *list;
	int pos;
	IogImage *img;
	IogWindow *window;
	int response;
	int n_images;
	gboolean success;
	gboolean can_trash;

	g_return_if_fail (IOG_IS_WINDOW (user_data));

	window = IOG_WINDOW (user_data);
	priv = window->priv;
	list = priv->store;

	n_images = iog_thumb_view_get_n_selected (IOG_THUMB_VIEW (priv->thumbview));

	if (n_images < 1) return;

	/* save position of selected image after the deletion */
	images = iog_thumb_view_get_selected_images (IOG_THUMB_VIEW (priv->thumbview));

	g_assert (images != NULL);

	/* HACK: iog_list_store_get_n_selected return list in reverse order */
	images = g_list_reverse (images);

	can_trash = iog_window_all_images_trasheable (images);

	if (g_ascii_strcasecmp (gtk_action_get_name (action), "Delete") == 0 ||
	    can_trash == FALSE) {
		response = show_move_to_trash_confirm_dialog (window, images, can_trash);

		if (response != GTK_RESPONSE_OK) return;
	}

	pos = iog_list_store_get_pos_by_image (list, IOG_IMAGE (images->data));

	/* FIXME: make a nice progress dialog */
	/* Do the work actually. First try to delete the image from the disk. If this
	 * is successful, remove it from the screen. Otherwise show error dialog.
	 */
	for (it = images; it != NULL; it = it->next) {
		GError *error = NULL;
		IogImage *image;

		image = IOG_IMAGE (it->data);

		success = move_to_trash_real (image, &error);

		if (success) {
			iog_list_store_remove_image (list, image);
		} else {
			char *header;
			GtkWidget *dlg;

			header = g_strdup_printf (_("Error on deleting image %s"),
						  iog_image_get_caption (image));

			dlg = gtk_message_dialog_new (GTK_WINDOW (window),
						      GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						      GTK_MESSAGE_ERROR,
						      GTK_BUTTONS_OK,
						      "%s", header);

			gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dlg),
								  "%s", error->message);

			gtk_dialog_run (GTK_DIALOG (dlg));

			gtk_widget_destroy (dlg);

			g_free (header);
		}
	}

	/* free list */
	g_list_foreach (images, (GFunc) g_object_unref, NULL);
	g_list_free (images);

	/* select image at previously saved position */
	pos = MIN (pos, iog_list_store_length (list) - 1);

	if (pos >= 0) {
		img = iog_list_store_get_image_by_pos (list, pos);

		iog_thumb_view_set_current_image (IOG_THUMB_VIEW (priv->thumbview),
						  img,
						  TRUE);

		if (img != NULL) {
			g_object_unref (img);
		}
	}
}

static void
iog_window_cmd_fullscreen (GtkAction *action, gpointer user_data)
{
	IogWindow *window;
	gboolean fullscreen;

	g_return_if_fail (IOG_IS_WINDOW (user_data));

	iog_debug (DEBUG_WINDOW);

	window = IOG_WINDOW (user_data);

	fullscreen = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

	if (fullscreen) {
		iog_window_run_fullscreen (window, FALSE);
	} else {
		iog_window_stop_fullscreen (window, FALSE);
	}
}

static void
iog_window_cmd_slideshow (GtkAction *action, gpointer user_data)
{
	IogWindow *window;
	gboolean slideshow;

	g_return_if_fail (IOG_IS_WINDOW (user_data));

	iog_debug (DEBUG_WINDOW);

	window = IOG_WINDOW (user_data);

	slideshow = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

	if (slideshow) {
		iog_window_run_fullscreen (window, TRUE);
	} else {
		iog_window_stop_fullscreen (window, TRUE);
	}
}

static void
iog_window_cmd_pause_slideshow (GtkAction *action, gpointer user_data)
{
	IogWindow *window;
	gboolean slideshow;

	g_return_if_fail (IOG_IS_WINDOW (user_data));

	iog_debug (DEBUG_WINDOW);

	window = IOG_WINDOW (user_data);

	slideshow = window->priv->mode == IOG_WINDOW_MODE_SLIDESHOW;

	if (!slideshow && window->priv->mode != IOG_WINDOW_MODE_FULLSCREEN)
		return;

	iog_window_run_fullscreen (window, !slideshow);
}

static void
iog_window_cmd_zoom_in (GtkAction *action, gpointer user_data)
{
	IogWindowPrivate *priv;

	g_return_if_fail (IOG_IS_WINDOW (user_data));

	iog_debug (DEBUG_WINDOW);

	priv = IOG_WINDOW (user_data)->priv;

	if (priv->view) {
		iog_scroll_view_zoom_in (IOG_SCROLL_VIEW (priv->view), FALSE);
	}
}

static void
iog_window_cmd_zoom_out (GtkAction *action, gpointer user_data)
{
	IogWindowPrivate *priv;

	g_return_if_fail (IOG_IS_WINDOW (user_data));

	iog_debug (DEBUG_WINDOW);

	priv = IOG_WINDOW (user_data)->priv;

	if (priv->view) {
		iog_scroll_view_zoom_out (IOG_SCROLL_VIEW (priv->view), FALSE);
	}
}

static void
iog_window_cmd_zoom_normal (GtkAction *action, gpointer user_data)
{
	IogWindowPrivate *priv;

	g_return_if_fail (IOG_IS_WINDOW (user_data));

	iog_debug (DEBUG_WINDOW);

	priv = IOG_WINDOW (user_data)->priv;

	if (priv->view) {
		iog_scroll_view_set_zoom (IOG_SCROLL_VIEW (priv->view), 1.0);
	}
}

static void
iog_window_cmd_zoom_fit (GtkAction *action, gpointer user_data)
{
	IogWindowPrivate *priv;

	g_return_if_fail (IOG_IS_WINDOW (user_data));

	iog_debug (DEBUG_WINDOW);

	priv = IOG_WINDOW (user_data)->priv;

	if (priv->view) {
		iog_scroll_view_zoom_fit (IOG_SCROLL_VIEW (priv->view));
	}
}

static void
iog_window_cmd_go_prev (GtkAction *action, gpointer user_data)
{
	IogWindowPrivate *priv;

	g_return_if_fail (IOG_IS_WINDOW (user_data));

	iog_debug (DEBUG_WINDOW);

	priv = IOG_WINDOW (user_data)->priv;

	iog_thumb_view_select_single (IOG_THUMB_VIEW (priv->thumbview),
				      IOG_THUMB_VIEW_SELECT_LEFT);
}

static void
iog_window_cmd_go_next (GtkAction *action, gpointer user_data)
{
	IogWindowPrivate *priv;

	g_return_if_fail (IOG_IS_WINDOW (user_data));

	iog_debug (DEBUG_WINDOW);

	priv = IOG_WINDOW (user_data)->priv;

	iog_thumb_view_select_single (IOG_THUMB_VIEW (priv->thumbview),
				      IOG_THUMB_VIEW_SELECT_RIGHT);
}

static void
iog_window_cmd_go_first (GtkAction *action, gpointer user_data)
{
	IogWindowPrivate *priv;

	g_return_if_fail (IOG_IS_WINDOW (user_data));

	iog_debug (DEBUG_WINDOW);

	priv = IOG_WINDOW (user_data)->priv;

	iog_thumb_view_select_single (IOG_THUMB_VIEW (priv->thumbview),
				      IOG_THUMB_VIEW_SELECT_FIRST);
}

static void
iog_window_cmd_go_last (GtkAction *action, gpointer user_data)
{
	IogWindowPrivate *priv;

	g_return_if_fail (IOG_IS_WINDOW (user_data));

	iog_debug (DEBUG_WINDOW);

	priv = IOG_WINDOW (user_data)->priv;

	iog_thumb_view_select_single (IOG_THUMB_VIEW (priv->thumbview),
				      IOG_THUMB_VIEW_SELECT_LAST);
}

static void
iog_window_cmd_go_random (GtkAction *action, gpointer user_data)
{
	IogWindowPrivate *priv;

	g_return_if_fail (IOG_IS_WINDOW (user_data));

	iog_debug (DEBUG_WINDOW);

	priv = IOG_WINDOW (user_data)->priv;

	iog_thumb_view_select_single (IOG_THUMB_VIEW (priv->thumbview),
				      IOG_THUMB_VIEW_SELECT_RANDOM);
}

static const GtkActionEntry action_entries_window[] = {
	{ "Image", NULL, N_("_Image") },
	{ "Edit",  NULL, N_("_Edit") },
	{ "View",  NULL, N_("_View") },
	{ "Go",    NULL, N_("_Go") },
	{ "Tools", NULL, N_("_Tools") },
	{ "Help",  NULL, N_("_Help") },

	{ "ImageOpen", GTK_STOCK_OPEN,  N_("_Open…"), "<control>O",
	  N_("Open a file"),
	  G_CALLBACK (iog_window_cmd_file_open) },
	{ "ImageClose", GTK_STOCK_CLOSE, N_("_Close"), "<control>W",
	  N_("Close window"),
	  G_CALLBACK (iog_window_cmd_close_window) },
	{ "EditToolbar", NULL, N_("T_oolbar"), NULL,
	  N_("Edit the application toolbar"),
	  G_CALLBACK (iog_window_cmd_edit_toolbar) },
	{ "EditPreferences", GTK_STOCK_PREFERENCES, N_("Prefere_nces"), NULL,
	  N_("Preferences for Eye of GDE2"),
	  G_CALLBACK (iog_window_cmd_preferences) },
	{ "HelpManual", GTK_STOCK_HELP, N_("_Contents"), "F1",
	  N_("Help on this application"),
	  G_CALLBACK (iog_window_cmd_help) },
	{ "HelpAbout", GTK_STOCK_ABOUT, N_("_About"), NULL,
	  N_("About this application"),
	  G_CALLBACK (iog_window_cmd_about) }
};

static const GtkToggleActionEntry toggle_entries_window[] = {
	{ "ViewToolbar", NULL, N_("_Toolbar"), NULL,
	  N_("Changes the visibility of the toolbar in the current window"),
	  G_CALLBACK (iog_window_cmd_show_hide_bar), TRUE },
	{ "ViewStatusbar", NULL, N_("_Statusbar"), NULL,
	  N_("Changes the visibility of the statusbar in the current window"),
	  G_CALLBACK (iog_window_cmd_show_hide_bar), TRUE },
	{ "ViewImageCollection", "iog-image-collection", N_("_Image Collection"), "F9",
	  N_("Changes the visibility of the image collection pane in the current window"),
	  G_CALLBACK (iog_window_cmd_show_hide_bar), TRUE },
	{ "ViewSidebar", NULL, N_("Side _Pane"), "<control>F9",
	  N_("Changes the visibility of the side pane in the current window"),
	  G_CALLBACK (iog_window_cmd_show_hide_bar), TRUE },
};

static const GtkActionEntry action_entries_image[] = {
	{ "ImageSave", GTK_STOCK_SAVE, N_("_Save"), "<control>s",
	  N_("Save changes in currently selected images"),
	  G_CALLBACK (iog_window_cmd_save) },
	{ "ImageOpenWith", NULL, N_("Open _with"), NULL,
	  N_("Open the selected image with a different application"),
	  NULL},
	{ "ImageSaveAs", GTK_STOCK_SAVE_AS, N_("Save _As…"), "<control><shift>s",
	  N_("Save the selected images with a different name"),
	  G_CALLBACK (iog_window_cmd_save_as) },
	{ "ImagePrint", GTK_STOCK_PRINT, N_("_Print…"), "<control>p",
	  N_("Print the selected image"),
	  G_CALLBACK (iog_window_cmd_print) },
	{ "ImageProperties", GTK_STOCK_PROPERTIES, N_("Prope_rties"), "<alt>Return",
	  N_("Show the properties and metadata of the selected image"),
	  G_CALLBACK (iog_window_cmd_properties) },
	{ "EditUndo", GTK_STOCK_UNDO, N_("_Undo"), "<control>z",
	  N_("Undo the last change in the image"),
	  G_CALLBACK (iog_window_cmd_undo) },
	{ "EditFlipHorizontal", "object-flip-horizontal", N_("Flip _Horizontal"), NULL,
	  N_("Mirror the image horizontally"),
	  G_CALLBACK (iog_window_cmd_flip_horizontal) },
	{ "EditFlipVertical", "object-flip-vertical", N_("Flip _Vertical"), NULL,
	  N_("Mirror the image vertically"),
	  G_CALLBACK (iog_window_cmd_flip_vertical) },
	{ "EditRotate90",  "object-rotate-right",  N_("_Rotate Clockwise"), "<control>r",
	  N_("Rotate the image 90 degrees to the right"),
	  G_CALLBACK (iog_window_cmd_rotate_90) },
	{ "EditRotate270", "object-rotate-left", N_("Rotate Counterc_lockwise"), "<ctrl><shift>r",
	  N_("Rotate the image 90 degrees to the left"),
	  G_CALLBACK (iog_window_cmd_rotate_270) },
	{ "ImageSetAsWallpaper", NULL, N_("Set as _Desktop Background"),
	  "<control>F8", N_("Set the selected image as the desktop background"),
	  G_CALLBACK (iog_window_cmd_wallpaper) },
	{ "EditMoveToTrash", "user-trash", N_("Move to _Trash"), NULL,
	  N_("Move the selected image to the trash folder"),
	  G_CALLBACK (iog_window_cmd_move_to_trash) },
	{ "EditCopyImage", "edit-copy", N_("_Copy"), "<control>C",
	  N_("Copy the selected image to the clipboard"),
	  G_CALLBACK (iog_window_cmd_copy_image) },
	{ "ViewZoomIn", GTK_STOCK_ZOOM_IN, N_("_Zoom In"), "<control>plus",
	  N_("Enlarge the image"),
	  G_CALLBACK (iog_window_cmd_zoom_in) },
	{ "ViewZoomOut", GTK_STOCK_ZOOM_OUT, N_("Zoom _Out"), "<control>minus",
	  N_("Shrink the image"),
	  G_CALLBACK (iog_window_cmd_zoom_out) },
	{ "ViewZoomNormal", GTK_STOCK_ZOOM_100, N_("_Normal Size"), "<control>0",
	  N_("Show the image at its normal size"),
	  G_CALLBACK (iog_window_cmd_zoom_normal) },
	{ "ViewZoomFit", GTK_STOCK_ZOOM_FIT, N_("_Best Fit"), "F",
	  N_("Fit the image to the window"),
	  G_CALLBACK (iog_window_cmd_zoom_fit) },
	{ "ControlEqual", GTK_STOCK_ZOOM_IN, N_("_Zoom In"), "<control>equal",
	  N_("Enlarge the image"),
	  G_CALLBACK (iog_window_cmd_zoom_in) },
	{ "ControlKpAdd", GTK_STOCK_ZOOM_IN, N_("_Zoom In"), "<control>KP_Add",
	  N_("Shrink the image"),
	  G_CALLBACK (iog_window_cmd_zoom_in) },
	{ "ControlKpSub", GTK_STOCK_ZOOM_OUT, N_("Zoom _Out"), "<control>KP_Subtract",
	  N_("Shrink the image"),
	  G_CALLBACK (iog_window_cmd_zoom_out) },
	{ "Delete", NULL, N_("Move to _Trash"), "Delete",
	  NULL,
	  G_CALLBACK (iog_window_cmd_move_to_trash) },
};

static const GtkToggleActionEntry toggle_entries_image[] = {
	{ "ViewFullscreen", GTK_STOCK_FULLSCREEN, N_("_Fullscreen"), "F11",
	  N_("Show the current image in fullscreen mode"),
	  G_CALLBACK (iog_window_cmd_fullscreen), FALSE },
	{ "PauseSlideshow", "media-playback-pause", N_("Pause Slideshow"),
	  NULL, N_("Pause or resume the slideshow"),
	  G_CALLBACK (iog_window_cmd_pause_slideshow), FALSE },
};

static const GtkActionEntry action_entries_collection[] = {
	{ "GoPrevious", GTK_STOCK_GO_BACK, N_("_Previous Image"), "<Alt>Left",
	  N_("Go to the previous image of the collection"),
	  G_CALLBACK (iog_window_cmd_go_prev) },
	{ "GoNext", GTK_STOCK_GO_FORWARD, N_("_Next Image"), "<Alt>Right",
	  N_("Go to the next image of the collection"),
	  G_CALLBACK (iog_window_cmd_go_next) },
	{ "GoFirst", GTK_STOCK_GOTO_FIRST, N_("_First Image"), "<Alt>Home",
	  N_("Go to the first image of the collection"),
	  G_CALLBACK (iog_window_cmd_go_first) },
	{ "GoLast", GTK_STOCK_GOTO_LAST, N_("_Last Image"), "<Alt>End",
	  N_("Go to the last image of the collection"),
	  G_CALLBACK (iog_window_cmd_go_last) },
	{ "GoRandom", NULL, N_("_Random Image"), "<control>M",
	  N_("Go to a random image of the collection"),
	  G_CALLBACK (iog_window_cmd_go_random) },
	{ "BackSpace", NULL, N_("_Previous Image"), "BackSpace",
	  NULL,
	  G_CALLBACK (iog_window_cmd_go_prev) },
	{ "Home", NULL, N_("_First Image"), "Home",
	  NULL,
	  G_CALLBACK (iog_window_cmd_go_first) },
	{ "End", NULL, N_("_Last Image"), "End",
	  NULL,
	  G_CALLBACK (iog_window_cmd_go_last) },
};

static const GtkToggleActionEntry toggle_entries_collection[] = {
	{ "ViewSlideshow", "slideshow-play", N_("S_lideshow"), "F5",
	  N_("Start a slideshow view of the images"),
	  G_CALLBACK (iog_window_cmd_slideshow), FALSE },
};

static void
menu_item_select_cb (GtkMenuItem *proxy, IogWindow *window)
{
	GtkAction *action;
	char *message;

	action = gtk_activatable_get_related_action (GTK_ACTIVATABLE (proxy));

	g_return_if_fail (action != NULL);

	g_object_get (G_OBJECT (action), "tooltip", &message, NULL);

	if (message) {
		gtk_statusbar_push (GTK_STATUSBAR (window->priv->statusbar),
				    window->priv->tip_message_cid, message);
		g_free (message);
	}
}

static void
menu_item_deselect_cb (GtkMenuItem *proxy, IogWindow *window)
{
	gtk_statusbar_pop (GTK_STATUSBAR (window->priv->statusbar),
			   window->priv->tip_message_cid);
}

static void
connect_proxy_cb (GtkUIManager *manager,
                  GtkAction *action,
                  GtkWidget *proxy,
                  IogWindow *window)
{
	if (GTK_IS_MENU_ITEM (proxy)) {
		g_signal_connect (proxy, "select",
				  G_CALLBACK (menu_item_select_cb), window);
		g_signal_connect (proxy, "deselect",
				  G_CALLBACK (menu_item_deselect_cb), window);
	}
}

static void
disconnect_proxy_cb (GtkUIManager *manager,
                     GtkAction *action,
                     GtkWidget *proxy,
                     IogWindow *window)
{
	if (GTK_IS_MENU_ITEM (proxy)) {
		g_signal_handlers_disconnect_by_func
			(proxy, G_CALLBACK (menu_item_select_cb), window);
		g_signal_handlers_disconnect_by_func
			(proxy, G_CALLBACK (menu_item_deselect_cb), window);
	}
}

static void
set_action_properties (GtkActionGroup *window_group,
		       GtkActionGroup *image_group,
		       GtkActionGroup *collection_group)
{
        GtkAction *action;

        action = gtk_action_group_get_action (collection_group, "GoPrevious");
        g_object_set (action, "short_label", _("Previous"), NULL);
        g_object_set (action, "is-important", TRUE, NULL);

        action = gtk_action_group_get_action (collection_group, "GoNext");
        g_object_set (action, "short_label", _("Next"), NULL);
        g_object_set (action, "is-important", TRUE, NULL);

        action = gtk_action_group_get_action (image_group, "EditRotate90");
        g_object_set (action, "short_label", _("Right"), NULL);

        action = gtk_action_group_get_action (image_group, "EditRotate270");
        g_object_set (action, "short_label", _("Left"), NULL);

        action = gtk_action_group_get_action (image_group, "ViewZoomIn");
        g_object_set (action, "short_label", _("In"), NULL);

        action = gtk_action_group_get_action (image_group, "ViewZoomOut");
        g_object_set (action, "short_label", _("Out"), NULL);

        action = gtk_action_group_get_action (image_group, "ViewZoomNormal");
        g_object_set (action, "short_label", _("Normal"), NULL);

        action = gtk_action_group_get_action (image_group, "ViewZoomFit");
        g_object_set (action, "short_label", _("Fit"), NULL);

        action = gtk_action_group_get_action (window_group, "ViewImageCollection");
        g_object_set (action, "short_label", _("Collection"), NULL);

        action = gtk_action_group_get_action (image_group, "EditMoveToTrash");
        g_object_set (action, "short_label", C_("action (to trash)", "Trash"), NULL);
}

static gint
sort_recents_mru (GtkRecentInfo *a, GtkRecentInfo *b)
{
	gboolean has_iog_a, has_iog_b;

	/* We need to check this first as gtk_recent_info_get_application_info
	 * will treat it as a non-fatal error when the GtkRecentInfo doesn't
	 * have the application registered. */
	has_iog_a = gtk_recent_info_has_application (a,
						     IOG_RECENT_FILES_APP_NAME);
	has_iog_b = gtk_recent_info_has_application (b,
						     IOG_RECENT_FILES_APP_NAME);
	if (has_iog_a && has_iog_b) {
		time_t time_a, time_b;

		/* These should not fail as we already checked that
		 * the application is registered with the info objects */
		gtk_recent_info_get_application_info (a,
						      IOG_RECENT_FILES_APP_NAME,
						      NULL,
						      NULL,
						      &time_a);
		gtk_recent_info_get_application_info (b,
						      IOG_RECENT_FILES_APP_NAME,
						      NULL,
						      NULL,
						      &time_b);

		return (time_b - time_a);
	} else if (has_iog_a) {
		return -1;
	} else if (has_iog_b) {
		return 1;
	}

	return 0;
}

static void
iog_window_update_recent_files_menu (IogWindow *window)
{
	IogWindowPrivate *priv;
	GList *actions = NULL, *li = NULL, *items = NULL;
	guint count_recent = 0;

	priv = window->priv;

	if (priv->recent_menu_id != 0)
		gtk_ui_manager_remove_ui (priv->ui_mgr, priv->recent_menu_id);

	actions = gtk_action_group_list_actions (priv->actions_recent);

	for (li = actions; li != NULL; li = li->next) {
		g_signal_handlers_disconnect_by_func (GTK_ACTION (li->data),
						      G_CALLBACK(iog_window_open_recent_cb),
						      window);

		gtk_action_group_remove_action (priv->actions_recent,
						GTK_ACTION (li->data));
	}

	g_list_free (actions);

	priv->recent_menu_id = gtk_ui_manager_new_merge_id (priv->ui_mgr);
	items = gtk_recent_manager_get_items (gtk_recent_manager_get_default());
	items = g_list_sort (items, (GCompareFunc) sort_recents_mru);

	for (li = items; li != NULL && count_recent < IOG_RECENT_FILES_LIMIT; li = li->next) {
		gchar *action_name;
		gchar *label;
		gchar *tip;
		gchar **display_name;
		gchar *label_filename;
		GtkAction *action;
		GtkRecentInfo *info = li->data;

		/* Sorting moves non-IOG files to the end of the list.
		 * So no file of interest will follow if this test fails */
		if (!gtk_recent_info_has_application (info, IOG_RECENT_FILES_APP_NAME))
			break;

		count_recent++;

		action_name = g_strdup_printf ("recent-info-%d", count_recent);
		display_name = g_strsplit (gtk_recent_info_get_display_name (info), "_", -1);
		label_filename = g_strjoinv ("__", display_name);
		label = g_strdup_printf ("%s_%d. %s",
				(is_rtl ? "\xE2\x80\x8F" : ""), count_recent, label_filename);
		g_free (label_filename);
		g_strfreev (display_name);

		tip = gtk_recent_info_get_uri_display (info);

		/* This is a workaround for a bug (#351945) regarding
		 * gtk_recent_info_get_uri_display() and remote URIs.
		 * gde2_vfs_format_uri_for_display is sufficient here
		 * since the password gets stripped when adding the
		 * file to the recently used list. */
		if (tip == NULL)
			tip = g_uri_unescape_string (gtk_recent_info_get_uri (info), NULL);

		action = gtk_action_new (action_name, label, tip, NULL);
		gtk_action_set_always_show_image (action, TRUE);

		g_object_set_data_full (G_OBJECT (action), "gtk-recent-info",
					gtk_recent_info_ref (info),
					(GDestroyNotify) gtk_recent_info_unref);

		g_object_set (G_OBJECT (action), "icon-name", "image-x-generic", NULL);

		g_signal_connect (action, "activate",
				  G_CALLBACK (iog_window_open_recent_cb),
				  window);

		gtk_action_group_add_action (priv->actions_recent, action);

		g_object_unref (action);

		gtk_ui_manager_add_ui (priv->ui_mgr, priv->recent_menu_id,
				       "/MainMenu/Image/RecentDocuments",
				       action_name, action_name,
				       GTK_UI_MANAGER_AUTO, FALSE);

		g_free (action_name);
		g_free (label);
		g_free (tip);
	}

	g_list_foreach (items, (GFunc) gtk_recent_info_unref, NULL);
	g_list_free (items);
}

static void
iog_window_recent_manager_changed_cb (GtkRecentManager *manager, IogWindow *window)
{
	iog_window_update_recent_files_menu (window);
}

static void
iog_window_drag_data_received (GtkWidget *widget,
                               GdkDragContext *context,
                               gint x, gint y,
                               GtkSelectionData *selection_data,
                               guint info, guint time)
{
	GSList *file_list;
	IogWindow *window;
	GdkAtom target;
	GtkWidget *src;

	target = gtk_selection_data_get_target (selection_data);

	if (!gtk_targets_include_uri (&target, 1))
		return;

	/* if the request is from another process this will return NULL */
	src = gtk_drag_get_source_widget (context);

	/* if the drag request originates from the current iog instance, ignore
	   the request if the source window is the same as the dest window */
	if (src &&
	    gtk_widget_get_toplevel (src) == gtk_widget_get_toplevel (widget))
	{
		gdk_drag_status (context, 0, time);
		return;
	}

	if (gdk_drag_context_get_suggested_action (context) == GDK_ACTION_COPY) {
		window = IOG_WINDOW (widget);

		file_list = iog_util_parse_uri_string_list_to_file_list ((const gchar *) gtk_selection_data_get_data (selection_data));

		iog_window_open_file_list (window, file_list);
	}
}

static void
iog_window_set_drag_dest (IogWindow *window)
{
	gtk_drag_dest_set (GTK_WIDGET (window),
                           GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP,
                           NULL, 0,
                           GDK_ACTION_COPY | GDK_ACTION_ASK);
	gtk_drag_dest_add_uri_targets (GTK_WIDGET (window));
}

static void
iog_window_sidebar_visibility_changed (GtkWidget *widget, IogWindow *window)
{
	GtkAction *action;
	gboolean visible;

	visible = gtk_widget_get_visible (window->priv->sidebar);

	g_settings_set_boolean (window->priv->ui_settings,
			       IOG_CONF_UI_SIDEBAR,
			       visible);

	action = gtk_action_group_get_action (window->priv->actions_window,
					      "ViewSidebar");

	if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)) != visible)
		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), visible);

	/* Focus the image */
	if (!visible && window->priv->image != NULL)
		gtk_widget_grab_focus (window->priv->view);
}

static void
iog_window_sidebar_page_added (IogSidebar  *sidebar,
			       GtkWidget   *main_widget,
			       IogWindow   *window)
{
	if (iog_sidebar_get_n_pages (sidebar) == 1) {
		GtkAction *action;
		gboolean show;

		action = gtk_action_group_get_action (window->priv->actions_window,
						      "ViewSidebar");

		gtk_action_set_sensitive (action, TRUE);

		show = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

		if (show)
			gtk_widget_show (GTK_WIDGET (sidebar));
	}
}
static void
iog_window_sidebar_page_removed (IogSidebar  *sidebar,
			         GtkWidget   *main_widget,
			         IogWindow   *window)
{
	if (iog_sidebar_is_empty (sidebar)) {
		GtkAction *action;

		gtk_widget_hide (GTK_WIDGET (sidebar));

		action = gtk_action_group_get_action (window->priv->actions_window,
						      "ViewSidebar");

		gtk_action_set_sensitive (action, FALSE);
	}
}

static void
iog_window_finish_saving (IogWindow *window)
{
	IogWindowPrivate *priv = window->priv;

	gtk_widget_set_sensitive (GTK_WIDGET (window), FALSE);

	do {
		gtk_main_iteration ();
	} while (priv->save_job != NULL);
}

static GAppInfo *
get_appinfo_for_editor (IogWindow *window)
{
	/* We want this function to always return the same thing, not
	 * just for performance reasons, but because if someone edits
	 * GConf while iog is running, the application could get into an
	 * inconsistent state.  If the editor exists once, it gets added
	 * to the "available" list of the EggToolbarsModel (for which
	 * there is no API to remove it).  If later the editor no longer
	 * existed when constructing a new window, we'd be unable to
	 * construct a GtkAction for the editor for that window, causing
	 * assertion failures when viewing the "Edit Toolbars" dialog
	 * (item is available, but can't find the GtkAction for it).
	 *
	 * By ensuring we keep the GAppInfo around, we avoid the
	 * possibility of that situation occurring.
	 */
	static GDesktopAppInfo *app_info = NULL;
	static gboolean initialised;

	if (!initialised) {
		gchar *editor;

		editor = g_settings_get_string (window->priv->ui_settings,
		                                IOG_CONF_UI_EXTERNAL_EDITOR);

		if (editor != NULL) {
			app_info = g_desktop_app_info_new (editor);
		}

		initialised = TRUE;
		g_free (editor);
	}

	return (GAppInfo *) app_info;
}

static void
iog_window_open_editor (GtkAction *action,
                        IogWindow *window)
{
	GdkAppLaunchContext *context;
	GAppInfo *app_info;
	GList files;

	app_info = get_appinfo_for_editor (window);

	if (app_info == NULL)
		return;

#if GTK_CHECK_VERSION (3, 0, 0)
	context = gdk_display_get_app_launch_context (
	  gtk_widget_get_display (GTK_WIDGET (window)));
#else
	context = gdk_app_launch_context_new ();
#endif
	gdk_app_launch_context_set_screen (context,
	  gtk_widget_get_screen (GTK_WIDGET (window)));
	gdk_app_launch_context_set_icon (context,
	  g_app_info_get_icon (app_info));
	gdk_app_launch_context_set_timestamp (context,
	  gtk_get_current_event_time ());

	{
		GList f = { iog_image_get_file (window->priv->image) };
		files = f;
	}

	g_app_info_launch (app_info, &files,
                           G_APP_LAUNCH_CONTEXT (context), NULL);

	g_object_unref (files.data);
	g_object_unref (context);
}

static void
iog_window_add_open_editor_action (IogWindow *window)
{
	EggToolbarsModel *model;
	GAppInfo *app_info;
	GtkAction *action;
        gchar *tooltip;

	app_info = get_appinfo_for_editor (window);

	if (app_info == NULL)
		return;

	model = iog_application_get_toolbars_model (IOG_APP);
	egg_toolbars_model_set_name_flags (model, "OpenEditor",
	                                   EGG_TB_MODEL_NAME_KNOWN);

	tooltip = g_strdup_printf (_("Edit the current image using %s"),
	                           g_app_info_get_name (app_info));
	action = gtk_action_new ("OpenEditor", _("Edit Image"), tooltip, NULL);
	gtk_action_set_gicon (action, g_app_info_get_icon (app_info));
	gtk_action_set_is_important (action, TRUE);

	g_signal_connect (action, "activate",
	                  G_CALLBACK (iog_window_open_editor), window);

	gtk_action_group_add_action (window->priv->actions_image, action);

	g_object_unref (action);
	g_free (tooltip);
}

static void
iog_window_construct_ui (IogWindow *window)
{
	IogWindowPrivate *priv;

	GError *error = NULL;

	GtkWidget *menubar;
	GtkWidget *thumb_popup;
	GtkWidget *view_popup;
	GtkWidget *hpaned;
	GtkWidget *menuitem;

	g_return_if_fail (IOG_IS_WINDOW (window));

	priv = window->priv;

	priv->box = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (window), priv->box);
	gtk_widget_show (priv->box);

	priv->ui_mgr = gtk_ui_manager_new ();

	priv->actions_window = gtk_action_group_new ("MenuActionsWindow");

	gtk_action_group_set_translation_domain (priv->actions_window,
						 GETTEXT_PACKAGE);

	gtk_action_group_add_actions (priv->actions_window,
				      action_entries_window,
				      G_N_ELEMENTS (action_entries_window),
				      window);

	gtk_action_group_add_toggle_actions (priv->actions_window,
					     toggle_entries_window,
					     G_N_ELEMENTS (toggle_entries_window),
					     window);

	gtk_ui_manager_insert_action_group (priv->ui_mgr, priv->actions_window, 0);

	priv->actions_image = gtk_action_group_new ("MenuActionsImage");
	gtk_action_group_set_translation_domain (priv->actions_image,
						 GETTEXT_PACKAGE);

	gtk_action_group_add_actions (priv->actions_image,
				      action_entries_image,
				      G_N_ELEMENTS (action_entries_image),
				      window);

	iog_window_add_open_editor_action (window);

	gtk_action_group_add_toggle_actions (priv->actions_image,
					     toggle_entries_image,
					     G_N_ELEMENTS (toggle_entries_image),
					     window);

	gtk_ui_manager_insert_action_group (priv->ui_mgr, priv->actions_image, 0);

	priv->actions_collection = gtk_action_group_new ("MenuActionsCollection");
	gtk_action_group_set_translation_domain (priv->actions_collection,
						 GETTEXT_PACKAGE);

	gtk_action_group_add_actions (priv->actions_collection,
				      action_entries_collection,
				      G_N_ELEMENTS (action_entries_collection),
				      window);

	gtk_action_group_add_toggle_actions (priv->actions_collection,
					     toggle_entries_collection,
					     G_N_ELEMENTS (toggle_entries_collection),
					     window);

	set_action_properties (priv->actions_window,
			       priv->actions_image,
			       priv->actions_collection);

	gtk_ui_manager_insert_action_group (priv->ui_mgr, priv->actions_collection, 0);

	if (!gtk_ui_manager_add_ui_from_file (priv->ui_mgr,
					      IOG_DATA_DIR"/iog-ui.xml",
					      &error)) {
		g_warning ("building menus failed: %s", error->message);
		g_error_free (error);
	}

	g_signal_connect (priv->ui_mgr, "connect_proxy",
			  G_CALLBACK (connect_proxy_cb), window);
	g_signal_connect (priv->ui_mgr, "disconnect_proxy",
			  G_CALLBACK (disconnect_proxy_cb), window);

	menubar = gtk_ui_manager_get_widget (priv->ui_mgr, "/MainMenu");
	g_assert (GTK_IS_WIDGET (menubar));
	gtk_box_pack_start (GTK_BOX (priv->box), menubar, FALSE, FALSE, 0);
	gtk_widget_show (menubar);

	menuitem = gtk_ui_manager_get_widget (priv->ui_mgr,
			"/MainMenu/Edit/EditFlipHorizontal");
	gtk_image_menu_item_set_always_show_image (
			GTK_IMAGE_MENU_ITEM (menuitem), TRUE);

	menuitem = gtk_ui_manager_get_widget (priv->ui_mgr,
			"/MainMenu/Edit/EditFlipVertical");
	gtk_image_menu_item_set_always_show_image (
			GTK_IMAGE_MENU_ITEM (menuitem), TRUE);

	menuitem = gtk_ui_manager_get_widget (priv->ui_mgr,
			"/MainMenu/Edit/EditRotate90");
	gtk_image_menu_item_set_always_show_image (
			GTK_IMAGE_MENU_ITEM (menuitem), TRUE);

	menuitem = gtk_ui_manager_get_widget (priv->ui_mgr,
			"/MainMenu/Edit/EditRotate270");
	gtk_image_menu_item_set_always_show_image (
			GTK_IMAGE_MENU_ITEM (menuitem), TRUE);

	priv->toolbar = GTK_WIDGET
		(g_object_new (EGG_TYPE_EDITABLE_TOOLBAR,
			       "ui-manager", priv->ui_mgr,
			       "model", iog_application_get_toolbars_model (IOG_APP),
			       NULL));

#if GTK_CHECK_VERSION (3, 0, 2)
	gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET (priv->toolbar)),
				     GTK_STYLE_CLASS_PRIMARY_TOOLBAR);
#endif

	egg_editable_toolbar_show (EGG_EDITABLE_TOOLBAR (priv->toolbar),
				   "Toolbar");

	gtk_box_pack_start (GTK_BOX (priv->box),
			    priv->toolbar,
			    FALSE,
			    FALSE,
			    0);

	gtk_widget_show (priv->toolbar);

	gtk_window_add_accel_group (GTK_WINDOW (window),
				    gtk_ui_manager_get_accel_group (priv->ui_mgr));

	priv->actions_recent = gtk_action_group_new ("RecentFilesActions");
	gtk_action_group_set_translation_domain (priv->actions_recent,
						 GETTEXT_PACKAGE);

	g_signal_connect (gtk_recent_manager_get_default (), "changed",
			  G_CALLBACK (iog_window_recent_manager_changed_cb),
			  window);

	iog_window_update_recent_files_menu (window);

	gtk_ui_manager_insert_action_group (priv->ui_mgr, priv->actions_recent, 0);

	priv->cbox = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (priv->box), priv->cbox, TRUE, TRUE, 0);
	gtk_widget_show (priv->cbox);

	priv->statusbar = iog_statusbar_new ();
	gtk_box_pack_end (GTK_BOX (priv->box),
			  GTK_WIDGET (priv->statusbar),
			  FALSE, FALSE, 0);
	gtk_widget_show (priv->statusbar);

	priv->image_info_message_cid =
		gtk_statusbar_get_context_id (GTK_STATUSBAR (priv->statusbar),
					      "image_info_message");
	priv->tip_message_cid =
		gtk_statusbar_get_context_id (GTK_STATUSBAR (priv->statusbar),
					      "tip_message");

	priv->layout = gtk_vbox_new (FALSE, 2);

#if GTK_CHECK_VERSION (3, 2, 0)
	hpaned = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);
#else
	hpaned = gtk_hpaned_new ();
#endif

	priv->sidebar = iog_sidebar_new ();
	/* The sidebar shouldn't be shown automatically on show_all(),
	   but only when the user actually wants it. */
	gtk_widget_set_no_show_all (priv->sidebar, TRUE);

	gtk_widget_set_size_request (priv->sidebar, 210, -1);

	g_signal_connect_after (priv->sidebar,
				"show",
				G_CALLBACK (iog_window_sidebar_visibility_changed),
				window);

	g_signal_connect_after (priv->sidebar,
				"hide",
				G_CALLBACK (iog_window_sidebar_visibility_changed),
				window);

	g_signal_connect_after (priv->sidebar,
				"page-added",
				G_CALLBACK (iog_window_sidebar_page_added),
				window);

	g_signal_connect_after (priv->sidebar,
				"page-removed",
				G_CALLBACK (iog_window_sidebar_page_removed),
				window);

 	priv->view = iog_scroll_view_new ();
	gtk_widget_set_size_request (GTK_WIDGET (priv->view), 100, 100);
	g_signal_connect (G_OBJECT (priv->view),
			  "zoom_changed",
			  G_CALLBACK (view_zoom_changed_cb),
			  window);

	g_settings_bind (priv->view_settings, IOG_CONF_VIEW_SCROLL_WHEEL_ZOOM,
			 priv->view, "scrollwheel-zoom", G_SETTINGS_BIND_GET);
	g_settings_bind (priv->view_settings, IOG_CONF_VIEW_ZOOM_MULTIPLIER,
			 priv->view, "zoom-multiplier", G_SETTINGS_BIND_GET);

	view_popup = gtk_ui_manager_get_widget (priv->ui_mgr, "/ViewPopup");
	iog_scroll_view_set_popup (IOG_SCROLL_VIEW (priv->view),
				   GTK_MENU (view_popup));

	gtk_paned_pack1 (GTK_PANED (hpaned),
			 priv->sidebar,
			 FALSE,
			 FALSE);

	gtk_paned_pack2 (GTK_PANED (hpaned),
			 priv->view,
			 TRUE,
			 FALSE);

	gtk_widget_show_all (hpaned);

	gtk_box_pack_start (GTK_BOX (priv->layout), hpaned, TRUE, TRUE, 0);

	priv->thumbview = g_object_ref (iog_thumb_view_new ());

	/* giving shape to the view */
	gtk_icon_view_set_margin (GTK_ICON_VIEW (priv->thumbview), 4);
	gtk_icon_view_set_row_spacing (GTK_ICON_VIEW (priv->thumbview), 0);

	g_signal_connect (G_OBJECT (priv->thumbview), "selection_changed",
			  G_CALLBACK (handle_image_selection_changed_cb), window);

	priv->nav = iog_thumb_nav_new (priv->thumbview,
				       IOG_THUMB_NAV_MODE_ONE_ROW,
				       g_settings_get_boolean (priv->ui_settings,
							      IOG_CONF_UI_SCROLL_BUTTONS));

	// Bind the scroll buttons to their GSettings key
	g_settings_bind (priv->ui_settings, IOG_CONF_UI_SCROLL_BUTTONS,
			 priv->nav, "show-buttons", G_SETTINGS_BIND_GET);

	thumb_popup = gtk_ui_manager_get_widget (priv->ui_mgr, "/ThumbnailPopup");
	iog_thumb_view_set_thumbnail_popup (IOG_THUMB_VIEW (priv->thumbview),
					    GTK_MENU (thumb_popup));

	gtk_box_pack_start (GTK_BOX (priv->layout), priv->nav, FALSE, FALSE, 0);

	gtk_box_pack_end (GTK_BOX (priv->cbox), priv->layout, TRUE, TRUE, 0);

	iog_window_can_save_changed_cb (priv->lockdown_settings,
					IOG_CONF_LOCKDOWN_CAN_SAVE,
					window);
	g_settings_bind (priv->ui_settings, IOG_CONF_UI_IMAGE_COLLECTION_POSITION,
			 window, "collection-position", G_SETTINGS_BIND_GET);
	g_settings_bind (priv->ui_settings, IOG_CONF_UI_IMAGE_COLLECTION_RESIZABLE,
			 window, "collection-resizable", G_SETTINGS_BIND_GET);

	if ((priv->flags & IOG_STARTUP_FULLSCREEN) ||
	    (priv->flags & IOG_STARTUP_SLIDE_SHOW)) {
		iog_window_run_fullscreen (window, (priv->flags & IOG_STARTUP_SLIDE_SHOW));
	} else {
		priv->mode = IOG_WINDOW_MODE_NORMAL;
		update_ui_visibility (window);
	}

	iog_window_set_drag_dest (window);
}

static void
iog_window_init (IogWindow *window)
{
	GdkGiogetry hints;
	GdkScreen *screen;
	IogWindowPrivate *priv;

	iog_debug (DEBUG_WINDOW);

#if GTK_CHECK_VERSION(3, 0, 0)
    GtkStyleContext *context;

    context = gtk_widget_get_style_context (GTK_WIDGET (window));
    gtk_style_context_add_class (context, "iog-window");
#endif

	hints.min_width  = IOG_WINDOW_MIN_WIDTH;
	hints.min_height = IOG_WINDOW_MIN_HEIGHT;

	screen = gtk_widget_get_screen (GTK_WIDGET (window));

	priv = window->priv = IOG_WINDOW_GET_PRIVATE (window);

	priv->view_settings = g_settings_new (IOG_CONF_VIEW);
	priv->ui_settings = g_settings_new (IOG_CONF_UI);
	priv->fullscreen_settings = g_settings_new (IOG_CONF_FULLSCREEN);
	priv->lockdown_settings = g_settings_new (IOG_CONF_LOCKDOWN_SCHEMA);

	g_signal_connect (priv->lockdown_settings,
					  "changed::" IOG_CONF_LOCKDOWN_CAN_SAVE,
					  G_CALLBACK (iog_window_can_save_changed_cb),
					  window);

	window->priv->store = NULL;
	window->priv->image = NULL;

	window->priv->fullscreen_popup = NULL;
	window->priv->fullscreen_timeout_source = NULL;
	window->priv->slideshow_random = FALSE;
	window->priv->slideshow_loop = FALSE;
	window->priv->slideshow_switch_timeout = 0;
	window->priv->slideshow_switch_source = NULL;

	gtk_window_set_giogetry_hints (GTK_WINDOW (window),
				       GTK_WIDGET (window),
				       &hints,
				       GDK_HINT_MIN_SIZE);

	gtk_window_set_default_size (GTK_WINDOW (window),
				     IOG_WINDOW_DEFAULT_WIDTH,
				     IOG_WINDOW_DEFAULT_HEIGHT);

	gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);

	window->priv->mode = IOG_WINDOW_MODE_UNKNOWN;
	window->priv->status = IOG_WINDOW_STATUS_UNKNOWN;

#ifdef HAVE_LCMS
	window->priv->display_profile =
		iog_window_get_display_profile (screen);
#endif

	window->priv->recent_menu_id = 0;

	window->priv->collection_position = 0;
	window->priv->collection_resizable = FALSE;

	window->priv->save_disabled = FALSE;

	window->priv->page_setup = NULL;
}

static void
iog_window_dispose (GObject *object)
{
	IogWindow *window;
	IogWindowPrivate *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (IOG_IS_WINDOW (object));

	iog_debug (DEBUG_WINDOW);

	window = IOG_WINDOW (object);
	priv = window->priv;

	if (priv->page_setup != NULL) {
		g_object_unref (priv->page_setup);
		priv->page_setup = NULL;
	}

	if (priv->thumbview)
	{
		/* Disconnect so we don't get any unwanted callbacks
		 * when the thumb view is disposed. */
		g_signal_handlers_disconnect_by_func (priv->thumbview,
		                 G_CALLBACK (handle_image_selection_changed_cb),
		                 window);
		g_clear_object (&priv->thumbview);
	}

	iog_plugin_engine_garbage_collect ();

	if (priv->store != NULL) {
		g_signal_handlers_disconnect_by_func (priv->store,
					      iog_window_list_store_image_added,
					      window);
		g_signal_handlers_disconnect_by_func (priv->store,
					    iog_window_list_store_image_removed,
					    window);
		g_object_unref (priv->store);
		priv->store = NULL;
	}

	if (priv->image != NULL) {
	  	g_signal_handlers_disconnect_by_func (priv->image,
						      image_thumb_changed_cb,
						      window);
		g_signal_handlers_disconnect_by_func (priv->image,
						      image_file_changed_cb,
						      window);
		g_object_unref (priv->image);
		priv->image = NULL;
	}

	if (priv->actions_window != NULL) {
		g_object_unref (priv->actions_window);
		priv->actions_window = NULL;
	}

	if (priv->actions_image != NULL) {
		g_object_unref (priv->actions_image);
		priv->actions_image = NULL;
	}

	if (priv->actions_collection != NULL) {
		g_object_unref (priv->actions_collection);
		priv->actions_collection = NULL;
	}

	if (priv->actions_recent != NULL) {
		g_object_unref (priv->actions_recent);
		priv->actions_recent = NULL;
	}

        if (priv->actions_open_with != NULL) {
                g_object_unref (priv->actions_open_with);
                priv->actions_open_with = NULL;
        }

	fullscreen_clear_timeout (window);

	if (window->priv->fullscreen_popup != NULL) {
		gtk_widget_destroy (priv->fullscreen_popup);
		priv->fullscreen_popup = NULL;
	}

	slideshow_clear_timeout (window);

	g_signal_handlers_disconnect_by_func (gtk_recent_manager_get_default (),
					      G_CALLBACK (iog_window_recent_manager_changed_cb),
					      window);

	priv->recent_menu_id = 0;

	iog_window_clear_load_job (window);

	iog_window_clear_transform_job (window);

	if (priv->view_settings) {
		g_object_unref (priv->view_settings);
		priv->view_settings = NULL;
	}
	if (priv->ui_settings) {
		g_object_unref (priv->ui_settings);
		priv->ui_settings = NULL;
	}
	if (priv->fullscreen_settings) {
		g_object_unref (priv->fullscreen_settings);
		priv->fullscreen_settings = NULL;
	}
	if (priv->lockdown_settings) {
		g_object_unref (priv->lockdown_settings);
		priv->lockdown_settings = NULL;
	}

	if (priv->file_list != NULL) {
		g_slist_foreach (priv->file_list, (GFunc) g_object_unref, NULL);
		g_slist_free (priv->file_list);
		priv->file_list = NULL;
	}

#ifdef HAVE_LCMS
	if (priv->display_profile != NULL) {
		cmsCloseProfile (priv->display_profile);
		priv->display_profile = NULL;
	}
#endif

	if (priv->last_save_as_folder != NULL) {
		g_object_unref (priv->last_save_as_folder);
		priv->last_save_as_folder = NULL;
	}

	iog_plugin_engine_garbage_collect ();

	G_OBJECT_CLASS (iog_window_parent_class)->dispose (object);
}

static void
iog_window_finalize (GObject *object)
{
	GList *windows = iog_application_get_windows (IOG_APP);

	g_return_if_fail (IOG_IS_WINDOW (object));

	iog_debug (DEBUG_WINDOW);

	if (windows == NULL) {
		iog_application_shutdown (IOG_APP);
	} else {
		g_list_free (windows);
	}

	G_OBJECT_CLASS (iog_window_parent_class)->finalize (object);
}

static gint
iog_window_delete (GtkWidget *widget, GdkEventAny *event)
{
	IogWindow *window;
	IogWindowPrivate *priv;

	g_return_val_if_fail (IOG_IS_WINDOW (widget), FALSE);

	window = IOG_WINDOW (widget);
	priv = window->priv;

	if (priv->save_job != NULL) {
		iog_window_finish_saving (window);
	}

	if (iog_window_unsaved_images_confirm (window)) {
		return TRUE;
	}

	gtk_widget_destroy (widget);

	return TRUE;
}

static gint
iog_window_key_press (GtkWidget *widget, GdkEventKey *event)
{
	GtkContainer *tbcontainer = GTK_CONTAINER ((IOG_WINDOW (widget)->priv->toolbar));
	gint result = FALSE;
	gboolean handle_selection = FALSE;

	switch (event->keyval) {
	case GDK_KEY_space:
		if (event->state & GDK_CONTROL_MASK) {
			handle_selection = TRUE;
			break;
		}
	case GDK_KEY_Return:
		if (gtk_container_get_focus_child (tbcontainer) == NULL) {
			/* Image properties dialog case */
			if (event->state & GDK_MOD1_MASK) {
				result = FALSE;
				break;
			}

			if (event->state & GDK_SHIFT_MASK) {
				iog_window_cmd_go_prev (NULL, IOG_WINDOW (widget));
			} else {
				iog_window_cmd_go_next (NULL, IOG_WINDOW (widget));
			}
			result = TRUE;
		}
		break;
	case GDK_KEY_p:
	case GDK_KEY_P:
		if (IOG_WINDOW (widget)->priv->mode == IOG_WINDOW_MODE_FULLSCREEN || IOG_WINDOW (widget)->priv->mode == IOG_WINDOW_MODE_SLIDESHOW) {
			gboolean slideshow;

			slideshow = IOG_WINDOW (widget)->priv->mode == IOG_WINDOW_MODE_SLIDESHOW;
			iog_window_run_fullscreen (IOG_WINDOW (widget), !slideshow);
		}
		break;
	case GDK_KEY_Q:
	case GDK_KEY_q:
	case GDK_KEY_Escape:
		if (IOG_WINDOW (widget)->priv->mode == IOG_WINDOW_MODE_FULLSCREEN) {
			iog_window_stop_fullscreen (IOG_WINDOW (widget), FALSE);
		} else if (IOG_WINDOW (widget)->priv->mode == IOG_WINDOW_MODE_SLIDESHOW) {
			iog_window_stop_fullscreen (IOG_WINDOW (widget), TRUE);
		} else {
			iog_window_cmd_close_window (NULL, IOG_WINDOW (widget));
			return TRUE;
		}
		break;
	case GDK_KEY_Left:
		if (event->state & GDK_MOD1_MASK) {
			/* Alt+Left moves to previous image */
			if (is_rtl) { /* move to next in RTL mode */
				iog_window_cmd_go_next (NULL, IOG_WINDOW (widget));
			} else {
				iog_window_cmd_go_prev (NULL, IOG_WINDOW (widget));
			}
			result = TRUE;
			break;
		} /* else fall-trough is intended */
	case GDK_KEY_Up:
		if (iog_scroll_view_scrollbars_visible (IOG_SCROLL_VIEW (IOG_WINDOW (widget)->priv->view))) {
			/* break to let scrollview handle the key */
			break;
		}
		if (gtk_container_get_focus_child (tbcontainer) != NULL)
			break;
		if (!gtk_widget_get_visible (IOG_WINDOW (widget)->priv->nav)) {
			if (is_rtl && event->keyval == GDK_KEY_Left) {
				/* handle RTL fall-through,
				 * need to behave like GDK_Down then */
				iog_window_cmd_go_next (NULL,
							IOG_WINDOW (widget));
			} else {
				iog_window_cmd_go_prev (NULL,
							IOG_WINDOW (widget));
			}
			result = TRUE;
			break;
		}
	case GDK_KEY_Right:
		if (event->state & GDK_MOD1_MASK) {
			/* Alt+Right moves to next image */
			if (is_rtl) { /* move to previous in RTL mode */
				iog_window_cmd_go_prev (NULL, IOG_WINDOW (widget));
			} else {
				iog_window_cmd_go_next (NULL, IOG_WINDOW (widget));
			}
			result = TRUE;
			break;
		} /* else fall-trough is intended */
	case GDK_KEY_Down:
		if (iog_scroll_view_scrollbars_visible (IOG_SCROLL_VIEW (IOG_WINDOW (widget)->priv->view))) {
			/* break to let scrollview handle the key */
			break;
		}
		if (gtk_container_get_focus_child (tbcontainer) != NULL)
			break;
		if (!gtk_widget_get_visible (IOG_WINDOW (widget)->priv->nav)) {
			if (is_rtl && event->keyval == GDK_KEY_Right) {
				/* handle RTL fall-through,
				 * need to behave like GDK_Up then */
				iog_window_cmd_go_prev (NULL,
							IOG_WINDOW (widget));
			} else {
				iog_window_cmd_go_next (NULL,
							IOG_WINDOW (widget));
			}
			result = TRUE;
			break;
		}
	case GDK_KEY_Page_Up:
		if (!iog_scroll_view_scrollbars_visible (IOG_SCROLL_VIEW (IOG_WINDOW (widget)->priv->view))) {
			if (!gtk_widget_get_visible (IOG_WINDOW (widget)->priv->nav)) {
				/* If the iconview is not visible skip to the
				 * previous image manually as it won't handle
				 * the keypress then. */
				iog_window_cmd_go_prev (NULL,
							IOG_WINDOW (widget));
				result = TRUE;
			} else
				handle_selection = TRUE;
		}
		break;
	case GDK_KEY_Page_Down:
		if (!iog_scroll_view_scrollbars_visible (IOG_SCROLL_VIEW (IOG_WINDOW (widget)->priv->view))) {
			if (!gtk_widget_get_visible (IOG_WINDOW (widget)->priv->nav)) {
				/* If the iconview is not visible skip to the
				 * next image manually as it won't handle
				 * the keypress then. */
				iog_window_cmd_go_next (NULL,
							IOG_WINDOW (widget));
				result = TRUE;
			} else
				handle_selection = TRUE;
		}
		break;
	}

	/* Update slideshow timeout */
	if (result && (IOG_WINDOW (widget)->priv->mode == IOG_WINDOW_MODE_SLIDESHOW)) {
		slideshow_set_timeout (IOG_WINDOW (widget));
	}

	if (handle_selection == TRUE && result == FALSE) {
		gtk_widget_grab_focus (GTK_WIDGET (IOG_WINDOW (widget)->priv->thumbview));

		result = gtk_widget_event (GTK_WIDGET (IOG_WINDOW (widget)->priv->thumbview),
					   (GdkEvent *) event);
	}

	/* If the focus is not in the toolbar and we still haven't handled the
	   event, give the scrollview a chance to do it.  */
	if (!gtk_container_get_focus_child (tbcontainer) && result == FALSE &&
		gtk_widget_get_realized (GTK_WIDGET (IOG_WINDOW (widget)->priv->view))) {
			result = gtk_widget_event (GTK_WIDGET (IOG_WINDOW (widget)->priv->view),
						   (GdkEvent *) event);
	}

	if (result == FALSE && GTK_WIDGET_CLASS (iog_window_parent_class)->key_press_event) {
		result = (* GTK_WIDGET_CLASS (iog_window_parent_class)->key_press_event) (widget, event);
	}

	return result;
}

static gint
iog_window_button_press (GtkWidget *widget, GdkEventButton *event)
{
	IogWindow *window = IOG_WINDOW (widget);
	gint result = FALSE;

	if (event->type == GDK_BUTTON_PRESS) {
		switch (event->button) {
		case 6:
			iog_thumb_view_select_single (IOG_THUMB_VIEW (window->priv->thumbview),
						      IOG_THUMB_VIEW_SELECT_LEFT);
			result = TRUE;
		       	break;
		case 7:
			iog_thumb_view_select_single (IOG_THUMB_VIEW (window->priv->thumbview),
						      IOG_THUMB_VIEW_SELECT_RIGHT);
			result = TRUE;
		       	break;
		}
	}

	if (result == FALSE && GTK_WIDGET_CLASS (iog_window_parent_class)->button_press_event) {
		result = (* GTK_WIDGET_CLASS (iog_window_parent_class)->button_press_event) (widget, event);
	}

	return result;
}

static gboolean
iog_window_focus_out_event (GtkWidget *widget, GdkEventFocus *event)
{
	IogWindow *window = IOG_WINDOW (widget);
	IogWindowPrivate *priv = window->priv;
	gboolean fullscreen;

	iog_debug (DEBUG_WINDOW);

	fullscreen = priv->mode == IOG_WINDOW_MODE_FULLSCREEN ||
		     priv->mode == IOG_WINDOW_MODE_SLIDESHOW;

	if (fullscreen) {
		gtk_widget_hide (priv->fullscreen_popup);
	}

	return GTK_WIDGET_CLASS (iog_window_parent_class)->focus_out_event (widget, event);
}

static void
iog_window_set_property (GObject      *object,
			 guint         property_id,
			 const GValue *value,
			 GParamSpec   *pspec)
{
	IogWindow *window;
	IogWindowPrivate *priv;

	g_return_if_fail (IOG_IS_WINDOW (object));

	window = IOG_WINDOW (object);
	priv = window->priv;

	switch (property_id) {
	case PROP_COLLECTION_POS:
		iog_window_set_collection_mode (window, g_value_get_enum (value),
					     priv->collection_resizable);
		break;
	case PROP_COLLECTION_RESIZABLE:
		iog_window_set_collection_mode (window, priv->collection_position,
					     g_value_get_boolean (value));
		break;
	case PROP_STARTUP_FLAGS:
		priv->flags = g_value_get_flags (value);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
iog_window_get_property (GObject    *object,
			 guint       property_id,
			 GValue     *value,
			 GParamSpec *pspec)
{
	IogWindow *window;
	IogWindowPrivate *priv;

	g_return_if_fail (IOG_IS_WINDOW (object));

	window = IOG_WINDOW (object);
	priv = window->priv;

	switch (property_id) {
	case PROP_COLLECTION_POS:
		g_value_set_enum (value, priv->collection_position);
		break;
	case PROP_COLLECTION_RESIZABLE:
		g_value_set_boolean (value, priv->collection_resizable);
		break;
	case PROP_STARTUP_FLAGS:
		g_value_set_flags (value, priv->flags);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static GObject *
iog_window_constructor (GType type,
			guint n_construct_properties,
			GObjectConstructParam *construct_params)
{
	GObject *object;

	object = G_OBJECT_CLASS (iog_window_parent_class)->constructor
			(type, n_construct_properties, construct_params);

	iog_window_construct_ui (IOG_WINDOW (object));

	iog_plugin_engine_update_plugins_ui (IOG_WINDOW (object), TRUE);

	return object;
}

static void
iog_window_class_init (IogWindowClass *class)
{
	GObjectClass *g_object_class = (GObjectClass *) class;
	GtkWidgetClass *widget_class = (GtkWidgetClass *) class;

	g_object_class->constructor = iog_window_constructor;
	g_object_class->dispose = iog_window_dispose;
	g_object_class->finalize = iog_window_finalize;
	g_object_class->set_property = iog_window_set_property;
	g_object_class->get_property = iog_window_get_property;

	widget_class->delete_event = iog_window_delete;
	widget_class->key_press_event = iog_window_key_press;
	widget_class->button_press_event = iog_window_button_press;
	widget_class->drag_data_received = iog_window_drag_data_received;
	widget_class->focus_out_event = iog_window_focus_out_event;

/**
 * IogWindow:collection-position:
 *
 * Determines the position of the image collection in the window
 * relative to the image.
 */
	g_object_class_install_property (
		g_object_class, PROP_COLLECTION_POS,
		g_param_spec_enum ("collection-position", NULL, NULL,
				   IOG_TYPE_WINDOW_COLLECTION_POS,
				   IOG_WINDOW_COLLECTION_POS_BOTTOM,
				   G_PARAM_READWRITE | G_PARAM_STATIC_NAME));

/**
 * IogWindow:collection-resizable:
 *
 * If %TRUE the collection will be resizable by the user otherwise it will be
 * in single column/row mode.
 */
	g_object_class_install_property (
		g_object_class, PROP_COLLECTION_RESIZABLE,
		g_param_spec_boolean ("collection-resizable", NULL, NULL, FALSE,
				      G_PARAM_READWRITE | G_PARAM_STATIC_NAME));

/**
 * IogWindow:startup-flags:
 *
 * A bitwise OR of #IogStartupFlags elements, indicating how the window
 * should behave upon creation.
 */
	g_object_class_install_property (g_object_class,
					 PROP_STARTUP_FLAGS,
					 g_param_spec_flags ("startup-flags",
							     NULL,
							     NULL,
							     IOG_TYPE_STARTUP_FLAGS,
					 		     0,
					 		     G_PARAM_READWRITE |
							     G_PARAM_CONSTRUCT_ONLY));

/**
 * IogWindow::prepared:
 * @window: the object which received the signal.
 *
 * The #IogWindow::prepared signal is emitted when the @window is ready
 * to be shown.
 */
	signals [SIGNAL_PREPARED] =
		g_signal_new ("prepared",
			      IOG_TYPE_WINDOW,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (IogWindowClass, prepared),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	g_type_class_add_private (g_object_class, sizeof (IogWindowPrivate));
}

/**
 * iog_window_new:
 * @flags: the initialization parameters for the new window.
 *
 *
 * Creates a new and empty #IogWindow. Use @flags to indicate
 * if the window should be initialized fullscreen, in slideshow mode,
 * and/or without the thumbnails collection visible. See #IogStartupFlags.
 *
 * Returns: a newly created #IogWindow.
 **/
GtkWidget*
iog_window_new (IogStartupFlags flags)
{
	IogWindow *window;

	iog_debug (DEBUG_WINDOW);

	window = IOG_WINDOW (g_object_new (IOG_TYPE_WINDOW,
					   "type", GTK_WINDOW_TOPLEVEL,
					   "startup-flags", flags,
					   NULL));

	return GTK_WIDGET (window);
}

static void
iog_window_list_store_image_added (GtkTreeModel *tree_model,
                                   GtkTreePath  *path,
                                   GtkTreeIter  *iter,
                                   gpointer      user_data)
{
	IogWindow *window = IOG_WINDOW (user_data);

	update_image_pos (window);
	update_action_groups_state (window);
}

static void
iog_window_list_store_image_removed (GtkTreeModel *tree_model,
                                     GtkTreePath  *path,
                                     gpointer      user_data)
{
	IogWindow *window = IOG_WINDOW (user_data);

	update_image_pos (window);
	update_action_groups_state (window);
}

static void
iog_job_model_cb (IogJobModel *job, gpointer data)
{
	IogWindow *window;
	IogWindowPrivate *priv;
	gint n_images;

	iog_debug (DEBUG_WINDOW);

#ifdef HAVE_EXIF
	int i;
	IogImage *image;
#endif

	g_return_if_fail (IOG_IS_WINDOW (data));

	window = IOG_WINDOW (data);
	priv = window->priv;

	if (priv->store != NULL) {
		g_object_unref (priv->store);
		priv->store = NULL;
	}

	priv->store = g_object_ref (job->store);

	n_images = iog_list_store_length (IOG_LIST_STORE (priv->store));

#ifdef HAVE_EXIF
	if (g_settings_get_boolean (priv->view_settings, IOG_CONF_VIEW_AUTOROTATE)) {
		for (i = 0; i < n_images; i++) {
			image = iog_list_store_get_image_by_pos (priv->store, i);
			iog_image_autorotate (image);
			g_object_unref (image);
		}
	}
#endif

	iog_thumb_view_set_model (IOG_THUMB_VIEW (priv->thumbview), priv->store);

	g_signal_connect (G_OBJECT (priv->store),
			  "row-inserted",
			  G_CALLBACK (iog_window_list_store_image_added),
			  window);

	g_signal_connect (G_OBJECT (priv->store),
			  "row-deleted",
			  G_CALLBACK (iog_window_list_store_image_removed),
			  window);

	if (n_images == 0) {
		gint n_files;

		priv->status = IOG_WINDOW_STATUS_NORMAL;
		update_action_groups_state (window);

		n_files = g_slist_length (priv->file_list);

		if (n_files > 0) {
			GtkWidget *message_area;
			GFile *file = NULL;

			if (n_files == 1) {
				file = (GFile *) priv->file_list->data;
			}

			message_area = iog_no_images_error_message_area_new (file);

			iog_window_set_message_area (window, message_area);

			gtk_widget_show (message_area);
		}

		g_signal_emit (window, signals[SIGNAL_PREPARED], 0);
	}
}

/**
 * iog_window_open_file_list:
 * @window: An #IogWindow.
 * @file_list: (element-type GFile): A %NULL-terminated list of #GFile's.
 *
 * Opens a list of files, adding them to the collection in @window.
 * Files will be checked to be readable and later filtered according
 * with iog_list_store_add_files().
 **/
void
iog_window_open_file_list (IogWindow *window, GSList *file_list)
{
	IogJob *job;

	iog_debug (DEBUG_WINDOW);

	window->priv->status = IOG_WINDOW_STATUS_INIT;

	g_slist_foreach (file_list, (GFunc) g_object_ref, NULL);
	window->priv->file_list = file_list;

	job = iog_job_model_new (file_list);

	g_signal_connect (job,
			  "finished",
			  G_CALLBACK (iog_job_model_cb),
			  window);

	iog_job_queue_add_job (job);
	g_object_unref (job);
}

/**
 * iog_window_get_ui_manager:
 * @window: An #IogWindow.
 *
 * Gets the #GtkUIManager that describes the UI of @window.
 *
 * Returns: (transfer none): A #GtkUIManager.
 **/
GtkUIManager *
iog_window_get_ui_manager (IogWindow *window)
{
	g_return_val_if_fail (IOG_IS_WINDOW (window), NULL);

	return window->priv->ui_mgr;
}

/**
 * iog_window_get_mode:
 * @window: An #IogWindow.
 *
 * Gets the mode of @window. See #IogWindowMode for details.
 *
 * Returns: An #IogWindowMode.
 **/
IogWindowMode
iog_window_get_mode (IogWindow *window)
{
	g_return_val_if_fail (IOG_IS_WINDOW (window), IOG_WINDOW_MODE_UNKNOWN);

	return window->priv->mode;
}

/**
 * iog_window_set_mode:
 * @window: an #IogWindow.
 * @mode: an #IogWindowMode value.
 *
 * Changes the mode of @window to normal, fullscreen, or slideshow.
 * See #IogWindowMode for details.
 **/
void
iog_window_set_mode (IogWindow *window, IogWindowMode mode)
{
	g_return_if_fail (IOG_IS_WINDOW (window));

	if (window->priv->mode == mode)
		return;

	switch (mode) {
	case IOG_WINDOW_MODE_NORMAL:
		iog_window_stop_fullscreen (window,
					    window->priv->mode == IOG_WINDOW_MODE_SLIDESHOW);
		break;
	case IOG_WINDOW_MODE_FULLSCREEN:
		iog_window_run_fullscreen (window, FALSE);
		break;
	case IOG_WINDOW_MODE_SLIDESHOW:
		iog_window_run_fullscreen (window, TRUE);
		break;
	case IOG_WINDOW_MODE_UNKNOWN:
		break;
	}
}

/**
 * iog_window_get_store:
 * @window: An #IogWindow.
 *
 * Gets the #IogListStore that contains the images in the collection
 * of @window.
 *
 * Returns: (transfer none): an #IogListStore.
 **/
IogListStore *
iog_window_get_store (IogWindow *window)
{
	g_return_val_if_fail (IOG_IS_WINDOW (window), NULL);

	return IOG_LIST_STORE (window->priv->store);
}

/**
 * iog_window_get_view:
 * @window: An #IogWindow.
 *
 * Gets the #IogScrollView in the window.
 *
 * Returns: (transfer none): the #IogScrollView.
 **/
GtkWidget *
iog_window_get_view (IogWindow *window)
{
	g_return_val_if_fail (IOG_IS_WINDOW (window), NULL);

	return window->priv->view;
}

/**
 * iog_window_get_sidebar:
 * @window: An #IogWindow.
 *
 * Gets the sidebar widget of @window.
 *
 * Returns: (transfer none): the #IogSidebar.
 **/
GtkWidget *
iog_window_get_sidebar (IogWindow *window)
{
	g_return_val_if_fail (IOG_IS_WINDOW (window), NULL);

	return window->priv->sidebar;
}

/**
 * iog_window_get_thumb_view:
 * @window: an #IogWindow.
 *
 * Gets the thumbnails view in @window.
 *
 * Returns: (transfer none): an #IogThumbView.
 **/
GtkWidget *
iog_window_get_thumb_view (IogWindow *window)
{
	g_return_val_if_fail (IOG_IS_WINDOW (window), NULL);

	return window->priv->thumbview;
}

/**
 * iog_window_get_thumb_nav:
 * @window: an #IogWindow.
 *
 * Gets the thumbnails navigation pane in @window.
 *
 * Returns: (transfer none): an #IogThumbNav.
 **/
GtkWidget *
iog_window_get_thumb_nav (IogWindow *window)
{
	g_return_val_if_fail (IOG_IS_WINDOW (window), NULL);

	return window->priv->nav;
}

/**
 * iog_window_get_statusbar:
 * @window: an #IogWindow.
 *
 * Gets the statusbar in @window.
 *
 * Returns: (transfer none): a #IogStatusBar.
 **/
GtkWidget *
iog_window_get_statusbar (IogWindow *window)
{
	g_return_val_if_fail (IOG_IS_WINDOW (window), NULL);

	return window->priv->statusbar;
}

/**
 * iog_window_get_image:
 * @window: an #IogWindow.
 *
 * Gets the image currently displayed in @window or %NULL if
 * no image is being displayed.
 *
 * Returns: (transfer none): an #IogImage.
 **/
IogImage *
iog_window_get_image (IogWindow *window)
{
	g_return_val_if_fail (IOG_IS_WINDOW (window), NULL);

	return window->priv->image;
}

/**
 * iog_window_is_empty:
 * @window: an #IogWindow.
 *
 * Tells whether @window is currently empty or not.
 *
 * Returns: %TRUE if @window has no images, %FALSE otherwise.
 **/
gboolean
iog_window_is_empty (IogWindow *window)
{
	IogWindowPrivate *priv;
	gboolean empty = TRUE;

	iog_debug (DEBUG_WINDOW);

	g_return_val_if_fail (IOG_IS_WINDOW (window), FALSE);

	priv = window->priv;

	if (priv->store != NULL) {
		empty = (iog_list_store_length (IOG_LIST_STORE (priv->store)) == 0);
	}

	return empty;
}

void
iog_window_reload_image (IogWindow *window)
{
	GtkWidget *view;

	g_return_if_fail (IOG_IS_WINDOW (window));

	if (window->priv->image == NULL)
		return;

	g_object_unref (window->priv->image);
	window->priv->image = NULL;

	view = iog_window_get_view (window);
	iog_scroll_view_set_image (IOG_SCROLL_VIEW (view), NULL);

	iog_thumb_view_select_single (IOG_THUMB_VIEW (window->priv->thumbview),
				      IOG_THUMB_VIEW_SELECT_CURRENT);
}
