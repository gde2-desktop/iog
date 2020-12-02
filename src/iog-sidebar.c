/* Eye of Gde2 - Side bar
 *
 * Copyright (C) 2004 Red Hat, Inc.
 * Copyright (C) 2007 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 *
 * Based on evince code (shell/ev-sidebar.c) by:
 * 	- Jonathan Blandford <jrb@alum.mit.edu>
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

#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "iog-sidebar.h"

enum {
	PROP_0,
	PROP_CURRENT_PAGE
};

enum {
	PAGE_COLUMN_TITLE,
	PAGE_COLUMN_MENU_ITEM,
	PAGE_COLUMN_MAIN_WIDGET,
	PAGE_COLUMN_NOTEBOOK_INDEX,
	PAGE_COLUMN_NUM_COLS
};

enum {
	SIGNAL_PAGE_ADDED,
	SIGNAL_PAGE_REMOVED,
	SIGNAL_LAST
};

static guint signals[SIGNAL_LAST] = { 0 };

struct _IogSidebarPrivate {
	GtkWidget *notebook;
	GtkWidget *select_button;
	GtkWidget *menu;
	GtkWidget *hbox;
	GtkWidget *label;

	GtkTreeModel *page_model;
};

#if GTK_CHECK_VERSION (3, 2, 0)
G_DEFINE_TYPE (IogSidebar, iog_sidebar, GTK_TYPE_BOX)
#else
G_DEFINE_TYPE (IogSidebar, iog_sidebar, GTK_TYPE_VBOX)
#endif

#define IOG_SIDEBAR_GET_PRIVATE(object) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((object), IOG_TYPE_SIDEBAR, IogSidebarPrivate))

#if GTK_CHECK_VERSION (3, 2, 0)
#define gtk_hbox_new(X,Y) gtk_box_new(GTK_ORIENTATION_HORIZONTAL,Y)
#define gtk_vbox_new(X,Y) gtk_box_new(GTK_ORIENTATION_VERTICAL,Y)
#endif

static void
#if GTK_CHECK_VERSION(3, 0, 0)
iog_sidebar_destroy (GtkWidget *object)
#else
iog_sidebar_destroy (GtkObject *object)
#endif
{
	IogSidebar *iog_sidebar = IOG_SIDEBAR (object);

	if (iog_sidebar->priv->menu) {
		gtk_menu_detach (GTK_MENU (iog_sidebar->priv->menu));
		iog_sidebar->priv->menu = NULL;
	}

	if (iog_sidebar->priv->page_model) {
		g_object_unref (iog_sidebar->priv->page_model);
		iog_sidebar->priv->page_model = NULL;
	}

#if GTK_CHECK_VERSION(3, 0, 0)
	(* GTK_WIDGET_CLASS (iog_sidebar_parent_class)->destroy) (object);
#else
	(* GTK_OBJECT_CLASS (iog_sidebar_parent_class)->destroy) (object);
#endif
}

static void
iog_sidebar_select_page (IogSidebar *iog_sidebar, GtkTreeIter *iter)
{
	gchar *title;
	gint index;

	gtk_tree_model_get (iog_sidebar->priv->page_model, iter,
			    PAGE_COLUMN_TITLE, &title,
			    PAGE_COLUMN_NOTEBOOK_INDEX, &index,
			    -1);

	gtk_notebook_set_current_page (GTK_NOTEBOOK (iog_sidebar->priv->notebook), index);
	gtk_label_set_text (GTK_LABEL (iog_sidebar->priv->label), title);

	g_free (title);
}

void
iog_sidebar_set_page (IogSidebar   *iog_sidebar,
		     GtkWidget   *main_widget)
{
	GtkTreeIter iter;
	gboolean valid;

	valid = gtk_tree_model_get_iter_first (iog_sidebar->priv->page_model, &iter);

	while (valid) {
		GtkWidget *widget;

		gtk_tree_model_get (iog_sidebar->priv->page_model, &iter,
				    PAGE_COLUMN_MAIN_WIDGET, &widget,
				    -1);

		if (widget == main_widget) {
			iog_sidebar_select_page (iog_sidebar, &iter);
			valid = FALSE;
		} else {
			valid = gtk_tree_model_iter_next (iog_sidebar->priv->page_model, &iter);
		}

		g_object_unref (widget);
	}

	g_object_notify (G_OBJECT (iog_sidebar), "current-page");
}

static GtkWidget *
iog_sidebar_get_current_page (IogSidebar *sidebar)
{
	GtkNotebook *notebook = GTK_NOTEBOOK (sidebar->priv->notebook);

	return gtk_notebook_get_nth_page
		(notebook, gtk_notebook_get_current_page (notebook));
}

static void
iog_sidebar_set_property (GObject     *object,
		         guint         prop_id,
		         const GValue *value,
		         GParamSpec   *pspec)
{
	IogSidebar *sidebar = IOG_SIDEBAR (object);

	switch (prop_id) {
	case PROP_CURRENT_PAGE:
		iog_sidebar_set_page (sidebar, g_value_get_object (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
iog_sidebar_get_property (GObject    *object,
		          guint       prop_id,
		          GValue     *value,
		          GParamSpec *pspec)
{
	IogSidebar *sidebar = IOG_SIDEBAR (object);

	switch (prop_id) {
	case PROP_CURRENT_PAGE:
		g_value_set_object (value, iog_sidebar_get_current_page (sidebar));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
iog_sidebar_class_init (IogSidebarClass *iog_sidebar_class)
{
	GObjectClass *g_object_class;
#if GTK_CHECK_VERSION(3, 0, 0)
	GtkWidgetClass *widget_class;
#else
	GtkObjectClass *gtk_object_klass;
#endif

	g_object_class = G_OBJECT_CLASS (iog_sidebar_class);
#if GTK_CHECK_VERSION(3, 0, 0)
	widget_class = GTK_WIDGET_CLASS (iog_sidebar_class);
#else
	gtk_object_klass = GTK_OBJECT_CLASS (iog_sidebar_class);
#endif

	g_type_class_add_private (g_object_class, sizeof (IogSidebarPrivate));

#if GTK_CHECK_VERSION(3, 0, 0)
	widget_class->destroy = iog_sidebar_destroy;
#else
	gtk_object_klass->destroy = iog_sidebar_destroy;
#endif
	g_object_class->get_property = iog_sidebar_get_property;
	g_object_class->set_property = iog_sidebar_set_property;

	g_object_class_install_property (g_object_class,
					 PROP_CURRENT_PAGE,
					 g_param_spec_object ("current-page",
							      "Current page",
							      "The currently visible page",
							      GTK_TYPE_WIDGET,
							      G_PARAM_READWRITE));

	signals[SIGNAL_PAGE_ADDED] =
		g_signal_new ("page-added",
			      IOG_TYPE_SIDEBAR,
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (IogSidebarClass, page_added),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1,
			      GTK_TYPE_WIDGET);

	signals[SIGNAL_PAGE_REMOVED] =
		g_signal_new ("page-removed",
			      IOG_TYPE_SIDEBAR,
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (IogSidebarClass, page_removed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1,
			      GTK_TYPE_WIDGET);
}

static void
iog_sidebar_menu_position_under (GtkMenu  *menu,
				 gint     *x,
				 gint     *y,
				 gboolean *push_in,
				 gpointer  user_data)
{
	GtkWidget *widget;
	GtkAllocation allocation;

	g_return_if_fail (GTK_IS_BUTTON (user_data));
	g_return_if_fail (!gtk_widget_get_has_window (user_data));

	widget = GTK_WIDGET (user_data);
	gtk_widget_get_allocation (widget, &allocation);

	gdk_window_get_origin (gtk_widget_get_window (widget), x, y);

	*x += allocation.x;
	*y += allocation.y + allocation.height;

	*push_in = FALSE;
}

static gboolean
iog_sidebar_select_button_press_cb (GtkWidget      *widget,
				    GdkEventButton *event,
				    gpointer        user_data)
{
	IogSidebar *iog_sidebar = IOG_SIDEBAR (user_data);

	if (event->button == 1) {
		GtkRequisition requisition;
		GtkAllocation allocation;

		gtk_widget_get_allocation (widget, &allocation);

		gtk_widget_set_size_request (iog_sidebar->priv->menu, -1, -1);
#if GTK_CHECK_VERSION (3, 0, 0)
		gtk_widget_get_preferred_size (iog_sidebar->priv->menu, &requisition, NULL);
#else
		gtk_widget_size_request (iog_sidebar->priv->menu, &requisition);
#endif
		gtk_widget_set_size_request (iog_sidebar->priv->menu,
					     MAX (allocation.width,
						  requisition.width), -1);

		gtk_widget_grab_focus (widget);

		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);

		gtk_menu_popup (GTK_MENU (iog_sidebar->priv->menu),
				NULL, NULL, iog_sidebar_menu_position_under, widget,
				event->button, event->time);

		return TRUE;
	}

	return FALSE;
}

static gboolean
iog_sidebar_select_button_key_press_cb (GtkWidget   *widget,
				        GdkEventKey *event,
				        gpointer     user_data)
{
	IogSidebar *iog_sidebar = IOG_SIDEBAR (user_data);

	if (event->keyval == GDK_KEY_space ||
	    event->keyval == GDK_KEY_KP_Space ||
	    event->keyval == GDK_KEY_Return ||
	    event->keyval == GDK_KEY_KP_Enter) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);

		gtk_menu_popup (GTK_MENU (iog_sidebar->priv->menu),
			        NULL, NULL, iog_sidebar_menu_position_under, widget,
				1, event->time);

		return TRUE;
	}

	return FALSE;
}

static void
iog_sidebar_close_clicked_cb (GtkWidget *widget,
 			      gpointer   user_data)
{
	IogSidebar *iog_sidebar = IOG_SIDEBAR (user_data);

	gtk_widget_hide (GTK_WIDGET (iog_sidebar));
}

static void
iog_sidebar_menu_deactivate_cb (GtkWidget *widget,
			       gpointer   user_data)
{
	GtkWidget *menu_button;

	menu_button = GTK_WIDGET (user_data);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (menu_button), FALSE);
}

static void
iog_sidebar_menu_detach_cb (GtkWidget *widget,
			   GtkMenu   *menu)
{
	IogSidebar *iog_sidebar = IOG_SIDEBAR (widget);

	iog_sidebar->priv->menu = NULL;
}

static void
iog_sidebar_menu_item_activate_cb (GtkWidget *widget,
				   gpointer   user_data)
{
	IogSidebar *iog_sidebar = IOG_SIDEBAR (user_data);
	GtkTreeIter iter;
	GtkWidget *menu_item, *item;
	gboolean valid;

	menu_item = gtk_menu_get_active (GTK_MENU (iog_sidebar->priv->menu));
	valid = gtk_tree_model_get_iter_first (iog_sidebar->priv->page_model, &iter);

	while (valid) {
		gtk_tree_model_get (iog_sidebar->priv->page_model, &iter,
				    PAGE_COLUMN_MENU_ITEM, &item,
				    -1);

		if (item == menu_item) {
			iog_sidebar_select_page (iog_sidebar, &iter);
			valid = FALSE;
		} else {
			valid = gtk_tree_model_iter_next (iog_sidebar->priv->page_model, &iter);
		}

		g_object_unref (item);
	}

	g_object_notify (G_OBJECT (iog_sidebar), "current-page");
}

static void
iog_sidebar_init (IogSidebar *iog_sidebar)
{
	GtkWidget *hbox;
	GtkWidget *close_button;
	GtkWidget *select_hbox;
	GtkWidget *arrow;
	GtkWidget *image;

#if GTK_CHECK_VERSION (3, 2, 0)
	gtk_orientable_set_orientation (GTK_ORIENTABLE (iog_sidebar),
					GTK_ORIENTATION_VERTICAL);
#endif

	iog_sidebar->priv = IOG_SIDEBAR_GET_PRIVATE (iog_sidebar);

	/* data model */
	iog_sidebar->priv->page_model = (GtkTreeModel *)
			gtk_list_store_new (PAGE_COLUMN_NUM_COLS,
					    G_TYPE_STRING,
					    GTK_TYPE_WIDGET,
					    GTK_TYPE_WIDGET,
					    G_TYPE_INT);

	/* top option menu */
	hbox = gtk_hbox_new (FALSE, 0);
	iog_sidebar->priv->hbox = hbox;
	gtk_box_pack_start (GTK_BOX (iog_sidebar), hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);

	iog_sidebar->priv->select_button = gtk_toggle_button_new ();
	gtk_button_set_relief (GTK_BUTTON (iog_sidebar->priv->select_button),
			       GTK_RELIEF_NONE);

	g_signal_connect (iog_sidebar->priv->select_button, "button_press_event",
			  G_CALLBACK (iog_sidebar_select_button_press_cb),
			  iog_sidebar);

	g_signal_connect (iog_sidebar->priv->select_button, "key_press_event",
			  G_CALLBACK (iog_sidebar_select_button_key_press_cb),
			  iog_sidebar);

	select_hbox = gtk_hbox_new (FALSE, 0);

	iog_sidebar->priv->label = gtk_label_new ("");

	gtk_box_pack_start (GTK_BOX (select_hbox),
			    iog_sidebar->priv->label,
			    FALSE, FALSE, 0);

	gtk_widget_show (iog_sidebar->priv->label);

