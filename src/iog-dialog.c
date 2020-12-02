/* Eye Of Gde2 - Image  Dialog
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "iog-dialog.h"

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#define IOG_DIALOG_GET_PRIVATE(object) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((object), IOG_TYPE_DIALOG, IogDialogPrivate))

G_DEFINE_TYPE (IogDialog, iog_dialog, G_TYPE_OBJECT);

enum {
        PROP_0,
        PROP_PARENT_WINDOW,
};

struct _IogDialogPrivate {
	GtkWidget   *dlg;
	GtkBuilder  *xml;
	GtkWindow   *parent;
};

static void
iog_dialog_construct_impl (IogDialog   *dialog,
			   const gchar *ui_file,
			   const gchar *dlg_node)
{
	IogDialogPrivate *priv;
	gchar *filename;

	g_return_if_fail (dialog != NULL);
	g_return_if_fail (IOG_IS_DIALOG (dialog));

	priv = dialog->priv;

	filename = g_build_filename (IOG_DATA_DIR, ui_file, NULL);

	priv->xml = gtk_builder_new ();
	gtk_builder_set_translation_domain (priv->xml, GETTEXT_PACKAGE);
	g_assert (gtk_builder_add_from_file (priv->xml, filename, NULL));

	g_free (filename);

	priv->dlg = GTK_WIDGET (gtk_builder_get_object (priv->xml, dlg_node));

	if (priv->parent != NULL) {
		gtk_window_set_transient_for (GTK_WINDOW (priv->dlg),
					      priv->parent);
	}
}

static void
iog_dialog_show_impl (IogDialog *dialog)
{
	g_return_if_fail (dialog != NULL);
	g_return_if_fail (IOG_IS_DIALOG (dialog));

	gtk_window_present (GTK_WINDOW (dialog->priv->dlg));
}

static void
iog_dialog_hide_impl (IogDialog *dialog)
{
	g_return_if_fail (dialog != NULL);
	g_return_if_fail (IOG_IS_DIALOG (dialog));

	gtk_widget_hide (dialog->priv->dlg);
}

static void
iog_dialog_set_property (GObject      *object,
			 guint         prop_id,
			 const GValue *value,
			 GParamSpec   *pspec)
{
	IogDialog *dialog = IOG_DIALOG (object);

	switch (prop_id) {
		case PROP_PARENT_WINDOW:
			dialog->priv->parent = g_value_get_object (value);
			break;
	}
}

static void
iog_dialog_get_property (GObject    *object,
			 guint       prop_id,
			 GValue     *value,
			 GParamSpec *pspec)
{
	IogDialog *dialog = IOG_DIALOG (object);

	switch (prop_id) {
		case PROP_PARENT_WINDOW:
			g_value_set_object (value, dialog->priv->parent);
			break;
	}
}

static void
iog_dialog_dispose (GObject *object)
{
	IogDialog *dialog;
	IogDialogPrivate *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (IOG_IS_DIALOG (object));

	dialog = IOG_DIALOG (object);
	priv = dialog->priv;

	if (priv->dlg) {
		gtk_widget_destroy (priv->dlg);
		priv->dlg = NULL;
	}

	if (priv->xml) {
		g_object_unref (priv->xml);
		priv->xml = NULL;
	}

	G_OBJECT_CLASS (iog_dialog_parent_class)->dispose (object);
}

static void
iog_dialog_class_init (IogDialogClass *class)
{
	GObjectClass *g_object_class = (GObjectClass *) class;

	g_object_class->dispose = iog_dialog_dispose;
	g_object_class->set_property = iog_dialog_set_property;
	g_object_class->get_property = iog_dialog_get_property;

	class->construct = iog_dialog_construct_impl;
	class->show = iog_dialog_show_impl;
	class->hide = iog_dialog_hide_impl;

	g_object_class_install_property (g_object_class,
					 PROP_PARENT_WINDOW,
					 g_param_spec_object ("parent-window",
							      "Parent window",
							      "Parent window",
							      GTK_TYPE_WINDOW,
							      G_PARAM_READWRITE |
							      G_PARAM_CONSTRUCT_ONLY |
							      G_PARAM_STATIC_NAME |
							      G_PARAM_STATIC_NICK |
							      G_PARAM_STATIC_BLURB));

	g_type_class_add_private (g_object_class, sizeof (IogDialogPrivate));
}

static void
iog_dialog_init (IogDialog *dialog)
{
	dialog->priv = IOG_DIALOG_GET_PRIVATE (dialog);

	dialog->priv->dlg = NULL;
	dialog->priv->xml = NULL;
	dialog->priv->parent = NULL;
}

void
iog_dialog_construct (IogDialog   *dialog,
		      const gchar *ui_file,
		      const gchar *dlg_node)
{
	IogDialogClass *klass = IOG_DIALOG_GET_CLASS (dialog);
	klass->construct (dialog, ui_file, dlg_node);
}

void
iog_dialog_show (IogDialog *dialog)
{
	IogDialogClass *klass = IOG_DIALOG_GET_CLASS (dialog);
	klass->show (dialog);
}

void
iog_dialog_hide (IogDialog *dialog)
{
	IogDialogClass *klass = IOG_DIALOG_GET_CLASS (dialog);
	klass->hide (dialog);
}

void
iog_dialog_get_controls (IogDialog   *dialog,
			 const gchar *property_id,
			 ...)
{
	IogDialogPrivate *priv;
        GtkWidget **wptr;
        va_list varargs;

	g_return_if_fail (dialog != NULL);
	g_return_if_fail (IOG_IS_DIALOG (dialog));

	priv = dialog->priv;

        va_start (varargs, property_id);

        while (property_id != NULL)
        {
                wptr = va_arg (varargs, GtkWidget **);
                *wptr = GTK_WIDGET (gtk_builder_get_object (priv->xml,
							    property_id));

                property_id = va_arg (varargs, const gchar *);
        }

        va_end (varargs);
}
