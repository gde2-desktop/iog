/* Eye Of Gde2 - Image
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define GDK_PIXBUF_ENABLE_BACKEND

#include "iog-image.h"
#include "iog-image-private.h"
#include "iog-debug.h"

#ifdef HAVE_JPEG
#include "iog-image-jpeg.h"
#endif

#include "iog-marshal.h"
#include "iog-pixbuf-util.h"
#include "iog-metadata-reader.h"
#include "iog-image-save-info.h"
#include "iog-transform.h"
#include "iog-util.h"
#include "iog-jobs.h"
#include "iog-thumbnail.h"

#include <unistd.h>
#include <string.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#ifdef HAVE_EXIF
#include "iog-exif-util.h"
#include <libexif/exif-data.h>
#include <libexif/exif-utils.h>
#include <libexif/exif-loader.h>
#endif

#ifdef HAVE_LCMS
#include <lcms2.h>
#ifndef EXIF_TAG_GAMMA
#define EXIF_TAG_GAMMA 0xa500
#endif
#endif

#define IOG_IMAGE_GET_PRIVATE(object) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((object), IOG_TYPE_IMAGE, IogImagePrivate))

G_DEFINE_TYPE (IogImage, iog_image, G_TYPE_OBJECT)

enum {
	SIGNAL_CHANGED,
	SIGNAL_SIZE_PREPARED,
	SIGNAL_THUMBNAIL_CHANGED,
	SIGNAL_SAVE_PROGRESS,
	SIGNAL_NEXT_FRAME,
	SIGNAL_FILE_CHANGED,
	SIGNAL_LAST
};

static guint signals[SIGNAL_LAST] = { 0 };

static GList *supported_mime_types = NULL;

#define IOG_IMAGE_READ_BUFFER_SIZE 65535

static void
iog_image_free_mem_private (IogImage *image)
{
	IogImagePrivate *priv;

	priv = image->priv;

	if (priv->status == IOG_IMAGE_STATUS_LOADING) {
		iog_image_cancel_load (image);
	} else {
		if (priv->anim_iter != NULL) {
			g_object_unref (priv->anim_iter);
			priv->anim_iter = NULL;
		}

		if (priv->anim != NULL) {
			g_object_unref (priv->anim);
			priv->anim = NULL;
		}

		priv->is_playing = FALSE;

		if (priv->image != NULL) {
			g_object_unref (priv->image);
			priv->image = NULL;
		}

#ifdef HAVE_RSVG
		if (priv->svg != NULL) {
			g_object_unref (priv->svg);
			priv->svg = NULL;
		}
#endif

#ifdef HAVE_EXIF
		if (priv->exif != NULL) {
			exif_data_unref (priv->exif);
			priv->exif = NULL;
		}
#endif

		if (priv->exif_chunk != NULL) {
			g_free (priv->exif_chunk);
			priv->exif_chunk = NULL;
		}

		priv->exif_chunk_len = 0;

#ifdef HAVE_EXEMPI
		if (priv->xmp != NULL) {
			xmp_free (priv->xmp);
			priv->xmp = NULL;
		}
#endif

#ifdef HAVE_LCMS
		if (priv->profile != NULL) {
			cmsCloseProfile (priv->profile);
			priv->profile = NULL;
		}
#endif

		priv->status = IOG_IMAGE_STATUS_UNKNOWN;
		priv->metadata_status = IOG_IMAGE_METADATA_NOT_READ;
	}
}

static void
iog_image_dispose (GObject *object)
{
	IogImagePrivate *priv;

	priv = IOG_IMAGE (object)->priv;

	iog_image_free_mem_private (IOG_IMAGE (object));

	if (priv->file) {
		g_object_unref (priv->file);
		priv->file = NULL;
	}

	if (priv->caption) {
		g_free (priv->caption);
		priv->caption = NULL;
	}

	if (priv->collate_key) {
		g_free (priv->collate_key);
		priv->collate_key = NULL;
	}

	if (priv->file_type) {
		g_free (priv->file_type);
		priv->file_type = NULL;
	}

	g_mutex_clear (&priv->status_mutex);

	if (priv->trans) {
		g_object_unref (priv->trans);
		priv->trans = NULL;
	}

	if (priv->trans_autorotate) {
		g_object_unref (priv->trans_autorotate);
		priv->trans_autorotate = NULL;
	}

	if (priv->undo_stack) {
		g_slist_foreach (priv->undo_stack, (GFunc) g_object_unref, NULL);
		g_slist_free (priv->undo_stack);
		priv->undo_stack = NULL;
	}

	G_OBJECT_CLASS (iog_image_parent_class)->dispose (object);
}

static void
iog_image_class_init (IogImageClass *klass)
{
	GObjectClass *object_class = (GObjectClass*) klass;

	object_class->dispose = iog_image_dispose;

	signals[SIGNAL_SIZE_PREPARED] =
		g_signal_new ("size-prepared",
			      IOG_TYPE_IMAGE,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (IogImageClass, size_prepared),
			      NULL, NULL,
			      iog_marshal_VOID__INT_INT,
			      G_TYPE_NONE, 2,
			      G_TYPE_INT,
			      G_TYPE_INT);

	signals[SIGNAL_CHANGED] =
		g_signal_new ("changed",
			      IOG_TYPE_IMAGE,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (IogImageClass, changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	signals[SIGNAL_THUMBNAIL_CHANGED] =
		g_signal_new ("thumbnail-changed",
			      IOG_TYPE_IMAGE,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (IogImageClass, thumbnail_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	signals[SIGNAL_SAVE_PROGRESS] =
		g_signal_new ("save-progress",
			      IOG_TYPE_IMAGE,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (IogImageClass, save_progress),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__FLOAT,
			      G_TYPE_NONE, 1,
			      G_TYPE_FLOAT);
 	/**
 	 * IogImage::next-frame:
  	 * @img: the object which received the signal.
	 * @delay: number of milliseconds the current frame will be displayed.
	 *
	 * The ::next-frame signal will be emitted each time an anigde2d image
	 * advances to the next frame.
	 */
	signals[SIGNAL_NEXT_FRAME] =
		g_signal_new ("next-frame",
			      IOG_TYPE_IMAGE,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (IogImageClass, next_frame),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__INT,
			      G_TYPE_NONE, 1,
			      G_TYPE_INT);

	signals[SIGNAL_FILE_CHANGED] = g_signal_new ("file-changed",
						     IOG_TYPE_IMAGE,
						     G_SIGNAL_RUN_LAST,
						     G_STRUCT_OFFSET (IogImageClass, file_changed),
						     NULL, NULL,
						     g_cclosure_marshal_VOID__VOID,
						     G_TYPE_NONE, 0);

	g_type_class_add_private (object_class, sizeof (IogImagePrivate));
}

static void
iog_image_init (IogImage *img)
{
	img->priv = IOG_IMAGE_GET_PRIVATE (img);

	img->priv->file = NULL;
	img->priv->image = NULL;
	img->priv->anim = NULL;
	img->priv->anim_iter = NULL;
	img->priv->is_playing = FALSE;
	img->priv->thumbnail = NULL;
	img->priv->width = -1;
	img->priv->height = -1;
	img->priv->modified = FALSE;
	img->priv->file_is_changed = FALSE;
	g_mutex_init (&img->priv->status_mutex);
	img->priv->status = IOG_IMAGE_STATUS_UNKNOWN;
        img->priv->metadata_status = IOG_IMAGE_METADATA_NOT_READ;
	img->priv->is_monitored = FALSE;
	img->priv->undo_stack = NULL;
	img->priv->trans = NULL;
	img->priv->trans_autorotate = NULL;
	img->priv->data_ref_count = 0;
#ifdef HAVE_EXIF
	img->priv->orientation = 0;
	img->priv->autorotate = FALSE;
	img->priv->exif = NULL;
#endif
#ifdef HAVE_EXEMPI
	img->priv->xmp = NULL;
#endif
#ifdef HAVE_LCMS
	img->priv->profile = NULL;
#endif
#ifdef HAVE_RSVG
	img->priv->svg = NULL;
#endif
}