#if GTK_CHECK_VERSION (3, 14, 0)
	arrow = gtk_image_new_from_icon_name ("pan-down-symbolic", GTK_ICON_SIZE_BUTTON);
#else
	arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE);
#endif
	gtk_box_pack_end (GTK_BOX (select_hbox), arrow, FALSE, FALSE, 0);
	gtk_widget_show (arrow);

	gtk_container_add (GTK_CONTAINER (iog_sidebar->priv->select_button), select_hbox);
	gtk_widget_show (select_hbox);

	gtk_box_pack_start (GTK_BOX (hbox), iog_sidebar->priv->select_button, TRUE, TRUE, 0);
	gtk_widget_show (iog_sidebar->priv->select_button);

	close_button = gtk_button_new ();

	gtk_button_set_relief (GTK_BUTTON (close_button), GTK_RELIEF_NONE);

	g_signal_connect (close_button, "clicked",
			  G_CALLBACK (iog_sidebar_close_clicked_cb),
			  iog_sidebar);

	image = gtk_image_new_from_stock (GTK_STOCK_CLOSE,
					  GTK_ICON_SIZE_MENU);
	gtk_container_add (GTK_CONTAINER (close_button), image);
	gtk_widget_show (image);

	gtk_box_pack_end (GTK_BOX (hbox), close_button, FALSE, FALSE, 0);
	gtk_widget_show (close_button);

	iog_sidebar->priv->menu = gtk_menu_new ();

	g_signal_connect (iog_sidebar->priv->menu, "deactivate",
			  G_CALLBACK (iog_sidebar_menu_deactivate_cb),
			  iog_sidebar->priv->select_button);

	gtk_menu_attach_to_widget (GTK_MENU (iog_sidebar->priv->menu),
				   GTK_WIDGET (iog_sidebar),
				   iog_sidebar_menu_detach_cb);

	gtk_widget_show (iog_sidebar->priv->menu);

	iog_sidebar->priv->notebook = gtk_notebook_new ();

	gtk_notebook_set_show_border (GTK_NOTEBOOK (iog_sidebar->priv->notebook), FALSE);
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (iog_sidebar->priv->notebook), FALSE);

	gtk_box_pack_start (GTK_BOX (iog_sidebar), iog_sidebar->priv->notebook,
			    TRUE, TRUE, 0);

	gtk_widget_show (iog_sidebar->priv->notebook);
}

