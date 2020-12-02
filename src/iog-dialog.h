/* Eye Of Gde2 - IOG Dialog
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

#ifndef __IOG_DIALOG_H__
#define __IOG_DIALOG_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _IogDialog IogDialog;
typedef struct _IogDialogClass IogDialogClass;
typedef struct _IogDialogPrivate IogDialogPrivate;

#define IOG_TYPE_DIALOG            (iog_dialog_get_type ())
#define IOG_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), IOG_TYPE_DIALOG, IogDialog))
#define IOG_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  IOG_TYPE_DIALOG, IogDialogClass))
#define IOG_IS_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), IOG_TYPE_DIALOG))
#define IOG_IS_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  IOG_TYPE_DIALOG))
#define IOG_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  IOG_TYPE_DIALOG, IogDialogClass))

struct _IogDialog {
	GObject object;

	IogDialogPrivate *priv;
};

struct _IogDialogClass {
	GObjectClass parent_class;

	void    (* construct)   (IogDialog   *dialog,
				 const gchar *ui_file,
				 const gchar *dlg_node);

	void    (* show)        (IogDialog   *dialog);

	void    (* hide)        (IogDialog   *dialog);
};

GType   iog_dialog_get_type      (void) G_GNUC_CONST;

void    iog_dialog_construct     (IogDialog   *dialog,
				  const gchar *ui_file,
				  const gchar *dlg_node);

void    iog_dialog_show	         (IogDialog *dialog);

void    iog_dialog_hide	         (IogDialog *dialog);

void    iog_dialog_get_controls  (IogDialog   *dialog,
				  const gchar *property_id,
				  ...);

G_END_DECLS

#endif /* __IOG_DIALOG_H__ */
