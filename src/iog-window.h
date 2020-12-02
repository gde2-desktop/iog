/* Eye of Gde2 - Main Window
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

#ifndef __IOG_WINDOW_H__
#define __IOG_WINDOW_H__

#include "iog-list-store.h"
#include "iog-image.h"

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _IogWindow IogWindow;
typedef struct _IogWindowClass IogWindowClass;
typedef struct _IogWindowPrivate IogWindowPrivate;

#define IOG_TYPE_WINDOW            (iog_window_get_type ())
#define IOG_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), IOG_TYPE_WINDOW, IogWindow))
#define IOG_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  IOG_TYPE_WINDOW, IogWindowClass))
#define IOG_IS_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IOG_TYPE_WINDOW))
#define IOG_IS_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  IOG_TYPE_WINDOW))
#define IOG_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  IOG_TYPE_WINDOW, IogWindowClass))

#define IOG_WINDOW_ERROR           (iog_window_error_quark ())

typedef enum {
	IOG_WINDOW_MODE_UNKNOWN,
	IOG_WINDOW_MODE_NORMAL,
	IOG_WINDOW_MODE_FULLSCREEN,
	IOG_WINDOW_MODE_SLIDESHOW
} IogWindowMode;

typedef enum {
	IOG_WINDOW_COLLECTION_POS_BOTTOM,
	IOG_WINDOW_COLLECTION_POS_LEFT,
	IOG_WINDOW_COLLECTION_POS_TOP,
	IOG_WINDOW_COLLECTION_POS_RIGHT
} IogWindowCollectionPos;

//TODO
typedef enum {
	IOG_WINDOW_ERROR_CONTROL_NOT_FOUND,
	IOG_WINDOW_ERROR_UI_NOT_FOUND,
	IOG_WINDOW_ERROR_NO_PERSIST_FILE_INTERFACE,
	IOG_WINDOW_ERROR_IO,
	IOG_WINDOW_ERROR_TRASH_NOT_FOUND,
	IOG_WINDOW_ERROR_GENERIC,
	IOG_WINDOW_ERROR_UNKNOWN
} IogWindowError;

typedef enum {
	IOG_STARTUP_FULLSCREEN         = 1 << 0,
	IOG_STARTUP_SLIDE_SHOW         = 1 << 1,
	IOG_STARTUP_DISABLE_COLLECTION = 1 << 2
} IogStartupFlags;

struct _IogWindow {
	GtkWindow win;

	IogWindowPrivate *priv;
};

struct _IogWindowClass {
	GtkWindowClass parent_class;

	void (* prepared) (IogWindow *window);
};

GType         iog_window_get_type  	(void) G_GNUC_CONST;

GtkWidget    *iog_window_new		(IogStartupFlags  flags);

IogWindowMode iog_window_get_mode       (IogWindow       *window);

void          iog_window_set_mode       (IogWindow       *window,
					 IogWindowMode    mode);

GtkUIManager *iog_window_get_ui_manager (IogWindow       *window);

IogListStore *iog_window_get_store      (IogWindow       *window);

GtkWidget    *iog_window_get_view       (IogWindow       *window);

GtkWidget    *iog_window_get_sidebar    (IogWindow       *window);

GtkWidget    *iog_window_get_thumb_view (IogWindow       *window);

GtkWidget    *iog_window_get_thumb_nav  (IogWindow       *window);

GtkWidget    *iog_window_get_statusbar  (IogWindow       *window);

IogImage     *iog_window_get_image      (IogWindow       *window);

void          iog_window_open_file_list	(IogWindow       *window,
					 GSList          *file_list);

gboolean      iog_window_is_empty 	(IogWindow       *window);

void          iog_window_reload_image (IogWindow *window);
G_END_DECLS

#endif
