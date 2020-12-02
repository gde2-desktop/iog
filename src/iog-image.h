/* Eye Of Gde2 - Image
 *
 * Copyright (C) 2007 The Free Software Foundation
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

#ifndef __IOG_IMAGE_H__
#define __IOG_IMAGE_H__

#include "iog-jobs.h"
#include "iog-window.h"
#include "iog-transform.h"
#include "iog-image-save-info.h"
#include "iog-enums.h"

#include <glib.h>
#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#ifdef HAVE_EXIF
#include <libexif/exif-data.h>
#include "iog-exif-util.h"
#endif

#ifdef HAVE_LCMS
#include <lcms2.h>
#endif

#ifdef HAVE_EXEMPI
#include <exempi/xmp.h>
#endif

#ifdef HAVE_RSVG
#include <librsvg/rsvg.h>
#endif

G_BEGIN_DECLS

#ifndef __IOG_IMAGE_DECLR__
#define __IOG_IMAGE_DECLR__
typedef struct _IogImage IogImage;
#endif
typedef struct _IogImageClass IogImageClass;
typedef struct _IogImagePrivate IogImagePrivate;

#define IOG_TYPE_IMAGE            (iog_image_get_type ())
#define IOG_IMAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), IOG_TYPE_IMAGE, IogImage))
#define IOG_IMAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  IOG_TYPE_IMAGE, IogImageClass))
#define IOG_IS_IMAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), IOG_TYPE_IMAGE))
#define IOG_IS_IMAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  IOG_TYPE_IMAGE))
#define IOG_IMAGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  IOG_TYPE_IMAGE, IogImageClass))

typedef enum {
	IOG_IMAGE_ERROR_SAVE_NOT_LOCAL,
	IOG_IMAGE_ERROR_NOT_LOADED,
	IOG_IMAGE_ERROR_VFS,
	IOG_IMAGE_ERROR_FILE_EXISTS,
	IOG_IMAGE_ERROR_TMP_FILE_FAILED,
	IOG_IMAGE_ERROR_GENERIC,
	IOG_IMAGE_ERROR_UNKNOWN
} IogImageError;

#define IOG_IMAGE_ERROR iog_image_error_quark ()

typedef enum {
	IOG_IMAGE_STATUS_UNKNOWN,
	IOG_IMAGE_STATUS_LOADING,
	IOG_IMAGE_STATUS_LOADED,
	IOG_IMAGE_STATUS_SAVING,
	IOG_IMAGE_STATUS_FAILED
} IogImageStatus;

typedef enum {
  IOG_IMAGE_METADATA_NOT_READ,
  IOG_IMAGE_METADATA_NOT_AVAILABLE,
  IOG_IMAGE_METADATA_READY
} IogImageMetadataStatus;

struct _IogImage {
	GObject parent;

	IogImagePrivate *priv;
};

struct _IogImageClass {
	GObjectClass parent_class;

	void (* changed) 	   (IogImage *img);

	void (* size_prepared)     (IogImage *img,
				    int       width,
				    int       height);

	void (* thumbnail_changed) (IogImage *img);

	void (* save_progress)     (IogImage *img,
				    gfloat    progress);

	void (* next_frame)        (IogImage *img,
				    gint delay);

	void (* file_changed)      (IogImage *img);
};

GType	          iog_image_get_type	             (void) G_GNUC_CONST;

GQuark            iog_image_error_quark              (void);

IogImage         *iog_image_new                      (const char *txt_uri);

IogImage         *iog_image_new_file                 (GFile *file);

gboolean          iog_image_load                     (IogImage   *img,
					              IogImageData data2read,
					              IogJob     *job,
					              GError    **error);

void              iog_image_cancel_load              (IogImage   *img);

gboolean          iog_image_has_data                 (IogImage   *img,
					              IogImageData data);

void              iog_image_data_ref                 (IogImage   *img);

void              iog_image_data_unref               (IogImage   *img);

void              iog_image_set_thumbnail            (IogImage   *img,
					              GdkPixbuf  *pixbuf);

gboolean          iog_image_save_as_by_info          (IogImage   *img,
		      			              IogImageSaveInfo *source,
		      			              IogImageSaveInfo *target,
		      			              GError    **error);

gboolean          iog_image_save_by_info             (IogImage   *img,
					              IogImageSaveInfo *source,
					              GError    **error);

GdkPixbuf*        iog_image_get_pixbuf               (IogImage   *img);

GdkPixbuf*        iog_image_get_thumbnail            (IogImage   *img);

void              iog_image_get_size                 (IogImage   *img,
					              gint       *width,
					              gint       *height);

goffset           iog_image_get_bytes                (IogImage   *img);

gboolean          iog_image_is_modified              (IogImage   *img);

void              iog_image_modified                 (IogImage   *img);

const gchar*      iog_image_get_caption              (IogImage   *img);

const gchar      *iog_image_get_collate_key          (IogImage   *img);

#if HAVE_EXIF
ExifData*         iog_image_get_exif_info            (IogImage   *img);
#endif

gpointer          iog_image_get_xmp_info             (IogImage   *img);

GFile*            iog_image_get_file                 (IogImage   *img);

gchar*            iog_image_get_uri_for_display      (IogImage   *img);

IogImageStatus    iog_image_get_status               (IogImage   *img);

IogImageMetadataStatus iog_image_get_metadata_status (IogImage   *img);

void              iog_image_transform                (IogImage   *img,
						      IogTransform *trans,
						      IogJob     *job);

#ifdef HAVE_EXIF
void              iog_image_autorotate               (IogImage   *img);
#endif

#ifdef HAVE_LCMS
cmsHPROFILE       iog_image_get_profile              (IogImage    *img);

void              iog_image_apply_display_profile    (IogImage    *img,
						      cmsHPROFILE  display_profile);
#endif

void              iog_image_undo                     (IogImage   *img);

GList		 *iog_image_get_supported_mime_types (void);

gboolean          iog_image_is_supported_mime_type   (const char *mime_type);

gboolean          iog_image_is_animation             (IogImage *img);

gboolean          iog_image_start_animation          (IogImage *img);

#ifdef HAVE_RSVG
gboolean          iog_image_is_svg                   (IogImage *img);
RsvgHandle       *iog_image_get_svg                  (IogImage *img);
IogTransform     *iog_image_get_transform            (IogImage *img);
#endif

void              iog_image_file_changed             (IogImage *img);
gboolean          iog_image_is_file_changed         (IogImage *img);

G_END_DECLS

#endif /* __IOG_IMAGE_H__ */