GtkWidget *
iog_sidebar_new (void)
{
	GtkWidget *iog_sidebar;

	iog_sidebar = g_object_new (IOG_TYPE_SIDEBAR, NULL);

	return iog_sidebar;
}

void
iog_sidebar_add_page (IogSidebar   *iog_sidebar,
		      const gchar  *title,
		      GtkWidget    *main_widget)
{
	GtkTreeIter iter;
	GtkWidget *menu_item;
	gchar *label_title;
	gint index;

	g_return_if_fail (IOG_IS_SIDEBAR (iog_sidebar));
	g_return_if_fail (GTK_IS_WIDGET (main_widget));

	index = gtk_notebook_append_page (GTK_NOTEBOOK (iog_sidebar->priv->notebook),
					  main_widget, NULL);

	menu_item = gtk_image_menu_item_new_with_label (title);

	g_signal_connect (menu_item, "activate",
			  G_CALLBACK (iog_sidebar_menu_item_activate_cb),
			  iog_sidebar);

	gtk_widget_show (menu_item);

	gtk_menu_shell_append (GTK_MENU_SHELL (iog_sidebar->priv->menu),
			       menu_item);

	/* Insert and move to end */
	gtk_list_store_insert_with_values (GTK_LIST_STORE (iog_sidebar->priv->page_model),
					   &iter, 0,
					   PAGE_COLUMN_TITLE, title,
					   PAGE_COLUMN_MENU_ITEM, menu_item,
					   PAGE_COLUMN_MAIN_WIDGET, main_widget,
					   PAGE_COLUMN_NOTEBOOK_INDEX, index,
					   -1);

	gtk_list_store_move_before (GTK_LIST_STORE(iog_sidebar->priv->page_model),
				    &iter,
				    NULL);

	/* Set the first item added as active */
	gtk_tree_model_get_iter_first (iog_sidebar->priv->page_model, &iter);
	gtk_tree_model_get (iog_sidebar->priv->page_model,
			    &iter,
			    PAGE_COLUMN_TITLE, &label_title,
			    PAGE_COLUMN_NOTEBOOK_INDEX, &index,
			    -1);

	gtk_menu_set_active (GTK_MENU (iog_sidebar->priv->menu), index);

	gtk_label_set_text (GTK_LABEL (iog_sidebar->priv->label), label_title);

	gtk_notebook_set_current_page (GTK_NOTEBOOK (iog_sidebar->priv->notebook),
				       index);

	g_free (label_title);

	g_signal_emit (G_OBJECT (iog_sidebar),
		       signals[SIGNAL_PAGE_ADDED], 0, main_widget);
}