IogImage *
iog_image_new (const char *txt_uri)
{
	IogImage *img;

	img = IOG_IMAGE (g_object_new (IOG_TYPE_IMAGE, NULL));

	img->priv->file = g_file_new_for_uri (txt_uri);

	return img;
}

IogImage *
iog_image_new_file (GFile *file)
{
	IogImage *img;

	img = IOG_IMAGE (g_object_new (IOG_TYPE_IMAGE, NULL));

	img->priv->file = g_object_ref (file);

	return img;
}

GQuark
iog_image_error_quark (void)
{
	static GQuark q = 0;

	if (q == 0) {
		q = g_quark_from_static_string ("iog-image-error-quark");
	}

	return q;
}

static void
iog_image_update_exif_data (IogImage *image)
{
#ifdef HAVE_EXIF
	IogImagePrivate *priv;
	ExifEntry *entry;
	ExifByteOrder bo;

	iog_debug (DEBUG_IMAGE_DATA);

	g_return_if_fail (IOG_IS_IMAGE (image));

	priv = image->priv;

	if (priv->exif == NULL) return;

	bo = exif_data_get_byte_order (priv->exif);

	/* Update image width */
	entry = exif_data_get_entry (priv->exif, EXIF_TAG_PIXEL_X_DIMENSION);
	if (entry != NULL && (priv->width >= 0)) {
		if (entry->format == EXIF_FORMAT_LONG)
			exif_set_long (entry->data, bo, priv->width);
		else if (entry->format == EXIF_FORMAT_SHORT)
			exif_set_short (entry->data, bo, priv->width);
		else
			g_warning ("Exif entry has unsupported size");
	}

	/* Update image height */
	entry = exif_data_get_entry (priv->exif, EXIF_TAG_PIXEL_Y_DIMENSION);
	if (entry != NULL && (priv->height >= 0)) {
		if (entry->format == EXIF_FORMAT_LONG)
			exif_set_long (entry->data, bo, priv->height);
		else if (entry->format == EXIF_FORMAT_SHORT)
			exif_set_short (entry->data, bo, priv->height);
		else
			g_warning ("Exif entry has unsupported size");
	}

	/* Update image orientation */
	entry = exif_data_get_entry (priv->exif, EXIF_TAG_ORIENTATION);
	if (entry != NULL) {
		if (entry->format == EXIF_FORMAT_LONG)
			exif_set_long (entry->data, bo, 1);
		else if (entry->format == EXIF_FORMAT_SHORT)
			exif_set_short (entry->data, bo, 1);
		else
			g_warning ("Exif entry has unsupported size");

		priv->orientation = 1;
	}
#endif
}

static void
iog_image_real_transform (IogImage     *img,
			  IogTransform *trans,
			  gboolean      is_undo,
			  IogJob       *job)
{
	IogImagePrivate *priv;
	GdkPixbuf *transformed;
	gboolean modified = FALSE;

	g_return_if_fail (IOG_IS_IMAGE (img));
	g_return_if_fail (IOG_IS_TRANSFORM (trans));

	priv = img->priv;

	if (priv->image != NULL) {
		transformed = iog_transform_apply (trans, priv->image, job);

		g_object_unref (priv->image);
		priv->image = transformed;

		priv->width = gdk_pixbuf_get_width (transformed);
		priv->height = gdk_pixbuf_get_height (transformed);

		modified = TRUE;
	}

	if (priv->thumbnail != NULL) {
		transformed = iog_transform_apply (trans, priv->thumbnail, NULL);

		g_object_unref (priv->thumbnail);
		priv->thumbnail = transformed;

		modified = TRUE;
	}

	if (modified) {
		priv->modified = TRUE;
		iog_image_update_exif_data (img);
	}

	if (priv->trans == NULL) {
		g_object_ref (trans);
		priv->trans = trans;
	} else {
		IogTransform *composition;

		composition = iog_transform_compose (priv->trans, trans);

		g_object_unref (priv->trans);

		priv->trans = composition;
	}

	if (!is_undo) {
		g_object_ref (trans);
		priv->undo_stack = g_slist_prepend (priv->undo_stack, trans);
	}
}

static gboolean
do_emit_size_prepared_signal (IogImage *img)
{
	g_signal_emit (img, signals[SIGNAL_SIZE_PREPARED], 0,
		       img->priv->width, img->priv->height);
	return FALSE;
}

static void
iog_image_emit_size_prepared (IogImage *img)
{
	g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
	                (GSourceFunc) do_emit_size_prepared_signal,
	                 g_object_ref (img), g_object_unref);
}

static void
iog_image_size_prepared (GdkPixbufLoader *loader,
			 gint             width,
			 gint             height,
			 gpointer         data)
{
	IogImage *img;

	iog_debug (DEBUG_IMAGE_LOAD);

	g_return_if_fail (IOG_IS_IMAGE (data));

	img = IOG_IMAGE (data);

	g_mutex_lock (&img->priv->status_mutex);

	img->priv->width = width;
	img->priv->height = height;

	g_mutex_unlock (&img->priv->status_mutex);

#ifdef HAVE_EXIF
	if (!img->priv->autorotate || img->priv->exif)
#endif
		iog_image_emit_size_prepared (img);
}

static IogMetadataReader*
check_for_metadata_img_format (IogImage *img, guchar *buffer, guint bytes_read)
{
	IogMetadataReader *md_reader = NULL;

	iog_debug_message (DEBUG_IMAGE_DATA, "Check image format for jpeg: %x%x - length: %i",
			   buffer[0], buffer[1], bytes_read);

	if (bytes_read >= 2) {
		/* SOI (start of image) marker for JPEGs is 0xFFD8 */
		if ((buffer[0] == 0xFF) && (buffer[1] == 0xD8)) {
			md_reader = iog_metadata_reader_new (IOG_METADATA_JPEG);
		}
		if (bytes_read >= 8 &&
		    memcmp (buffer, "\x89PNG\x0D\x0A\x1a\x0A", 8) == 0) {
			md_reader = iog_metadata_reader_new (IOG_METADATA_PNG);
		}
	}

	return md_reader;
}

static gboolean
iog_image_needs_transformation (IogImage *img)
{
	g_return_val_if_fail (IOG_IS_IMAGE (img), FALSE);

	return (img->priv->trans != NULL || img->priv->trans_autorotate != NULL);
}

static gboolean
iog_image_apply_transformations (IogImage *img, GError **error)
{
	GdkPixbuf *transformed = NULL;
	IogTransform *composition = NULL;
	IogImagePrivate *priv;

	g_return_val_if_fail (IOG_IS_IMAGE (img), FALSE);

	priv = img->priv;

	if (priv->trans == NULL && priv->trans_autorotate == NULL) {
		return TRUE;
	}

	if (priv->image == NULL) {
		g_set_error (error,
			     IOG_IMAGE_ERROR,
			     IOG_IMAGE_ERROR_NOT_LOADED,
			     _("Transformation on unloaded image."));

		return FALSE;
	}

	if (priv->trans != NULL && priv->trans_autorotate != NULL) {
		composition = iog_transform_compose (priv->trans,
						     priv->trans_autorotate);
	} else if (priv->trans != NULL) {
		composition = g_object_ref (priv->trans);
	} else if (priv->trans_autorotate != NULL) {
		composition = g_object_ref (priv->trans_autorotate);
	}

	if (composition != NULL) {
		transformed = iog_transform_apply (composition, priv->image, NULL);
	}

	g_object_unref (priv->image);
	priv->image = transformed;

	if (transformed != NULL) {
		priv->width = gdk_pixbuf_get_width (priv->image);
		priv->height = gdk_pixbuf_get_height (priv->image);
	} else {
		g_set_error (error,
			     IOG_IMAGE_ERROR,
			     IOG_IMAGE_ERROR_GENERIC,
			     _("Transformation failed."));
 	}

	g_object_unref (composition);

	return (transformed != NULL);
}

