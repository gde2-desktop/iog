#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif

#include "iog-fullscreen-plugin.h"

#include <gmodule.h>
#include <glib/gi18n-lib.h>

#include <iog-debug.h>
#include <iog-scroll-view.h>

#define WINDOW_DATA_KEY "IogFullscreenWindowData"

IOG_PLUGIN_REGISTER_TYPE(IogFullscreenPlugin, iog_fullscreen_plugin)

typedef struct {
	gulong signal_id;
} WindowData;

static gboolean on_button_press(GtkWidget* button, GdkEventButton* event, IogWindow* window)
{
	if (event->button == 1 && event->type == GDK_2BUTTON_PRESS)
	{
		IogWindowMode mode = iog_window_get_mode(window);

		if (mode == IOG_WINDOW_MODE_SLIDESHOW || mode == IOG_WINDOW_MODE_FULLSCREEN)
		{
			iog_window_set_mode(window, IOG_WINDOW_MODE_NORMAL);
		}
		else if (mode == IOG_WINDOW_MODE_NORMAL)
		{
			iog_window_set_mode(window, IOG_WINDOW_MODE_FULLSCREEN);
		}

		return TRUE;
	}

	return FALSE;
}

static void free_window_data(WindowData* data)
{
	g_return_if_fail(data != NULL);

	iog_debug(DEBUG_PLUGINS);

	g_free(data);
}

static void iog_fullscreen_plugin_init (IogFullscreenPlugin* plugin)
{
	iog_debug_message(DEBUG_PLUGINS, "IogFullscreenPlugin initializing");
}

static void iog_fullscreen_plugin_finalize(GObject* object)
{
	iog_debug_message (DEBUG_PLUGINS, "IogFullscreenPlugin finalizing");

	G_OBJECT_CLASS(iog_fullscreen_plugin_parent_class)->finalize(object);
}

static void impl_activate(IogPlugin* plugin, IogWindow* window)
{
	GtkWidget* view = iog_window_get_view(window);
	WindowData* data;

	iog_debug(DEBUG_PLUGINS);

	data = g_new(WindowData, 1);

	data->signal_id = g_signal_connect(G_OBJECT(view), "button-press-event", G_CALLBACK(on_button_press), window);

	g_object_set_data_full(G_OBJECT(window), WINDOW_DATA_KEY, data, (GDestroyNotify) free_window_data);
}

static void impl_deactivate(IogPlugin* plugin, IogWindow* window)
{
	GtkWidget* view = iog_window_get_view(window);
	WindowData* data = (WindowData*) g_object_get_data(G_OBJECT(window), WINDOW_DATA_KEY);

	g_signal_handler_disconnect(view, data->signal_id);

	g_object_set_data(G_OBJECT(window), WINDOW_DATA_KEY, NULL);
}

static void impl_update_ui(IogPlugin* plugin, IogWindow* window)
{
	/* Nothing */
}

static void iog_fullscreen_plugin_class_init(IogFullscreenPluginClass* klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS(klass);
	IogPluginClass* plugin_class = IOG_PLUGIN_CLASS(klass);

	object_class->finalize = iog_fullscreen_plugin_finalize;

	plugin_class->activate = impl_activate;
	plugin_class->deactivate = impl_deactivate;
	plugin_class->update_ui = impl_update_ui;
}