void
iog_sidebar_remove_page (IogSidebar *iog_sidebar, GtkWidget *main_widget)
{
	GtkTreeIter iter;
	GtkWidget *widget, *menu_item;
	gboolean valid;
	gint index;

	g_return_if_fail (IOG_IS_SIDEBAR (iog_sidebar));
	g_return_if_fail (GTK_IS_WIDGET (main_widget));

	valid = gtk_tree_model_get_iter_first (iog_sidebar->priv->page_model, &iter);

	while (valid) {
		gtk_tree_model_get (iog_sidebar->priv->page_model, &iter,
				    PAGE_COLUMN_NOTEBOOK_INDEX, &index,
				    PAGE_COLUMN_MENU_ITEM, &menu_item,
				    PAGE_COLUMN_MAIN_WIDGET, &widget,
				    -1);

		if (widget == main_widget) {
			break;
		} else {
			valid = gtk_tree_model_iter_next (iog_sidebar->priv->page_model,
							  &iter);
		}

		g_object_unref (menu_item);
		g_object_unref (widget);
	}

	if (valid) {
		gtk_notebook_remove_page (GTK_NOTEBOOK (iog_sidebar->priv->notebook),
					  index);

		gtk_container_remove (GTK_CONTAINER (iog_sidebar->priv->menu), menu_item);

		gtk_list_store_remove (GTK_LIST_STORE (iog_sidebar->priv->page_model),
				       &iter);

		g_signal_emit (G_OBJECT (iog_sidebar),
			       signals[SIGNAL_PAGE_REMOVED], 0, main_widget);
	}
}

gint
iog_sidebar_get_n_pages (IogSidebar *iog_sidebar)
{
	g_return_val_if_fail (IOG_IS_SIDEBAR (iog_sidebar), TRUE);

	return gtk_tree_model_iter_n_children (
		GTK_TREE_MODEL (iog_sidebar->priv->page_model), NULL);
}

gboolean
iog_sidebar_is_empty (IogSidebar *iog_sidebar)
{
	g_return_val_if_fail (IOG_IS_SIDEBAR (iog_sidebar), TRUE);

	return gtk_tree_model_iter_n_children (
		GTK_TREE_MODEL (iog_sidebar->priv->page_model), NULL) == 0;
}
