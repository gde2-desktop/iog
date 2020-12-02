/* Eye Of GDE2 -- Print Dialog Custom Widget
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

#include "iog-image.h"

#ifndef IOG_PRINT_IMAGE_SETUP_H
#define IOG_PRINT_IMAGE_SETUP_H

G_BEGIN_DECLS

typedef struct _IogPrintImageSetup         IogPrintImageSetup;
typedef struct _IogPrintImageSetupClass    IogPrintImageSetupClass;
typedef struct _IogPrintImageSetupPrivate   IogPrintImageSetupPrivate;

#define IOG_TYPE_PRINT_IMAGE_SETUP            (iog_print_image_setup_get_type ())
#define IOG_PRINT_IMAGE_SETUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), IOG_TYPE_PRINT_IMAGE_SETUP, IogPrintImageSetup))
#define IOG_PRINT_IMAGE_SETUP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), IOG_TYPE_PRINT_IMAGE_SETUP, IogPrintImageSetupClass))
#define IOG_IS_PRINT_IMAGE_SETUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IOG_TYPE_PRINT_IMAGE_SETUP))
#define IOG_IS_PRINT_IMAGE_SETUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), IOG_TYPE_PRINT_IMAGE_SETUP))
#define IOG_PRINT_IMAGE_SETUP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), IOG_TYPE_PRINT_IMAGE_SETUP, IogPrintImageSetupClass))

struct _IogPrintImageSetup {
#if GTK_CHECK_VERSION (3, 4, 0)
	GtkGrid parent_instance;
#else
	GtkTable parent_instance;
#endif

	IogPrintImageSetupPrivate *priv;
};

struct _IogPrintImageSetupClass {
#if GTK_CHECK_VERSION (3, 4, 0)
	GtkGridClass parent_class;
#else
	GtkTableClass parent_class;
#endif
};

G_GNUC_INTERNAL
GType		  iog_print_image_setup_get_type    (void) G_GNUC_CONST;

G_GNUC_INTERNAL
GtkWidget        *iog_print_image_setup_new         (IogImage     *image,
						     GtkPageSetup *page_setup);

G_GNUC_INTERNAL
void              iog_print_image_setup_get_options (IogPrintImageSetup *setup,
						     gdouble            *left,
						     gdouble            *top,
						     gdouble            *scale,
						     GtkUnit            *unit);
void              iog_print_image_setup_update      (GtkPrintOperation *operation,
						     GtkWidget         *custom_widget,
						     GtkPageSetup      *page_setup,
						     GtkPrintSettings  *print_settings,
						     gpointer           user_data);

G_END_DECLS

#endif /* IOG_PRINT_IMAGE_SETUP_H */
