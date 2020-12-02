/* Eye Of Gde2 - Thumbnail View
 *
 * Copyright (C) 2006 The Free Software Foundation
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

#ifndef IOG_THUMB_VIEW_H
#define IOG_THUMB_VIEW_H

#include "iog-image.h"
#include "iog-list-store.h"

G_BEGIN_DECLS

#define IOG_TYPE_THUMB_VIEW            (iog_thumb_view_get_type ())
#define IOG_THUMB_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), IOG_TYPE_THUMB_VIEW, IogThumbView))
#define IOG_THUMB_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  IOG_TYPE_THUMB_VIEW, IogThumbViewClass))
#define IOG_IS_THUMB_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IOG_TYPE_THUMB_VIEW))
#define IOG_IS_THUMB_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  IOG_TYPE_THUMB_VIEW))
#define IOG_THUMB_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  IOG_TYPE_THUMB_VIEW, IogThumbViewClass))

typedef struct _IogThumbView IogThumbView;
typedef struct _IogThumbViewClass IogThumbViewClass;
typedef struct _IogThumbViewPrivate IogThumbViewPrivate;

typedef enum {
	IOG_THUMB_VIEW_SELECT_CURRENT = 0,
	IOG_THUMB_VIEW_SELECT_LEFT,
	IOG_THUMB_VIEW_SELECT_RIGHT,
	IOG_THUMB_VIEW_SELECT_FIRST,
	IOG_THUMB_VIEW_SELECT_LAST,
	IOG_THUMB_VIEW_SELECT_RANDOM
} IogThumbViewSelectionChange;

struct _IogThumbView {
	GtkIconView icon_view;
	IogThumbViewPrivate *priv;
};

struct _IogThumbViewClass {
	 GtkIconViewClass icon_view_class;
};

GType       iog_thumb_view_get_type 		    (void) G_GNUC_CONST;

GtkWidget  *iog_thumb_view_new 			    (void);

void	    iog_thumb_view_set_model 		    (IogThumbView *thumbview,
						     IogListStore *store);

void        iog_thumb_view_set_item_height          (IogThumbView *thumbview,
						     gint          height);

guint	    iog_thumb_view_get_n_selected 	    (IogThumbView *thumbview);

IogImage   *iog_thumb_view_get_first_selected_image (IogThumbView *thumbview);

GList      *iog_thumb_view_get_selected_images 	    (IogThumbView *thumbview);

void        iog_thumb_view_select_single 	    (IogThumbView *thumbview,
						     IogThumbViewSelectionChange change);

void        iog_thumb_view_set_current_image	    (IogThumbView *thumbview,
						     IogImage     *image,
						     gboolean     deselect_other);

void        iog_thumb_view_set_thumbnail_popup      (IogThumbView *thumbview,
						     GtkMenu      *menu);

G_END_DECLS

#endif /* IOG_THUMB_VIEW_H */
