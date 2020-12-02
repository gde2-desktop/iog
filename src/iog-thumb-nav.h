/* Eye Of Gde2 - Thumbnail Navigator
 *
 * Copyright (C) 2006 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
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

#ifndef __IOG_THUMB_NAV_H__
#define __IOG_THUMB_NAV_H__

#include "iog-thumb-view.h"

#include <gtk/gtk.h>
#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _IogThumbNav IogThumbNav;
typedef struct _IogThumbNavClass IogThumbNavClass;
typedef struct _IogThumbNavPrivate IogThumbNavPrivate;

#define IOG_TYPE_THUMB_NAV            (iog_thumb_nav_get_type ())
#define IOG_THUMB_NAV(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), IOG_TYPE_THUMB_NAV, IogThumbNav))
#define IOG_THUMB_NAV_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  IOG_TYPE_THUMB_NAV, IogThumbNavClass))
#define IOG_IS_THUMB_NAV(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), IOG_TYPE_THUMB_NAV))
#define IOG_IS_THUMB_NAV_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  IOG_TYPE_THUMB_NAV))
#define IOG_THUMB_NAV_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  IOG_TYPE_THUMB_NAV, IogThumbNavClass))

typedef enum {
	IOG_THUMB_NAV_MODE_ONE_ROW,
	IOG_THUMB_NAV_MODE_ONE_COLUMN,
	IOG_THUMB_NAV_MODE_MULTIPLE_ROWS,
	IOG_THUMB_NAV_MODE_MULTIPLE_COLUMNS
} IogThumbNavMode;

struct _IogThumbNav {
#if GTK_CHECK_VERSION (3, 2, 0)
	GtkBox base_instance;
#else
	GtkHBox base_instance;
#endif

	IogThumbNavPrivate *priv;
};

struct _IogThumbNavClass {
#if GTK_CHECK_VERSION (3, 2, 0)
	GtkBoxClass parent_class;
#else
	GtkHBoxClass parent_class;
#endif
};

GType	         iog_thumb_nav_get_type          (void) G_GNUC_CONST;

GtkWidget       *iog_thumb_nav_new               (GtkWidget         *thumbview,
						  IogThumbNavMode    mode,
	             			          gboolean           show_buttons);

gboolean         iog_thumb_nav_get_show_buttons  (IogThumbNav       *nav);

void             iog_thumb_nav_set_show_buttons  (IogThumbNav       *nav,
                                                  gboolean           show_buttons);

IogThumbNavMode  iog_thumb_nav_get_mode          (IogThumbNav       *nav);

void             iog_thumb_nav_set_mode          (IogThumbNav       *nav,
                                                  IogThumbNavMode    mode);

G_END_DECLS

#endif /* __IOG_THUMB_NAV_H__ */
