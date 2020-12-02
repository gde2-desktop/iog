/* Eye of Gde2 - Statusbar
 *
 * Copyright (C) 2000-2006 The Free Software Foundation
 *
 * Author: Federico Mena-Quintero <federico@gnu.org>
 *	   Jens Finke <jens@gnome.org>
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

#ifndef __IOG_STATUSBAR_H__
#define __IOG_STATUSBAR_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _IogStatusbar        IogStatusbar;
typedef struct _IogStatusbarPrivate IogStatusbarPrivate;
typedef struct _IogStatusbarClass   IogStatusbarClass;

#define IOG_TYPE_STATUSBAR            (iog_statusbar_get_type ())
#define IOG_STATUSBAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), IOG_TYPE_STATUSBAR, IogStatusbar))
#define IOG_STATUSBAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),   IOG_TYPE_STATUSBAR, IogStatusbarClass))
#define IOG_IS_STATUSBAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IOG_TYPE_STATUSBAR))
#define IOG_IS_STATUSBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  IOG_TYPE_STATUSBAR))
#define IOG_STATUSBAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  IOG_TYPE_STATUSBAR, IogStatusbarClass))

struct _IogStatusbar
{
        GtkStatusbar parent;

        IogStatusbarPrivate *priv;
};

struct _IogStatusbarClass
{
        GtkStatusbarClass parent_class;
};

GType		 iog_statusbar_get_type			(void) G_GNUC_CONST;

GtkWidget	*iog_statusbar_new			(void);

void		 iog_statusbar_set_image_number		(IogStatusbar   *statusbar,
							 gint           num,
							 gint           tot);

void		 iog_statusbar_set_progress		(IogStatusbar   *statusbar,
							 gdouble        progress);

G_END_DECLS

#endif /* __IOG_STATUSBAR_H__ */
