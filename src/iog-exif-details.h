/* Eye Of Gde2 - IOG Image Exif Details
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

#ifndef __IOG_EXIF_DETAILS__
#define __IOG_EXIF_DETAILS__

#include <glib-object.h>
#include <gtk/gtk.h>
#if HAVE_EXIF
#include <libexif/exif-data.h>
#endif
#if HAVE_EXEMPI
#include <exempi/xmp.h>
#endif

G_BEGIN_DECLS

typedef struct _IogExifDetails IogExifDetails;
typedef struct _IogExifDetailsClass IogExifDetailsClass;
typedef struct _IogExifDetailsPrivate IogExifDetailsPrivate;

#define IOG_TYPE_EXIF_DETAILS            (iog_exif_details_get_type ())
#define IOG_EXIF_DETAILS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), IOG_TYPE_EXIF_DETAILS, IogExifDetails))
#define IOG_EXIF_DETAILS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), IOG_TYPE_EXIF_DETAILS, IogExifDetailsClass))
#define IOG_IS_EXIF_DETAILS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IOG_TYPE_EXIF_DETAILS))
#define IOG_IS_EXIF_DETAILS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), IOG_TYPE_EXIF_DETAILS))
#define IOG_EXIF_DETAILS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), IOG_TYPE_EXIF_DETAILS, IogExifDetailsClass))

struct _IogExifDetails {
        GtkTreeView parent;

	IogExifDetailsPrivate *priv;
};

struct _IogExifDetailsClass {
	GtkTreeViewClass parent_class;
};

GType               iog_exif_details_get_type    (void) G_GNUC_CONST;

GtkWidget          *iog_exif_details_new         (void);

#if HAVE_EXIF
void                iog_exif_details_update      (IogExifDetails *view,
						  ExifData       *data);
#endif
#if HAVE_EXEMPI
void                iog_exif_details_xmp_update  (IogExifDetails *view,
							XmpPtr          xmp_data);
#endif

G_END_DECLS

#endif /* __IOG_EXIF_DETAILS__ */
