/*
 * iog-close-confirmation-dialog.h
 * This file is part of iog
 *
 * Author: Marcus Carlson <marcus@mejlamej.nu>
 *
 * Based on gedit code (gedit/gedit-close-confirmation.h) by gedit Team
 *
 * Copyright (C) 2004-2009 GNOME Foundation 
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA 02110-1301, USA. 
 */

#ifndef __IOG_CLOSE_CONFIRMATION_DIALOG_H__
#define __IOG_CLOSE_CONFIRMATION_DIALOG_H__

#include <glib.h>
#include <gtk/gtk.h>

#include <iog-image.h>

#define IOG_TYPE_CLOSE_CONFIRMATION_DIALOG		(iog_close_confirmation_dialog_get_type ())
#define IOG_CLOSE_CONFIRMATION_DIALOG(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), IOG_TYPE_CLOSE_CONFIRMATION_DIALOG, IogCloseConfirmationDialog))
#define IOG_CLOSE_CONFIRMATION_DIALOG_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), IOG_TYPE_CLOSE_CONFIRMATION_DIALOG, IogCloseConfirmationDialogClass))
#define IOG_IS_CLOSE_CONFIRMATION_DIALOG(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), IOG_TYPE_CLOSE_CONFIRMATION_DIALOG))
#define IOG_IS_CLOSE_CONFIRMATION_DIALOG_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), IOG_TYPE_CLOSE_CONFIRMATION_DIALOG))
#define IOG_CLOSE_CONFIRMATION_DIALOG_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj),IOG_TYPE_CLOSE_CONFIRMATION_DIALOG, IogCloseConfirmationDialogClass))

typedef struct _IogCloseConfirmationDialog 		IogCloseConfirmationDialog;
typedef struct _IogCloseConfirmationDialogClass 	IogCloseConfirmationDialogClass;
typedef struct _IogCloseConfirmationDialogPrivate 	IogCloseConfirmationDialogPrivate;

struct _IogCloseConfirmationDialog 
{
	GtkDialog parent;

	/*< private > */
	IogCloseConfirmationDialogPrivate *priv;
};

struct _IogCloseConfirmationDialogClass 
{
	GtkDialogClass parent_class;
};

G_GNUC_INTERNAL
GType 		 iog_close_confirmation_dialog_get_type		(void) G_GNUC_CONST;

G_GNUC_INTERNAL
GtkWidget	*iog_close_confirmation_dialog_new			(GtkWindow     *parent,
									 GList         *unsaved_documents);
G_GNUC_INTERNAL
GtkWidget 	*iog_close_confirmation_dialog_new_single 		(GtkWindow     *parent, 
									 IogImage      *image);

G_GNUC_INTERNAL
const GList	*iog_close_confirmation_dialog_get_unsaved_images	(IogCloseConfirmationDialog *dlg);

G_GNUC_INTERNAL
GList		*iog_close_confirmation_dialog_get_selected_images	(IogCloseConfirmationDialog *dlg);

G_GNUC_INTERNAL
void		 iog_close_confirmation_dialog_set_sensitive		(IogCloseConfirmationDialog *dlg, gboolean value);

#endif /* __IOG_CLOSE_CONFIRMATION_DIALOG_H__ */

