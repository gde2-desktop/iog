/* Eye Of Gde2 - IOG Preferences Dialog
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

#ifndef __IOG_PREFERENCES_DIALOG_H__
#define __IOG_PREFERENCES_DIALOG_H__

#include "iog-dialog.h"
#include "iog-image.h"
#include "iog-thumb-view.h"

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <gio/gio.h>

G_BEGIN_DECLS

typedef struct _IogPreferencesDialog IogPreferencesDialog;
typedef struct _IogPreferencesDialogClass IogPreferencesDialogClass;
typedef struct _IogPreferencesDialogPrivate IogPreferencesDialogPrivate;

#define IOG_TYPE_PREFERENCES_DIALOG            (iog_preferences_dialog_get_type ())
#define IOG_PREFERENCES_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), IOG_TYPE_PREFERENCES_DIALOG, IogPreferencesDialog))
#define IOG_PREFERENCES_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  IOG_TYPE_PREFERENCES_DIALOG, IogPreferencesDialogClass))
#define IOG_IS_PREFERENCES_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), IOG_TYPE_PREFERENCES_DIALOG))
#define IOG_IS_PREFERENCES_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  IOG_TYPE_PREFERENCES_DIALOG))
#define IOG_PREFERENCES_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  IOG_TYPE_PREFERENCES_DIALOG, IogPreferencesDialogClass))

struct _IogPreferencesDialog {
	IogDialog dialog;

	IogPreferencesDialogPrivate *priv;
};

struct _IogPreferencesDialogClass {
	IogDialogClass parent_class;
};

G_GNUC_INTERNAL
GType	    iog_preferences_dialog_get_type	  (void) G_GNUC_CONST;

G_GNUC_INTERNAL
GObject    *iog_preferences_dialog_get_instance	  (GtkWindow   *parent);

G_END_DECLS

#endif /* __IOG_PREFERENCES_DIALOG_H__ */