static void
iog_image_get_file_info (IogImage *img,
			 goffset *bytes,
			 gchar **mime_type,
			 GError **error)
{
	GFileInfo *file_info;

	file_info = g_file_query_info (img->priv->file,
				       G_FILE_ATTRIBUTE_STANDARD_SIZE ","
				       G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
				       0, NULL, error);

	if (file_info == NULL) {
		if (bytes)
			*bytes = 0;

		if (mime_type)
			*mime_type = NULL;

		g_set_error (error,
			     IOG_IMAGE_ERROR,
			     IOG_IMAGE_ERROR_VFS,
			     "Error in getting image file info");
	} else {
		if (bytes)
			*bytes = g_file_info_get_size (file_info);

		if (mime_type)
			*mime_type = g_strdup (g_file_info_get_content_type (file_info));
		g_object_unref (file_info);
	}
}

#ifdef HAVE_LCMS
void
iog_image_apply_display_profile (IogImage *img, cmsHPROFILE screen)
{
	IogImagePrivate *priv;
	cmsHTRANSFORM transform;
	gint row, width, rows, stride;
	guchar *p;

	g_return_if_fail (img != NULL);

	priv = img->priv;

	if (screen == NULL || priv->profile == NULL) return;

	/* TODO: support other colorspaces than RGB */
	if (cmsGetColorSpace (priv->profile) != cmsSigRgbData ||
	    cmsGetColorSpace (screen) != cmsSigRgbData) {
		iog_debug_message (DEBUG_LCMS, "One or both ICC profiles not in RGB colorspace; not correcting");
		return;
	}

	/* TODO: find the right way to colorcorrect RGBA images */
	if (gdk_pixbuf_get_has_alpha (priv->image)) {
		iog_debug_message (DEBUG_LCMS, "Colorcorrecting RGBA images is unsupported.");
		return;
	}

	transform = cmsCreateTransform (priv->profile,
				        TYPE_RGB_8,
				        screen,
				        TYPE_RGB_8,
				        INTENT_PERCEPTUAL,
				        0);

	if (G_LIKELY (transform != NULL)) {
		rows = gdk_pixbuf_get_height (priv->image);
		width = gdk_pixbuf_get_width (priv->image);
		stride = gdk_pixbuf_get_rowstride (priv->image);
		p = gdk_pixbuf_get_pixels (priv->image);

		for (row = 0; row < rows; ++row) {
			cmsDoTransform (transform, p, p, width);
			p += stride;
		}
		cmsDeleteTransform (transform);
	}
}

static void
iog_image_set_icc_data (IogImage *img, IogMetadataReader *md_reader)
{
	IogImagePrivate *priv = img->priv;

	priv->profile = iog_metadata_reader_get_icc_profile (md_reader);


}
#endif

#ifdef HAVE_EXIF
static void
iog_image_set_orientation (IogImage *img)
{
	IogImagePrivate *priv;
	ExifData* exif;

	g_return_if_fail (IOG_IS_IMAGE (img));

	priv = img->priv;

	exif = (ExifData*) iog_image_get_exif_info (img);

	if (exif != NULL) {
		ExifByteOrder o = exif_data_get_byte_order (exif);

		ExifEntry *entry = exif_data_get_entry (exif,
							EXIF_TAG_ORIENTATION);

		if (entry && entry->data != NULL) {
			priv->orientation = exif_get_short (entry->data, o);
		}
	}

	/* exif_data_unref handles NULL values like g_free */
	exif_data_unref (exif);

	if (priv->orientation > 4 &&
	    priv->orientation < 9) {
		gint tmp;

		tmp = priv->width;
		priv->width = priv->height;
		priv->height = tmp;
	}
}

static void
iog_image_real_autorotate (IogImage *img)
{
	static const IogTransformType lookup[8] = {IOG_TRANSFORM_NONE,
					     IOG_TRANSFORM_FLIP_HORIZONTAL,
					     IOG_TRANSFORM_ROT_180,
					     IOG_TRANSFORM_FLIP_VERTICAL,
					     IOG_TRANSFORM_TRANSPOSE,
					     IOG_TRANSFORM_ROT_90,
					     IOG_TRANSFORM_TRANSVERSE,
					     IOG_TRANSFORM_ROT_270};
	IogImagePrivate *priv;
	IogTransformType type;

	g_return_if_fail (IOG_IS_IMAGE (img));

	priv = img->priv;

	type = (priv->orientation >= 1 && priv->orientation <= 8 ?
		lookup[priv->orientation - 1] : IOG_TRANSFORM_NONE);

	if (type != IOG_TRANSFORM_NONE) {
		img->priv->trans_autorotate = iog_transform_new (type);
	}

	/* Disable auto orientation for next loads */
	priv->autorotate = FALSE;
}

void
iog_image_autorotate (IogImage *img)
{
	g_return_if_fail (IOG_IS_IMAGE (img));

	/* Schedule auto orientation */
	img->priv->autorotate = TRUE;
}
#endif

#ifdef HAVE_EXEMPI
static void
iog_image_set_xmp_data (IogImage *img, IogMetadataReader *md_reader)
{
	IogImagePrivate *priv;

	g_return_if_fail (IOG_IS_IMAGE (img));

	priv = img->priv;

	if (priv->xmp) {
		xmp_free (priv->xmp);
	}
	priv->xmp = iog_metadata_reader_get_xmp_data (md_reader);
}
#endif

static void
iog_image_set_exif_data (IogImage *img, IogMetadataReader *md_reader)
{
	IogImagePrivate *priv;

	g_return_if_fail (IOG_IS_IMAGE (img));

	priv = img->priv;

#ifdef HAVE_EXIF
	g_mutex_lock (&priv->status_mutex);
	if (priv->exif) {
		exif_data_unref (priv->exif);
	}
	priv->exif = iog_metadata_reader_get_exif_data (md_reader);
	g_mutex_unlock (&priv->status_mutex);

	priv->exif_chunk = NULL;
	priv->exif_chunk_len = 0;

	/* EXIF data is already available, set the image orientation */
	if (priv->autorotate) {
		iog_image_set_orientation (img);

		/* Emit size prepared signal if we have the size */
		if (priv->width > 0 &&
		    priv->height > 0) {
			iog_image_emit_size_prepared (img);
		}
	}
#else
	if (priv->exif_chunk) {
		g_free (priv->exif_chunk);
	}
	iog_metadata_reader_get_exif_chunk (md_reader,
					    &priv->exif_chunk,
					    &priv->exif_chunk_len);
#endif
}

/*
 * Attempts to get the image dimensions from the thumbnail.
 * Returns FALSE if this information is not found.
 **/
static gboolean
iog_image_get_dimension_from_thumbnail (IogImage *image,
			                gint     *width,
			                gint     *height)
{
	if (image->priv->thumbnail == NULL)
		return FALSE;

	*width = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (image->priv->thumbnail),
						     IOG_THUMBNAIL_ORIGINAL_WIDTH));
	*height = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (image->priv->thumbnail),
						      IOG_THUMBNAIL_ORIGINAL_HEIGHT));

	return (*width || *height);
}

