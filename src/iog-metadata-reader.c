/* Eye Of GDE2 -- Metadata Reader Interface
 *
 * Copyright (C) 2008 The Free Software Foundation
 *
 * Author: Felix Riemann <friemann@svn.gnome.org>
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
#include <config.h>
#endif

#include "iog-metadata-reader.h"
#include "iog-metadata-reader-jpg.h"
#include "iog-metadata-reader-png.h"
#include "iog-debug.h"

G_DEFINE_INTERFACE (IogMetadataReader, iog_metadata_reader, G_TYPE_INVALID)

IogMetadataReader*
iog_metadata_reader_new (IogMetadataFileType type)
{
	IogMetadataReader *emr;

	switch (type) {
	case IOG_METADATA_JPEG:
		emr = IOG_METADATA_READER (g_object_new (IOG_TYPE_METADATA_READER_JPG, NULL));
		break;
	case IOG_METADATA_PNG:
		emr = IOG_METADATA_READER (g_object_new (IOG_TYPE_METADATA_READER_PNG, NULL));
		break;
	default:
		emr = NULL;
		break;
	}

	return emr;
}

gboolean
iog_metadata_reader_finished (IogMetadataReader *emr)
{
	g_return_val_if_fail (IOG_IS_METADATA_READER (emr), TRUE);

	return IOG_METADATA_READER_GET_INTERFACE (emr)->finished (emr);
}


void
iog_metadata_reader_consume (IogMetadataReader *emr, const guchar *buf, guint len)
{
	IOG_METADATA_READER_GET_INTERFACE (emr)->consume (emr, buf, len);
}

/* Returns the raw exif data. NOTE: The caller of this function becomes
 * the new owner of this piece of memory and is responsible for freeing it!
 */
void
iog_metadata_reader_get_exif_chunk (IogMetadataReader *emr, guchar **data, guint *len)
{
	g_return_if_fail (data != NULL && len != NULL);

	IOG_METADATA_READER_GET_INTERFACE (emr)->get_raw_exif (emr, data, len);
}

#ifdef HAVE_EXIF
ExifData*
iog_metadata_reader_get_exif_data (IogMetadataReader *emr)
{
	return IOG_METADATA_READER_GET_INTERFACE (emr)->get_exif_data (emr);
}
#endif

#ifdef HAVE_EXEMPI
XmpPtr
iog_metadata_reader_get_xmp_data (IogMetadataReader *emr)
{
	return IOG_METADATA_READER_GET_INTERFACE (emr)->get_xmp_ptr (emr);
}
#endif

#ifdef HAVE_LCMS
cmsHPROFILE
iog_metadata_reader_get_icc_profile (IogMetadataReader *emr)
{
	return IOG_METADATA_READER_GET_INTERFACE (emr)->get_icc_profile (emr);
}
#endif

/* Default vfunc that simply clears the output if not overriden by the
   implementing class. This mimics the old behavour of get_exif_chunk(). */
static void
_iog_metadata_reader_default_get_raw_exif (IogMetadataReader *emr,
					   guchar **data, guint *length)
{
	g_return_if_fail (data != NULL && length != NULL);
	*data = NULL;
	*length = 0;
}

/* Default vfunc that simply returns NULL if not overriden by the implementing
   class. Mimics the old fallback behaviour of the getter functions. */
static gpointer
_iog_metadata_reader_default_get_null (IogMetadataReader *emr)
{
	return NULL;
}
static void
iog_metadata_reader_default_init (IogMetadataReaderInterface *iface)
{
	/* consume and finished are required to be implemented */
	/* Not-implemented funcs return NULL by default */
	iface->get_raw_exif = _iog_metadata_reader_default_get_raw_exif;
	iface->get_exif_data = _iog_metadata_reader_default_get_null;
	iface->get_icc_profile = _iog_metadata_reader_default_get_null;
	iface->get_xmp_ptr = _iog_metadata_reader_default_get_null;
}
