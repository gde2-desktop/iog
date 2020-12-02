/* Eye Of Gde2 - Image Properties Dialog
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

#ifndef __IOG_PROPERTIES_DIALOG_H__
#define __IOG_PROPERTIES_DIALOG_H__

#include "iog-dialog.h"
#include "iog-image.h"
#include "iog-thumb-view.h"

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _IogPropertiesDialog IogPropertiesDialog;
typedef struct _IogPropertiesDialogClass IogPropertiesDialogClass;
typedef struct _IogPropertiesDialogPrivate IogPropertiesDialogPrivate;

#define IOG_TYPE_PROPERTIES_DIALOG            (iog_properties_dialog_get_type ())
#define IOG_PROPERTIES_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), IOG_TYPE_PROPERTIES_DIALOG, IogPropertiesDialog))
#define IOG_PROPERTIES_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  IOG_TYPE_PROPERTIES_DIALOG, IogPropertiesDialogClass))
#define IOG_IS_PROPERTIES_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), IOG_TYPE_PROPERTIES_DIALOG))
#define IOG_IS_PROPERTIES_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  IOG_TYPE_PROPERTIES_DIALOG))
#define IOG_PROPERTIES_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  IOG_TYPE_PROPERTIES_DIALOG, IogPropertiesDialogClass))

typedef enum {
	IOG_PROPERTIES_DIALOG_PAGE_GENERAL = 0,
	IOG_PROPERTIES_DIALOG_PAGE_EXIF,
	IOG_PROPERTIES_DIALOG_PAGE_DETAILS,
	IOG_PROPERTIES_DIALOG_N_PAGES
} IogPropertiesDialogPage;

struct _IogPropertiesDialog {
	IogDialog dialog;

	IogPropertiesDialogPrivate *priv;
};

struct _IogPropertiesDialogClass {
	IogDialogClass parent_class;
};

GType	    iog_properties_dialog_get_type	(void) G_GNUC_CONST;

GObject    *iog_properties_dialog_new	  	(GtkWindow               *parent,
                                                 IogThumbView            *thumbview,
						 GtkAction               *next_image_action,
						 GtkAction               *previous_image_action);

void	    iog_properties_dialog_update  	(IogPropertiesDialog     *prop,
						 IogImage                *image);

void	    iog_properties_dialog_set_page  	(IogPropertiesDialog     *prop,
						 IogPropertiesDialogPage  page);

void	    iog_properties_dialog_set_netbook_mode (IogPropertiesDialog *dlg,
						    gboolean enable);
G_END_DECLS

#endif /* __IOG_PROPERTIES_DIALOG_H__ */