static gboolean
iog_image_real_load (IogImage *img,
		     guint     data2read,
		     IogJob   *job,
		     GError  **error)
{
	IogImagePrivate *priv;
	GFileInputStream *input_stream;
	IogMetadataReader *md_reader = NULL;
	GdkPixbufFormat *format;
	gchar *mime_type;
	GdkPixbufLoader *loader = NULL;
	guchar *buffer;
	goffset bytes_read, bytes_read_total = 0;
	gboolean failed = FALSE;
	gboolean first_run = TRUE;
	gboolean set_metadata = TRUE;
	gboolean read_image_data = (data2read & IOG_IMAGE_DATA_IMAGE);
	gboolean read_only_dimension = (data2read & IOG_IMAGE_DATA_DIMENSION) &&
				  ((data2read ^ IOG_IMAGE_DATA_DIMENSION) == 0);


	priv = img->priv;

 	g_assert (!read_image_data || priv->image == NULL);

	if (read_image_data && priv->file_type != NULL) {
		g_free (priv->file_type);
		priv->file_type = NULL;
	}

	iog_image_get_file_info (img, &priv->bytes, &mime_type, error);

	if (error && *error) {
		g_free (mime_type);
		return FALSE;
	}

	if (read_only_dimension) {
		gint width, height;
		gboolean done;

		done = iog_image_get_dimension_from_thumbnail (img,
							       &width,
							       &height);

		if (done) {
			priv->width = width;
			priv->height = height;

			g_free (mime_type);
			return TRUE;
		}
	}

	input_stream = g_file_read (priv->file, NULL, error);

	if (input_stream == NULL) {
		g_free (mime_type);

		if (error != NULL) {
			g_clear_error (error);
			g_set_error (error,
				     IOG_IMAGE_ERROR,
				     IOG_IMAGE_ERROR_VFS,
				     "Failed to open input stream for file");
		}
		return FALSE;
	}

	buffer = g_new0 (guchar, IOG_IMAGE_READ_BUFFER_SIZE);

	if (read_image_data || read_only_dimension) {
#ifdef HAVE_RSVG
		if (priv->svg != NULL) {
			g_object_unref (priv->svg);
			priv->svg = NULL;
		}

		if (!strcmp (mime_type, "image/svg+xml")) {
			gchar *file_path;
			/* Keep the object for rendering */
			priv->svg = rsvg_handle_new ();
			file_path = g_file_get_path (priv->file);
			rsvg_handle_set_base_uri (priv->svg, file_path);
			g_free (file_path);
		}
#endif
		loader = gdk_pixbuf_loader_new_with_mime_type (mime_type, error);

		if (error && *error) {
			g_error_free (*error);
			*error = NULL;

			loader = gdk_pixbuf_loader_new ();
		}

		g_signal_connect_object (G_OBJECT (loader),
					 "size-prepared",
					 G_CALLBACK (iog_image_size_prepared),
					 img,
					 0);
        }
	g_free (mime_type);

	while (!priv->cancel_loading) {
		/* FIXME: make this async */
		bytes_read = g_input_stream_read (G_INPUT_STREAM (input_stream),
						  buffer,
						  IOG_IMAGE_READ_BUFFER_SIZE,
						  NULL, error);

		if (bytes_read == 0) {
			/* End of the file */
			break;
		} else if (bytes_read == -1) {
			failed = TRUE;

			g_set_error (error,
				     IOG_IMAGE_ERROR,
				     IOG_IMAGE_ERROR_VFS,
				     "Failed to read from input stream");

			break;
		}

		if ((read_image_data || read_only_dimension)) {
			if (!gdk_pixbuf_loader_write (loader, buffer, bytes_read, error)) {
				failed = TRUE;
				break;
			}
#ifdef HAVE_RSVG
			if (iog_image_is_svg (img) &&
			    !rsvg_handle_write (priv->svg, buffer, bytes_read, error)) {
				failed = TRUE;
				break;
			}
#endif
		}

		bytes_read_total += bytes_read;

		if (job != NULL) {
			float progress = (float) bytes_read_total / (float) priv->bytes;
			iog_job_set_progress (job, progress);
		}

		if (first_run) {
			md_reader = check_for_metadata_img_format (img, buffer, bytes_read);

			if (md_reader == NULL) {
				if (data2read == IOG_IMAGE_DATA_EXIF) {
					g_set_error (error,
						     IOG_IMAGE_ERROR,
						     IOG_IMAGE_ERROR_GENERIC,
						     _("EXIF not supported for this file format."));
					break;
				}

				iog_image_emit_size_prepared (img);

				priv->metadata_status = IOG_IMAGE_METADATA_NOT_AVAILABLE;
			}

			first_run = FALSE;
		}

		if (md_reader != NULL) {
			iog_metadata_reader_consume (md_reader, buffer, bytes_read);

			if (iog_metadata_reader_finished (md_reader)) {
				if (set_metadata) {
					iog_image_set_exif_data (img, md_reader);

#ifdef HAVE_LCMS
					iog_image_set_icc_data (img, md_reader);
#endif

#ifdef HAVE_EXEMPI
					iog_image_set_xmp_data (img, md_reader);
#endif
					set_metadata = FALSE;
                                        priv->metadata_status = IOG_IMAGE_METADATA_READY;
				}

				if (data2read == IOG_IMAGE_DATA_EXIF)
					break;
			}
		}

		if (read_only_dimension &&
		    iog_image_has_data (img, IOG_IMAGE_DATA_DIMENSION)) {
			break;
		}
	}

	if (read_image_data || read_only_dimension) {
		if (failed) {
			gdk_pixbuf_loader_close (loader, NULL);
		} else if (!gdk_pixbuf_loader_close (loader, error)) {
			if (gdk_pixbuf_loader_get_pixbuf (loader) != NULL) {
				/* Clear error in order to support partial
				 * images as well. */
				g_clear_error (error);
			}
	        }
#ifdef HAVE_RSVG
		if (iog_image_is_svg (img))
			/* Ignore the error if loading failed earlier
			 * as the error will already be set in that case */
			rsvg_handle_close (priv->svg,
			                   (failed ? NULL : error));
#endif
        }

	g_free (buffer);

	g_object_unref (G_OBJECT (input_stream));

	failed = (failed ||
		  priv->cancel_loading ||
		  bytes_read_total == 0 ||
		  (error && *error != NULL));

	if (failed) {
		if (priv->cancel_loading) {
			priv->cancel_loading = FALSE;
			priv->status = IOG_IMAGE_STATUS_UNKNOWN;
		} else {
			priv->status = IOG_IMAGE_STATUS_FAILED;
		}
	} else if (read_image_data) {
		if (priv->image != NULL) {
			g_object_unref (priv->image);
		}

		priv->anim = gdk_pixbuf_loader_get_animation (loader);

		if (gdk_pixbuf_animation_is_static_image (priv->anim)) {
			priv->image = gdk_pixbuf_animation_get_static_image (priv->anim);
			priv->anim = NULL;
		} else {
			priv->anim_iter = gdk_pixbuf_animation_get_iter (priv->anim,NULL);
			priv->image = gdk_pixbuf_animation_iter_get_pixbuf (priv->anim_iter);
		}

		if (G_LIKELY (priv->image != NULL)) {
			g_object_ref (priv->image);

			priv->width = gdk_pixbuf_get_width (priv->image);
			priv->height = gdk_pixbuf_get_height (priv->image);

			format = gdk_pixbuf_loader_get_format (loader);

			if (format != NULL) {
				priv->file_type = gdk_pixbuf_format_get_name (format);
			}

			priv->file_is_changed = FALSE;
		} else {
			/* Some loaders don't report errors correctly.
			 * Error will be set below. */
			failed = TRUE;
			priv->status = IOG_IMAGE_STATUS_FAILED;
		}
	}

	if (loader != NULL) {
		g_object_unref (loader);
	}

	if (md_reader != NULL) {
		g_object_unref (md_reader);
		md_reader = NULL;
	}

	/* Catch-all in case of poor-error reporting */
	if (failed && error && *error == NULL) {
		g_set_error (error,
			     IOG_IMAGE_ERROR,
			     IOG_IMAGE_ERROR_GENERIC,
			     _("Image loading failed."));
	}

	return !failed;
}

gboolean
iog_image_has_data (IogImage *img, IogImageData req_data)
{
	IogImagePrivate *priv;
	gboolean has_data = TRUE;

	g_return_val_if_fail (IOG_IS_IMAGE (img), FALSE);

	priv = img->priv;

	if ((req_data & IOG_IMAGE_DATA_IMAGE) > 0) {
		req_data = (req_data & ~IOG_IMAGE_DATA_IMAGE);
		has_data = has_data && (priv->image != NULL);
	}

	if ((req_data & IOG_IMAGE_DATA_DIMENSION) > 0 ) {
		req_data = (req_data & ~IOG_IMAGE_DATA_DIMENSION);
		has_data = has_data && (priv->width >= 0) && (priv->height >= 0);
	}

	if ((req_data & IOG_IMAGE_DATA_EXIF) > 0) {
		req_data = (req_data & ~IOG_IMAGE_DATA_EXIF);
#ifdef HAVE_EXIF
		has_data = has_data && (priv->exif != NULL);
#else
		has_data = has_data && (priv->exif_chunk != NULL);
#endif
	}

	if ((req_data & IOG_IMAGE_DATA_XMP) > 0) {
		req_data = (req_data & ~IOG_IMAGE_DATA_XMP);
#ifdef HAVE_EXEMPI
		has_data = has_data && (priv->xmp != NULL);
#endif
	}

	if (req_data != 0) {
		g_warning ("Asking for unknown data, remaining: %i\n", req_data);
		has_data = FALSE;
	}

	return has_data;
}

