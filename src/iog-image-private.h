/* Eye Of Gde2 - Image Private Data
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

#ifndef __IOG_IMAGE_PRIVATE_H__
#define __IOG_IMAGE_PRIVATE_H__

#include "iog-image.h"
#ifdef HAVE_RSVG
#include <librsvg/rsvg.h>
#endif

G_BEGIN_DECLS

struct _IogImagePrivate {
	GFile            *file;

	IogImageStatus    status;
	IogImageStatus    prev_status;
	gboolean          is_monitored;
        IogImageMetadataStatus metadata_status;

	GdkPixbuf        *image;
	GdkPixbufAnimation     *anim;
	GdkPixbufAnimationIter *anim_iter;
	gboolean          is_playing;
	GdkPixbuf        *thumbnail;
#ifdef HAVE_RSVG
	RsvgHandle       *svg;
#endif

	gint              width;
	gint              height;

	goffset           bytes;
	gchar            *file_type;

	/* Holds EXIF raw data */
	guint             exif_chunk_len;
	guchar           *exif_chunk;

	/* Holds IPTC raw data */
	guchar           *iptc_chunk;
	guint             iptc_chunk_len;

	gboolean          modified;
	gboolean          file_is_changed;

#ifdef HAVE_EXIF
	gboolean          autorotate;
	gint              orientation;
	ExifData         *exif;
#endif
#ifdef HAVE_EXEMPI
 	XmpPtr   xmp;
#endif

#ifdef HAVE_LCMS
	cmsHPROFILE       profile;
#endif

	gchar            *caption;

	gchar            *collate_key;

	GMutex           status_mutex;

	gboolean          cancel_loading;
	guint             data_ref_count;

	GSList           *undo_stack;

	IogTransform     *trans;
	IogTransform     *trans_autorotate;
};

G_END_DECLS

#endif /* __IOG_IMAGE_PRIVATE_H__ */
