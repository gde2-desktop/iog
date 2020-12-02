/*
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

#ifndef _IOG_FILE_CHOOSER_H_
#define _IOG_FILE_CHOOSER_H_

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

#define IOG_TYPE_FILE_CHOOSER          (iog_file_chooser_get_type ())
#define IOG_FILE_CHOOSER(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), IOG_TYPE_FILE_CHOOSER, IogFileChooser))
#define IOG_FILE_CHOOSER_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST((k), IOG_TYPE_FILE_CHOOSER, IogFileChooserClass))

#define IOG_IS_FILE_CHOOSER(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), IOG_TYPE_FILE_CHOOSER))
#define IOG_IS_FILE_CHOOSER_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), IOG_TYPE_FILE_CHOOSER))
#define IOG_FILE_CHOOSER_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), IOG_TYPE_FILE_CHOOSER, IogFileChooserClass))

typedef struct _IogFileChooser         IogFileChooser;
typedef struct _IogFileChooserClass    IogFileChooserClass;
typedef struct _IogFileChooserPrivate  IogFileChooserPrivate;

struct _IogFileChooser
{
	GtkFileChooserDialog  parent;

	IogFileChooserPrivate *priv;
};

struct _IogFileChooserClass
{
	GtkFileChooserDialogClass  parent_class;
};


GType		 iog_file_chooser_get_type	(void) G_GNUC_CONST;

GtkWidget	*iog_file_chooser_new		(GtkFileChooserAction action);

GdkPixbufFormat	*iog_file_chooser_get_format	(IogFileChooser *chooser);


G_END_DECLS

#endif /* _IOG_FILE_CHOOSER_H_ */