gboolean
iog_image_load (IogImage *img, IogImageData data2read, IogJob *job, GError **error)
{
	IogImagePrivate *priv;
	gboolean success = FALSE;

	iog_debug (DEBUG_IMAGE_LOAD);

	g_return_val_if_fail (IOG_IS_IMAGE (img), FALSE);

	priv = IOG_IMAGE (img)->priv;

	if (data2read == 0) {
		return TRUE;
	}

	if (iog_image_has_data (img, data2read)) {
		return TRUE;
	}

	priv->status = IOG_IMAGE_STATUS_LOADING;

	success = iog_image_real_load (img, data2read, job, error);

#ifdef HAVE_EXIF
	/* Check that the metadata was loaded at least once before
	 * trying to autorotate. Also only an imatge load job should try to
	 * autorotate and image */
	if (priv->autorotate && 
	    priv->metadata_status == IOG_IMAGE_METADATA_READY &&
	      data2read & IOG_IMAGE_DATA_IMAGE) {
		iog_image_real_autorotate (img);
	}
#endif

	if (success && iog_image_needs_transformation (img)) {
		success = iog_image_apply_transformations (img, error);
	}

	if (success) {
		priv->status = IOG_IMAGE_STATUS_LOADED;
	} else {
		priv->status = IOG_IMAGE_STATUS_FAILED;
	}

	return success;
}

void
iog_image_set_thumbnail (IogImage *img, GdkPixbuf *thumbnail)
{
	IogImagePrivate *priv;

	g_return_if_fail (IOG_IS_IMAGE (img));
	g_return_if_fail (GDK_IS_PIXBUF (thumbnail) || thumbnail == NULL);

	priv = img->priv;

	if (priv->thumbnail != NULL) {
		g_object_unref (priv->thumbnail);
		priv->thumbnail = NULL;
	}

	if (thumbnail != NULL && priv->trans != NULL) {
		priv->thumbnail = iog_transform_apply (priv->trans, thumbnail, NULL);
	} else {
		priv->thumbnail = thumbnail;

		if (thumbnail != NULL) {
			g_object_ref (priv->thumbnail);
		}
	}

	if (priv->thumbnail != NULL) {
		g_signal_emit (img, signals[SIGNAL_THUMBNAIL_CHANGED], 0);
	}
}

/**
 * iog_image_get_pixbuf:
 * @img: a #IogImage
 *
 * Gets the #GdkPixbuf of the image
 *
 * Returns: (transfer full): a #GdkPixbuf
 **/
GdkPixbuf *
iog_image_get_pixbuf (IogImage *img)
{
	GdkPixbuf *image = NULL;

	g_return_val_if_fail (IOG_IS_IMAGE (img), NULL);

	g_mutex_lock (&img->priv->status_mutex);
	image = img->priv->image;
	g_mutex_unlock (&img->priv->status_mutex);

	if (image != NULL) {
		g_object_ref (image);
	}

	return image;
}

#ifdef HAVE_LCMS
cmsHPROFILE
iog_image_get_profile (IogImage *img)
{
	g_return_val_if_fail (IOG_IS_IMAGE (img), NULL);

	return img->priv->profile;
}
#endif

/**
 * iog_image_get_thumbnail:
 * @img: a #IogImage
 *
 * Gets the thumbnail pixbuf for @img
 *
 * Returns: (transfer full): a #GdkPixbuf with a thumbnail
 **/
GdkPixbuf *
iog_image_get_thumbnail (IogImage *img)
{
	g_return_val_if_fail (IOG_IS_IMAGE (img), NULL);

	if (img->priv->thumbnail != NULL) {
		return g_object_ref (img->priv->thumbnail);
	}

	return NULL;
}

void
iog_image_get_size (IogImage *img, int *width, int *height)
{
	IogImagePrivate *priv;

	g_return_if_fail (IOG_IS_IMAGE (img));

	priv = img->priv;

	*width = priv->width;
	*height = priv->height;
}

void
iog_image_transform (IogImage *img, IogTransform *trans, IogJob *job)
{
	iog_image_real_transform (img, trans, FALSE, job);
}

void
iog_image_undo (IogImage *img)
{
	IogImagePrivate *priv;
	IogTransform *trans;
	IogTransform *inverse;

	g_return_if_fail (IOG_IS_IMAGE (img));

	priv = img->priv;

	if (priv->undo_stack != NULL) {
		trans = IOG_TRANSFORM (priv->undo_stack->data);

		inverse = iog_transform_reverse (trans);

		iog_image_real_transform (img, inverse, TRUE, NULL);

		priv->undo_stack = g_slist_delete_link (priv->undo_stack, priv->undo_stack);

		g_object_unref (trans);
		g_object_unref (inverse);

		if (iog_transform_is_identity (priv->trans)) {
			g_object_unref (priv->trans);
			priv->trans = NULL;
		}
	}

	priv->modified = (priv->undo_stack != NULL);
}

static GFile *
tmp_file_get (void)
{
	GFile *tmp_file;
	char *tmp_file_path;
	gint fd;

	tmp_file_path = g_build_filename (g_get_tmp_dir (), "iog-save-XXXXXX", NULL);
	fd = g_mkstemp (tmp_file_path);
	if (fd == -1) {
		g_free (tmp_file_path);
		return NULL;
	}
	else {
		tmp_file = g_file_new_for_path (tmp_file_path);
		g_free (tmp_file_path);
		return tmp_file;
	}
}

static void
transfer_progress_cb (goffset cur_bytes,
		      goffset total_bytes,
		      gpointer user_data)
{
	IogImage *image = IOG_IMAGE (user_data);

	if (cur_bytes > 0) {
		g_signal_emit (G_OBJECT(image),
			       signals[SIGNAL_SAVE_PROGRESS],
			       0,
			       (gfloat) cur_bytes / (gfloat) total_bytes);
	}
}

static void
tmp_file_restore_unix_attributes (GFile *temp_file,
				  GFile *target_file)
{
	GFileInfo *file_info;
	guint      uid;
	guint      gid;
	guint      mode;
	guint      mode_mask = 00600;

	GError    *error = NULL;

	g_return_if_fail (G_IS_FILE (temp_file));
	g_return_if_fail (G_IS_FILE (target_file));

	/* check if file exists */
	if (!g_file_query_exists (target_file, NULL)) {
		iog_debug_message (DEBUG_IMAGE_SAVE,
				   "Target file doesn't exist. Setting default attributes.");
		return;
	}

	/* retrieve UID, GID, and MODE of the original file info */
	file_info = g_file_query_info (target_file,
				       "unix::uid,unix::gid,unix::mode",
				       G_FILE_QUERY_INFO_NONE,
				       NULL,
				       &error);

	/* check that there aren't any error */
	if (error != NULL) {
		iog_debug_message (DEBUG_IMAGE_SAVE,
				   "File information not available. Setting default attributes.");

		/* free objects */
		g_object_unref (file_info);
		g_clear_error (&error);

		return;
	}

	/* save UID, GID and MODE values */
	uid = g_file_info_get_attribute_uint32 (file_info,
						G_FILE_ATTRIBUTE_UNIX_UID);

	gid = g_file_info_get_attribute_uint32 (file_info,
						G_FILE_ATTRIBUTE_UNIX_GID);

	mode = g_file_info_get_attribute_uint32 (file_info,
						 G_FILE_ATTRIBUTE_UNIX_MODE);

	/* apply default mode mask to file mode */
	mode |= mode_mask;

	/* restore original UID, GID, and MODE into the temporal file */
	g_file_set_attribute_uint32 (temp_file,
				     G_FILE_ATTRIBUTE_UNIX_UID,
				     uid,
				     G_FILE_QUERY_INFO_NONE,
				     NULL,
				     &error);

	/* check that there aren't any error */
	if (error != NULL) {
		iog_debug_message (DEBUG_IMAGE_SAVE,
				   "You do not have the permissions necessary to change the file UID.");

		g_clear_error (&error);
	}

	g_file_set_attribute_uint32 (temp_file,
				     G_FILE_ATTRIBUTE_UNIX_GID,
				     gid,
				     G_FILE_QUERY_INFO_NONE,
				     NULL,
				     &error);

	/* check that there aren't any error */
	if (error != NULL) {
		iog_debug_message (DEBUG_IMAGE_SAVE,
				   "You do not have the permissions necessary to change the file GID. Setting user default GID.");

		g_clear_error (&error);
	}

	g_file_set_attribute_uint32 (temp_file,
				     G_FILE_ATTRIBUTE_UNIX_MODE,
				     mode,
				     G_FILE_QUERY_INFO_NONE,
				     NULL,
				     &error);

	/* check that there aren't any error */
	if (error != NULL) {
		iog_debug_message (DEBUG_IMAGE_SAVE,
				   "You do not have the permissions necessary to change the file MODE.");

		g_clear_error (&error);
	}

	/* free objects */
	g_object_unref (file_info);
}

