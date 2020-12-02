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

#ifndef _IOG_METADATA_READER_H_
#define _IOG_METADATA_READER_H_

#include <glib-object.h>
#if HAVE_EXIF
#include "iog-exif-util.h"
#endif
#if HAVE_EXEMPI
#include <exempi/xmp.h>
#endif
#if HAVE_LCMS
#include <lcms2.h>
#endif

G_BEGIN_DECLS

#define IOG_TYPE_METADATA_READER	      (iog_metadata_reader_get_type ())
#define IOG_METADATA_READER(o)		      (G_TYPE_CHECK_INSTANCE_CAST ((o), IOG_TYPE_METADATA_READER, IogMetadataReader))
#define IOG_IS_METADATA_READER(o)	      (G_TYPE_CHECK_INSTANCE_TYPE ((o), IOG_TYPE_METADATA_READER))
#define IOG_METADATA_READER_GET_INTERFACE(o)  (G_TYPE_INSTANCE_GET_INTERFACE ((o), IOG_TYPE_METADATA_READER, IogMetadataReaderInterface))

typedef struct _IogMetadataReader IogMetadataReader;
typedef struct _IogMetadataReaderInterface IogMetadataReaderInterface;

struct _IogMetadataReaderInterface {
	GTypeInterface parent;

	void		(*consume)		(IogMetadataReader *self,
						 const guchar *buf,
						 guint len);

	gboolean	(*finished)		(IogMetadataReader *self);

	void		(*get_raw_exif)		(IogMetadataReader *self,
						 guchar **data,
						 guint *len);

	gpointer	(*get_exif_data)	(IogMetadataReader *self);

	gpointer	(*get_icc_profile)	(IogMetadataReader *self);

	gpointer	(*get_xmp_ptr)		(IogMetadataReader *self);
};

typedef enum {
	IOG_METADATA_JPEG,
	IOG_METADATA_PNG
} IogMetadataFileType;

G_GNUC_INTERNAL
GType                iog_metadata_reader_get_type	(void) G_GNUC_CONST;

G_GNUC_INTERNAL
IogMetadataReader*   iog_metadata_reader_new 		(IogMetadataFileType type);

G_GNUC_INTERNAL
void                 iog_metadata_reader_consume	(IogMetadataReader *emr,
							 const guchar *buf,
							 guint len);

G_GNUC_INTERNAL
gboolean             iog_metadata_reader_finished	(IogMetadataReader *emr);

G_GNUC_INTERNAL
void                 iog_metadata_reader_get_exif_chunk (IogMetadataReader *emr,
							 guchar **data,
							 guint *len);

#ifdef HAVE_EXIF
G_GNUC_INTERNAL
ExifData*            iog_metadata_reader_get_exif_data	(IogMetadataReader *emr);
#endif

#ifdef HAVE_EXEMPI
G_GNUC_INTERNAL
XmpPtr	     	     iog_metadata_reader_get_xmp_data	(IogMetadataReader *emr);
#endif

#if 0
gpointer             iog_metadata_reader_get_iptc_chunk	(IogMetadataReader *emr);
IptcData*            iog_metadata_reader_get_iptc_data	(IogMetadataReader *emr);
#endif

#ifdef HAVE_LCMS
G_GNUC_INTERNAL
cmsHPROFILE          iog_metadata_reader_get_icc_profile (IogMetadataReader *emr);
#endif

G_END_DECLS

#endif /* _IOG_METADATA_READER_H_ */
