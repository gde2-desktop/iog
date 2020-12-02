/* Eye Of Gde2 - Image Store
 *
 * Copyright (C) 2006-2007 The Free Software Foundation
 *
 * Author: Claudio Saavedra <csaavedra@alumnos.utalca.cl>
 *
 * Based on code by: Jens Finke <jens@triq.net>
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

#ifndef IOG_LIST_STORE_H
#define IOG_LIST_STORE_H

#include <gtk/gtk.h>
#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#ifndef __IOG_IMAGE_DECLR__
#define __IOG_IMAGE_DECLR__
  typedef struct _IogImage IogImage;
#endif

typedef struct _IogListStore IogListStore;
typedef struct _IogListStoreClass IogListStoreClass;
typedef struct _IogListStorePrivate IogListStorePrivate;

#define IOG_TYPE_LIST_STORE            iog_list_store_get_type()
#define IOG_LIST_STORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), IOG_TYPE_LIST_STORE, IogListStore))
#define IOG_LIST_STORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  IOG_TYPE_LIST_STORE, IogListStoreClass))
#define IOG_IS_LIST_STORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IOG_TYPE_LIST_STORE))
#define IOG_IS_LIST_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  IOG_TYPE_LIST_STORE))
#define IOG_LIST_STORE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  IOG_TYPE_LIST_STORE, IogListStoreClass))

#define IOG_LIST_STORE_THUMB_SIZE 90

typedef enum {
	IOG_LIST_STORE_THUMBNAIL = 0,
	IOG_LIST_STORE_THUMB_SET,
	IOG_LIST_STORE_IOG_IMAGE,
	IOG_LIST_STORE_IOG_JOB,
	IOG_LIST_STORE_NUM_COLUMNS
} IogListStoreColumn;

struct _IogListStore {
        GtkListStore parent;
	IogListStorePrivate *priv;
};

struct _IogListStoreClass {
        GtkListStoreClass parent_class;

	/* Padding for future expansion */
	void (* _iog_reserved1) (void);
	void (* _iog_reserved2) (void);
	void (* _iog_reserved3) (void);
	void (* _iog_reserved4) (void);
};

GType           iog_list_store_get_type 	     (void) G_GNUC_CONST;

GtkListStore   *iog_list_store_new 		     (void);

GtkListStore   *iog_list_store_new_from_glist 	     (GList *list);

void            iog_list_store_append_image 	     (IogListStore *store,
						      IogImage     *image);

void            iog_list_store_add_files 	     (IogListStore *store,
						      GList        *file_list);

void            iog_list_store_remove_image 	     (IogListStore *store,
						      IogImage     *image);

gint            iog_list_store_get_pos_by_image      (IogListStore *store,
						      IogImage     *image);

IogImage       *iog_list_store_get_image_by_pos      (IogListStore *store,
						      gint   pos);

gint            iog_list_store_get_pos_by_iter 	     (IogListStore *store,
						      GtkTreeIter  *iter);

gint            iog_list_store_length                (IogListStore *store);

gint            iog_list_store_get_initial_pos 	     (IogListStore *store);

void            iog_list_store_thumbnail_set         (IogListStore *store,
						      GtkTreeIter *iter);

void            iog_list_store_thumbnail_unset       (IogListStore *store,
						      GtkTreeIter *iter);

void            iog_list_store_thumbnail_refresh     (IogListStore *store,
						      GtkTreeIter *iter);

G_END_DECLS

#endif