static gboolean
tmp_file_move_to_uri (IogImage *image,
		      GFile *tmpfile,
		      GFile *file,
		      gboolean overwrite,
		      GError **error)
{
	gboolean result;
	GError *ioerror = NULL;

	/* try to restore target file unix attributes */
	tmp_file_restore_unix_attributes (tmpfile, file);

	/* replace target file with temporal file */
	result = g_file_move (tmpfile,
			      file,
			      (overwrite ? G_FILE_COPY_OVERWRITE : 0) |
			      G_FILE_COPY_ALL_METADATA,
			      NULL,
			      (GFileProgressCallback) transfer_progress_cb,
			      image,
			      &ioerror);

	if (result == FALSE) {
		if (g_error_matches (ioerror, G_IO_ERROR,
				     G_IO_ERROR_EXISTS)) {
			g_set_error (error, IOG_IMAGE_ERROR,
				     IOG_IMAGE_ERROR_FILE_EXISTS,
				     "File exists");
		} else {
			g_set_error (error, IOG_IMAGE_ERROR,
				     IOG_IMAGE_ERROR_VFS,
				     "VFS error moving the temp file");
		}
		g_clear_error (&ioerror);
	}

	return result;
}

static gboolean
tmp_file_delete (GFile *tmpfile)
{
	gboolean result;
	GError *err = NULL;

	if (tmpfile == NULL) return FALSE;

	result = g_file_delete (tmpfile, NULL, &err);
	if (result == FALSE) {
		char *tmpfile_path;
		if (err != NULL) {
			if (err->code == G_IO_ERROR_NOT_FOUND) {
				g_error_free (err);
				return TRUE;
			}
			g_error_free (err);
		}
		tmpfile_path = g_file_get_path (tmpfile);
		g_warning ("Couldn't delete temporary file: %s", tmpfile_path);
		g_free (tmpfile_path);
	}

	return result;
}

static void
iog_image_reset_modifications (IogImage *image)
{
	IogImagePrivate *priv;

	g_return_if_fail (IOG_IS_IMAGE (image));

	priv = image->priv;

	g_slist_foreach (priv->undo_stack, (GFunc) g_object_unref, NULL);
	g_slist_free (priv->undo_stack);
	priv->undo_stack = NULL;

	if (priv->trans != NULL) {
		g_object_unref (priv->trans);
		priv->trans = NULL;
	}

	if (priv->trans_autorotate != NULL) {
		g_object_unref (priv->trans_autorotate);
		priv->trans_autorotate = NULL;
	}

	priv->modified = FALSE;
}

static void
iog_image_link_with_target (IogImage *image, IogImageSaveInfo *target)
{
	IogImagePrivate *priv;

	g_return_if_fail (IOG_IS_IMAGE (image));
	g_return_if_fail (IOG_IS_IMAGE_SAVE_INFO (target));

	priv = image->priv;

	/* update file location */
	if (priv->file != NULL) {
		g_object_unref (priv->file);
	}
	priv->file = g_object_ref (target->file);

	/* Clear caption and caption key, these will be
	 * updated on next iog_image_get_caption call.
	 */
	if (priv->caption != NULL) {
		g_free (priv->caption);
		priv->caption = NULL;
	}
	if (priv->collate_key != NULL) {
		g_free (priv->collate_key);
		priv->collate_key = NULL;
	}

	/* update file format */
	if (priv->file_type != NULL) {
		g_free (priv->file_type);
	}
	priv->file_type = g_strdup (target->format);
}

gboolean
iog_image_save_by_info (IogImage *img, IogImageSaveInfo *source, GError **error)
{
	IogImagePrivate *priv;
	IogImageStatus prev_status;
	gboolean success = FALSE;
	GFile *tmp_file;
	char *tmp_file_path;

	g_return_val_if_fail (IOG_IS_IMAGE (img), FALSE);
	g_return_val_if_fail (IOG_IS_IMAGE_SAVE_INFO (source), FALSE);

	priv = img->priv;

	prev_status = priv->status;

	/* Image is now being saved */
	priv->status = IOG_IMAGE_STATUS_SAVING;

	/* see if we need any saving at all */
	if (source->exists && !source->modified) {
		return TRUE;
	}

	/* fail if there is no image to save */
	if (priv->image == NULL) {
		g_set_error (error, IOG_IMAGE_ERROR,
			     IOG_IMAGE_ERROR_NOT_LOADED,
			     _("No image loaded."));
		return FALSE;
	}

	/* generate temporary file */
	tmp_file = tmp_file_get ();

	if (tmp_file == NULL) {
		g_set_error (error, IOG_IMAGE_ERROR,
			     IOG_IMAGE_ERROR_TMP_FILE_FAILED,
			     _("Temporary file creation failed."));
		return FALSE;
	}

	tmp_file_path = g_file_get_path (tmp_file);

#ifdef HAVE_JPEG
	/* determine kind of saving */
	if ((g_ascii_strcasecmp (source->format, IOG_FILE_FORMAT_JPEG) == 0) &&
	    source->exists && source->modified)
	{
		success = iog_image_jpeg_save_file (img, tmp_file_path, source, NULL, error);
	}
#endif

	if (!success && (*error == NULL)) {
		success = gdk_pixbuf_save (priv->image, tmp_file_path, source->format, error, NULL);
	}

	if (success) {
		/* try to move result file to target uri */
		success = tmp_file_move_to_uri (img, tmp_file, priv->file, TRUE /*overwrite*/, error);
	}

	if (success) {
		iog_image_reset_modifications (img);
	}

	tmp_file_delete (tmp_file);

	g_free (tmp_file_path);
	g_object_unref (tmp_file);

	priv->status = prev_status;

	return success;
}

static gboolean
iog_image_copy_file (IogImage *image, IogImageSaveInfo *source, IogImageSaveInfo *target, GError **error)
{
	gboolean result;
	GError *ioerror = NULL;

	g_return_val_if_fail (IOG_IS_IMAGE_SAVE_INFO (source), FALSE);
	g_return_val_if_fail (IOG_IS_IMAGE_SAVE_INFO (target), FALSE);

	result = g_file_copy (source->file,
			      target->file,
			      (target->overwrite ? G_FILE_COPY_OVERWRITE : 0) |
			      G_FILE_COPY_ALL_METADATA,
			      NULL,
			      IOG_IS_IMAGE (image) ? transfer_progress_cb :NULL,
			      image,
			      &ioerror);

	if (result == FALSE) {
		if (ioerror->code == G_IO_ERROR_EXISTS) {
			g_set_error (error, IOG_IMAGE_ERROR,
				     IOG_IMAGE_ERROR_FILE_EXISTS,
				     "%s", ioerror->message);
		} else {
		g_set_error (error, IOG_IMAGE_ERROR,
			     IOG_IMAGE_ERROR_VFS,
			     "%s", ioerror->message);
		}
		g_error_free (ioerror);
	}

	return result;
}

