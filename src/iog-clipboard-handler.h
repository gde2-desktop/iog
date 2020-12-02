/*
 * iog-clipboard-handler.h
 * This file is part of iog
 *
 * Author: Felix Riemann <friemann@gnome.org>
 *
 * Copyright (C) 2010 GNOME Foundation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __IOG_CLIPBOARD_HANDLER_H__
#define __IOG_CLIPBOARD_HANDLER_H__

#include <glib-object.h>
#include <gtk/gtk.h>

#include "iog-image.h"

G_BEGIN_DECLS

#define IOG_TYPE_CLIPBOARD_HANDLER          (iog_clipboard_handler_get_type ())
#define IOG_CLIPBOARD_HANDLER(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), IOG_TYPE_CLIPBOARD_HANDLER, IogClipboardHandler))
#define IOG_CLIPBOARD_HANDLER_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST((k), IOG_TYPE_CLIPBOARD_HANDLER, IogClipboardHandlerClass))
#define IOG_IS_CLIPBOARD_HANDLER(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), IOG_TYPE_CLIPBOARD_HANDLER))
#define IOG_IS_CLIPBOARD_HANDLER_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), IOG_TYPE_CLIPBOARD_HANDLER))
#define IOG_CLIPBOARD_HANDLER_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), IOG_TYPE_CLIPBOARD_HANDLER, IogClipboardHandlerClass))

typedef struct _IogClipboardHandler IogClipboardHandler;
typedef struct _IogClipboardHandlerClass IogClipboardHandlerClass;
typedef struct _IogClipboardHandlerPrivate IogClipboardHandlerPrivate;

struct _IogClipboardHandler {
	GObject parent;

	IogClipboardHandlerPrivate *priv;
};

struct _IogClipboardHandlerClass {
	GObjectClass parent_klass;
};

GType iog_clipboard_handler_get_type (void) G_GNUC_CONST;

IogClipboardHandler* iog_clipboard_handler_new (IogImage *img);

void iog_clipboard_handler_copy_to_clipboard (IogClipboardHandler *handler,
					      GtkClipboard *clipboard);

G_END_DECLS
#endif /* __IOG_CLIPBOARD_HANDLER_H__ */
