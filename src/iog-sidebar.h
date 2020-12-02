/* Eye of Gde2 - Side bar
 *
 * Copyright (C) 2004 Red Hat, Inc.
 * Copyright (C) 2007 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 *
 * Based on evince code (shell/ev-sidebar.h) by:
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

#ifndef __IOG_SIDEBAR_H__
#define __IOG_SIDEBAR_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _IogSidebar IogSidebar;
typedef struct _IogSidebarClass IogSidebarClass;
typedef struct _IogSidebarPrivate IogSidebarPrivate;

#define IOG_TYPE_SIDEBAR	    (iog_sidebar_get_type())
#define IOG_SIDEBAR(obj)	    (G_TYPE_CHECK_INSTANCE_CAST((obj), IOG_TYPE_SIDEBAR, IogSidebar))
#define IOG_SIDEBAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  IOG_TYPE_SIDEBAR, IogSidebarClass))
#define IOG_IS_SIDEBAR(obj)	    (G_TYPE_CHECK_INSTANCE_TYPE((obj), IOG_TYPE_SIDEBAR))
#define IOG_IS_SIDEBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  IOG_TYPE_SIDEBAR))
#define IOG_SIDEBAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  IOG_TYPE_SIDEBAR, IogSidebarClass))

struct _IogSidebar {
#if GTK_CHECK_VERSION (3, 2, 0)
	GtkBox base_instance;
#else
	GtkVBox base_instance;
#endif

	IogSidebarPrivate *priv;
};

struct _IogSidebarClass {
#if GTK_CHECK_VERSION (3, 2, 0)
	GtkBoxClass base_class;
#else
	GtkVBoxClass base_class;
#endif

	void (* page_added)   (IogSidebar *iog_sidebar,
			       GtkWidget  *main_widget);

	void (* page_removed) (IogSidebar *iog_sidebar,
			       GtkWidget  *main_widget);
};

GType      iog_sidebar_get_type     (void);

GtkWidget *iog_sidebar_new          (void);

void       iog_sidebar_add_page     (IogSidebar  *iog_sidebar,
				     const gchar *title,
				     GtkWidget   *main_widget);

void       iog_sidebar_remove_page  (IogSidebar  *iog_sidebar,
				     GtkWidget   *main_widget);

void       iog_sidebar_set_page     (IogSidebar  *iog_sidebar,
				     GtkWidget   *main_widget);

gint       iog_sidebar_get_n_pages  (IogSidebar  *iog_sidebar);

gboolean   iog_sidebar_is_empty     (IogSidebar  *iog_sidebar);

G_END_DECLS

#endif /* __IOG_SIDEBAR_H__ */