gboolean
iog_image_save_as_by_info (IogImage *img, IogImageSaveInfo *source, IogImageSaveInfo *target, GError **error)
{
	IogImagePrivate *priv;
	gboolean success = FALSE;
	char *tmp_file_path;
	GFile *tmp_file;
	gboolean direct_copy = FALSE;

	g_return_val_if_fail (IOG_IS_IMAGE (img), FALSE);
	g_return_val_if_fail (IOG_IS_IMAGE_SAVE_INFO (source), FALSE);
	g_return_val_if_fail (IOG_IS_IMAGE_SAVE_INFO (target), FALSE);

	priv = img->priv;

	/* fail if there is no image to save */
	if (priv->image == NULL) {
		g_set_error (error,
			     IOG_IMAGE_ERROR,
			     IOG_IMAGE_ERROR_NOT_LOADED,
			     _("No image loaded."));

		return FALSE;
	}

	/* generate temporary file name */
	tmp_file = tmp_file_get ();

	if (tmp_file == NULL) {
		g_set_error (error,
			     IOG_IMAGE_ERROR,
			     IOG_IMAGE_ERROR_TMP_FILE_FAILED,
			     _("Temporary file creation failed."));

		return FALSE;
	}
	tmp_file_path = g_file_get_path (tmp_file);

	/* determine kind of saving */
	if (g_ascii_strcasecmp (source->format, target->format) == 0 && !source->modified) {
		success = iog_image_copy_file (img, source, target, error);
		direct_copy = success;
	}

#ifdef HAVE_JPEG
	else if ((g_ascii_strcasecmp (source->format, IOG_FILE_FORMAT_JPEG) == 0 && source->exists) ||
		 (g_ascii_strcasecmp (target->format, IOG_FILE_FORMAT_JPEG) == 0))
	{
		success = iog_image_jpeg_save_file (img, tmp_file_path, source, target, error);
	}
#endif

	if (!success && (*error == NULL)) {
		success = gdk_pixbuf_save (priv->image, tmp_file_path, target->format, error, NULL);
	}

	if (success && !direct_copy) { /* not required if we alredy copied the file directly */
		/* try to move result file to target uri */
		success = tmp_file_move_to_uri (img, tmp_file, target->file, target->overwrite, error);
	}

	if (success) {
		/* update image information to new uri */
		iog_image_reset_modifications (img);
		iog_image_link_with_target (img, target);
	}

	tmp_file_delete (tmp_file);
	g_object_unref (tmp_file);
	g_free (tmp_file_path);

	priv->status = IOG_IMAGE_STATUS_UNKNOWN;

	return success;
}


/*
 * This function is extracted from
 * File: caja/libcaja-private/caja-file.c
 * Revision: 1.309
 * Author: Darin Adler <darin@bentspoon.com>
 */
static gboolean
have_broken_filenames (void)
{
	static gboolean initialized = FALSE;
	static gboolean broken;

	if (initialized) {
		return broken;
	}

	broken = g_getenv ("G_BROKEN_FILENAMES") != NULL;

	initialized = TRUE;

	return broken;
}

/*
 * This function is inspired by
 * caja/libcaja-private/caja-file.c:caja_file_get_display_name_nocopy
 * Revision: 1.309
 * Author: Darin Adler <darin@bentspoon.com>
 */
const gchar*
iog_image_get_caption (IogImage *img)
{
	IogImagePrivate *priv;
	char *name;
	char *utf8_name;
	char *scheme;
	gboolean validated = FALSE;
	gboolean broken_filenames;

	g_return_val_if_fail (IOG_IS_IMAGE (img), NULL);

	priv = img->priv;

	if (priv->file == NULL) return NULL;

	if (priv->caption != NULL)
		/* Use cached caption string */
		return priv->caption;

	name = g_file_get_basename (priv->file);
	scheme = g_file_get_uri_scheme (priv->file);

	if (name != NULL && g_ascii_strcasecmp (scheme, "file") == 0) {
		/* Support the G_BROKEN_FILENAMES feature of
		 * glib by using g_filename_to_utf8 to convert
		 * local filenames to UTF-8. Also do the same
		 * thing with any local filename that does not
		 * validate as good UTF-8.
		 */
		broken_filenames = have_broken_filenames ();

		if (broken_filenames || !g_utf8_validate (name, -1, NULL)) {
			utf8_name = g_locale_to_utf8 (name, -1, NULL, NULL, NULL);
			if (utf8_name != NULL) {
				g_free (name);
				name = utf8_name;
				/* Guaranteed to be correct utf8 here */
				validated = TRUE;
			}
		} else if (!broken_filenames) {
			/* name was valid, no need to re-validate */
			validated = TRUE;
		}
	}

	if (!validated && !g_utf8_validate (name, -1, NULL)) {
		if (name == NULL) {
			name = g_strdup ("[Invalid Unicode]");
		} else {
			utf8_name = iog_util_make_valid_utf8 (name);
			g_free (name);
			name = utf8_name;
		}
	}

	priv->caption = name;

	if (priv->caption == NULL) {
		char *short_str;

		short_str = g_file_get_basename (priv->file);
		if (g_utf8_validate (short_str, -1, NULL)) {
			priv->caption = g_strdup (short_str);
		} else {
			priv->caption = g_filename_to_utf8 (short_str, -1, NULL, NULL, NULL);
		}
		g_free (short_str);
	}
	g_free (scheme);

	return priv->caption;
}

const gchar*
iog_image_get_collate_key (IogImage *img)
{
	IogImagePrivate *priv;

	g_return_val_if_fail (IOG_IS_IMAGE (img), NULL);

	priv = img->priv;

	if (priv->collate_key == NULL) {
		const char *caption;

		caption = iog_image_get_caption (img);

		priv->collate_key = g_utf8_collate_key_for_filename (caption, -1);
	}

	return priv->collate_key;
}

void
iog_image_cancel_load (IogImage *img)
{
	IogImagePrivate *priv;

	g_return_if_fail (IOG_IS_IMAGE (img));

	priv = img->priv;

	g_mutex_lock (&priv->status_mutex);

	if (priv->status == IOG_IMAGE_STATUS_LOADING) {
		priv->cancel_loading = TRUE;
	}

	g_mutex_unlock (&priv->status_mutex);
}

#ifdef HAVE_EXIF
ExifData *
iog_image_get_exif_info (IogImage *img)
{
	IogImagePrivate *priv;
	ExifData *data = NULL;

	g_return_val_if_fail (IOG_IS_IMAGE (img), NULL);

	priv = img->priv;

	g_mutex_lock (&priv->status_mutex);

	exif_data_ref (priv->exif);
	data = priv->exif;

	g_mutex_unlock (&priv->status_mutex);

	return data;
}
#endif

/**
 * iog_image_get_xmp_info:
 * @img: a #IogImage
 *
 * Gets the XMP info for @img or NULL if compiled without
 * libexempi support.
 *
 * Returns: (transfer full): the xmp data
 **/
gpointer
iog_image_get_xmp_info (IogImage *img)
{
	IogImagePrivate *priv;
 	gpointer data = NULL;

 	g_return_val_if_fail (IOG_IS_IMAGE (img), NULL);

 	priv = img->priv;

#ifdef HAVE_EXEMPI
	g_mutex_lock (&priv->status_mutex);
 	data = (gpointer) xmp_copy (priv->xmp);
	g_mutex_unlock (&priv->status_mutex);
#endif

 	return data;
}


/**
 * iog_image_get_file:
 * @img: a #IogImage
 *
 * Gets the #GFile associated with @img
 *
 * Returns: (transfer full): a #GFile
 **/
GFile *
iog_image_get_file (IogImage *img)
{
	g_return_val_if_fail (IOG_IS_IMAGE (img), NULL);

	return g_object_ref (img->priv->file);
}

gboolean
iog_image_is_modified (IogImage *img)
{
	g_return_val_if_fail (IOG_IS_IMAGE (img), FALSE);

	return img->priv->modified;
}

goffset
iog_image_get_bytes (IogImage *img)
{
	g_return_val_if_fail (IOG_IS_IMAGE (img), 0);

	return img->priv->bytes;
}

void
iog_image_modified (IogImage *img)
{
	g_return_if_fail (IOG_IS_IMAGE (img));

	g_signal_emit (G_OBJECT (img), signals[SIGNAL_CHANGED], 0);
}

