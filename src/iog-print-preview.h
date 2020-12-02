/* Eye of GDE2 -- Print Preview Widget
 *
 * Copyright (C) 2006-2007 The Free Software Foundation
 *
 * Author: Claudio Saavedra <csaavedra@alumnos.utalca.cl>
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

#ifndef _IOG_PRINT_PREVIEW_H_
#define _IOG_PRINT_PREVIEW_H_

G_BEGIN_DECLS

typedef struct _IogPrintPreview IogPrintPreview;
typedef struct _IogPrintPreviewClass IogPrintPreviewClass;
typedef struct _IogPrintPreviewPrivate IogPrintPreviewPrivate;

#define IOG_TYPE_PRINT_PREVIEW            (iog_print_preview_get_type ())
#define IOG_PRINT_PREVIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), IOG_TYPE_PRINT_PREVIEW, IogPrintPreview))
#define IOG_PRINT_PREVIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), IOG_TYPE_PRINT_PREVIEW, IogPrintPreviewClass))
#define IOG_IS_PRINT_PREVIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IOG_TYPE_PRINT_PREVIEW))
#define IOG_IS_PRINT_PREVIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), IOG_TYPE_PRINT_PREVIEW))

struct _IogPrintPreview {
	GtkAspectFrame aspect_frame;

	IogPrintPreviewPrivate *priv;
};

struct _IogPrintPreviewClass {
	GtkAspectFrameClass parent_class;

};

G_GNUC_INTERNAL
GType        iog_print_preview_get_type            (void) G_GNUC_CONST;

G_GNUC_INTERNAL
GtkWidget   *iog_print_preview_new                 (void);

G_GNUC_INTERNAL
GtkWidget   *iog_print_preview_new_with_pixbuf     (GdkPixbuf       *pixbuf);

G_GNUC_INTERNAL
void         iog_print_preview_set_page_margins    (IogPrintPreview *preview,
						    gfloat          l_margin,
						    gfloat          r_margin,
						    gfloat          t_margin,
						    gfloat          b_margin);

G_GNUC_INTERNAL
void         iog_print_preview_set_from_page_setup (IogPrintPreview *preview,
						    GtkPageSetup    *setup);

G_GNUC_INTERNAL
void         iog_print_preview_get_image_position  (IogPrintPreview *preview,
						    gdouble         *x,
						    gdouble         *y);

G_GNUC_INTERNAL
void         iog_print_preview_set_image_position  (IogPrintPreview *preview,
						    gdouble          x,
						    gdouble          y);

G_GNUC_INTERNAL
void         iog_print_preview_set_scale           (IogPrintPreview *preview,
						    gfloat           scale);

G_END_DECLS

#endif /* _IOG_PRINT_PREVIEW_H_ */
