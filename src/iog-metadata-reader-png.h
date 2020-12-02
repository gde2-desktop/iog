/* Eye Of GDE2 -- PNG Metadata Reader
 *
 * Copyright (C) 2008 The Free Software Foundation
 *
 * Author: Felix Riemann <friemann@svn.gnome.org>
 *
 * Based on the old IogMetadataReader code.
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

#ifndef _IOG_METADATA_READER_PNG_H_
#define _IOG_METADATA_READER_PNG_H_

G_BEGIN_DECLS

#define IOG_TYPE_METADATA_READER_PNG		(iog_metadata_reader_png_get_type ())
#define IOG_METADATA_READER_PNG(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), IOG_TYPE_METADATA_READER_PNG, IogMetadataReaderPng))
#define IOG_METADATA_READER_PNG_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), IOG_TYPE_METADATA_READER_PNG, IogMetadataReaderPngClass))
#define IOG_IS_METADATA_READER_PNG(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), IOG_TYPE_METADATA_READER_PNG))
#define IOG_IS_METADATA_READER_PNG_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), IOG_TYPE_METADATA_READER_PNG))
#define IOG_METADATA_READER_PNG_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), IOG_TYPE_METADATA_READER_PNG, IogMetadataReaderPngClass))

typedef struct _IogMetadataReaderPng IogMetadataReaderPng;
typedef struct _IogMetadataReaderPngClass IogMetadataReaderPngClass;
typedef struct _IogMetadataReaderPngPrivate IogMetadataReaderPngPrivate;

struct _IogMetadataReaderPng {
	GObject parent;

	IogMetadataReaderPngPrivate *priv;
};

struct _IogMetadataReaderPngClass {
	GObjectClass parent_klass;
};

G_GNUC_INTERNAL
GType		      iog_metadata_reader_png_get_type	(void) G_GNUC_CONST;

G_END_DECLS

#endif /* _IOG_METADATA_READER_PNG_H_ */