gchar*
iog_image_get_uri_for_display (IogImage *img)
{
	IogImagePrivate *priv;
	gchar *uri_str = NULL;
	gchar *str = NULL;

	g_return_val_if_fail (IOG_IS_IMAGE (img), NULL);

	priv = img->priv;

	if (priv->file != NULL) {
		uri_str = g_file_get_uri (priv->file);

		if (uri_str != NULL) {
			str = g_uri_unescape_string (uri_str, NULL);
			g_free (uri_str);
		}
	}

	return str;
}

IogImageStatus
iog_image_get_status (IogImage *img)
{
	g_return_val_if_fail (IOG_IS_IMAGE (img), IOG_IMAGE_STATUS_UNKNOWN);

	return img->priv->status;
}

/**
 * iog_image_get_metadata_status:
 * @img: a #IogImage
 *
 * Returns the current status of the image metadata, that is,
 * whether the metadata has not been read yet, is ready, or not available at all.
 *
 * Returns: one of #IogImageMetadataStatus
 **/
IogImageMetadataStatus
iog_image_get_metadata_status (IogImage *img)
{
        g_return_val_if_fail (IOG_IS_IMAGE (img), IOG_IMAGE_METADATA_NOT_AVAILABLE);

        return img->priv->metadata_status;
}

void
iog_image_data_ref (IogImage *img)
{
	g_return_if_fail (IOG_IS_IMAGE (img));

	g_object_ref (G_OBJECT (img));
	img->priv->data_ref_count++;

	g_assert (img->priv->data_ref_count <= G_OBJECT (img)->ref_count);
}

void
iog_image_data_unref (IogImage *img)
{
	g_return_if_fail (IOG_IS_IMAGE (img));

	if (img->priv->data_ref_count > 0) {
		img->priv->data_ref_count--;
	} else {
		g_warning ("More image data unrefs than refs.");
	}

	if (img->priv->data_ref_count == 0) {
		iog_image_free_mem_private (img);
	}

	g_object_unref (G_OBJECT (img));

	g_assert (img->priv->data_ref_count <= G_OBJECT (img)->ref_count);
}

static gint
compare_quarks (gconstpointer a, gconstpointer b)
{
	GQuark quark;

	quark = g_quark_from_string ((const gchar *) a);

	return quark - GPOINTER_TO_INT (b);
}

/**
 * iog_image_get_supported_mime_types:
 *
 * Gets the list of supported mimetypes
 *
 * Returns: (transfer none)(element-type utf8): a #GList of supported mimetypes
 **/
GList *
iog_image_get_supported_mime_types (void)
{
	GSList *format_list, *it;
	gchar **mime_types;
	int i;

	if (!supported_mime_types) {
		format_list = gdk_pixbuf_get_formats ();

		for (it = format_list; it != NULL; it = it->next) {
			mime_types =
				gdk_pixbuf_format_get_mime_types ((GdkPixbufFormat *) it->data);

			for (i = 0; mime_types[i] != NULL; i++) {
				supported_mime_types =
					g_list_prepend (supported_mime_types,
							g_strdup (mime_types[i]));
			}

			g_strfreev (mime_types);
		}

		supported_mime_types = g_list_sort (supported_mime_types,
						    (GCompareFunc) compare_quarks);

		g_slist_free (format_list);
	}

	return supported_mime_types;
}

gboolean
iog_image_is_supported_mime_type (const char *mime_type)
{
	GList *supported_mime_types, *result;
	GQuark quark;

	if (mime_type == NULL) {
		return FALSE;
	}

	supported_mime_types = iog_image_get_supported_mime_types ();

	quark = g_quark_from_string (mime_type);

	result = g_list_find_custom (supported_mime_types,
				     GINT_TO_POINTER (quark),
				     (GCompareFunc) compare_quarks);

	return (result != NULL);
}

static gboolean
iog_image_iter_advance (IogImage *img)
{
	IogImagePrivate *priv;
 	gboolean new_frame;

	g_return_val_if_fail (IOG_IS_IMAGE (img), FALSE);
	g_return_val_if_fail (GDK_IS_PIXBUF_ANIMATION_ITER (img->priv->anim_iter), FALSE);

	priv = img->priv;

	if ((new_frame = gdk_pixbuf_animation_iter_advance (img->priv->anim_iter, NULL)) == TRUE)
	  {      
		g_mutex_lock (&priv->status_mutex);
		g_object_unref (priv->image);
		priv->image = gdk_pixbuf_animation_iter_get_pixbuf (priv->anim_iter);
	 	g_object_ref (priv->image);
		/* keep the transformation over time */
		if (IOG_IS_TRANSFORM (priv->trans)) {
			GdkPixbuf* transformed = iog_transform_apply (priv->trans, priv->image, NULL);
			g_object_unref (priv->image);
			priv->image = transformed;
			priv->width = gdk_pixbuf_get_width (transformed);
			priv->height = gdk_pixbuf_get_height (transformed);
		}      
		g_mutex_unlock (&priv->status_mutex);
		/* Emit next frame signal so we can update the display */
		g_signal_emit (img, signals[SIGNAL_NEXT_FRAME], 0,
			       gdk_pixbuf_animation_iter_get_delay_time (priv->anim_iter));
	  }

	return new_frame;
}

/**
 * iog_image_is_animation:
 * @img: a #IogImage
 *
 * Checks whether a given image is anigde2d.
 *
 * Returns: #TRUE if it is an anigde2d image, #FALSE otherwise.
 * 
 **/
gboolean
iog_image_is_animation (IogImage *img)
{
	g_return_val_if_fail (IOG_IS_IMAGE (img), FALSE);
	return img->priv->anim != NULL;
}

static gboolean
private_timeout (gpointer data)
{
	IogImage *img = IOG_IMAGE (data);
	IogImagePrivate *priv = img->priv;

	if (iog_image_is_animation (img) && 
	    !g_source_is_destroyed (g_main_current_source ()) &&
	    priv->is_playing) {
		while (iog_image_iter_advance (img) != TRUE) {}; /* cpu-sucking ? */
			g_timeout_add (gdk_pixbuf_animation_iter_get_delay_time (priv->anim_iter), private_timeout, img);
	 		return FALSE;
 	}
	priv->is_playing = FALSE;
	return FALSE; /* stop playing */
}

/**
 * iog_image_start_animation:
 * @img: a #IogImage
 *
 * Starts playing an anigde2d image.
 *
 * Returns: %TRUE on success, %FALSE if @img is already playing or isn't an anigde2d image.
 **/
gboolean
iog_image_start_animation (IogImage *img)
{
	IogImagePrivate *priv;

	g_return_val_if_fail (IOG_IS_IMAGE (img), FALSE);
	priv = img->priv;

	if (!iog_image_is_animation (img) || priv->is_playing)
		return FALSE;

	g_mutex_lock (&priv->status_mutex);
	g_object_ref (priv->anim_iter);
	priv->is_playing = TRUE;
	g_mutex_unlock (&priv->status_mutex);

 	g_timeout_add (gdk_pixbuf_animation_iter_get_delay_time (priv->anim_iter), private_timeout, img);

	return TRUE;
}

#ifdef HAVE_RSVG
gboolean
iog_image_is_svg (IogImage *img)
{
	g_return_val_if_fail (IOG_IS_IMAGE (img), FALSE);

	return (img->priv->svg != NULL);
}

RsvgHandle *
iog_image_get_svg (IogImage *img)
{
	g_return_val_if_fail (IOG_IS_IMAGE (img), NULL);

	return img->priv->svg;
}

IogTransform *
iog_image_get_transform (IogImage *img)
{
	g_return_val_if_fail (IOG_IS_IMAGE (img), NULL);

	return img->priv->trans;
}

#endif

/**
 * iog_image_file_changed:
 * @img: a #IogImage
 *
 * Marks the image files contents as changed. Also, emits
 * IogImage::file-changed signal
 **/
void
iog_image_file_changed (IogImage *img)
{
	g_return_if_fail (IOG_IS_IMAGE (img));

	img->priv->file_is_changed = TRUE;
	g_signal_emit (img, signals[SIGNAL_FILE_CHANGED], 0);
}

gboolean
iog_image_is_file_changed (IogImage *img)
{
	g_return_val_if_fail (IOG_IS_IMAGE (img), TRUE);

	return img->priv->file_is_changed;
}
