#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif

#include "iog-reload-plugin.h"

#include <gmodule.h>
#include <glib/gi18n-lib.h>

#include <iog-debug.h>
#include <iog-scroll-view.h>
#include <iog-thumb-view.h>
#include <iog-image.h>

#define WINDOW_DATA_KEY "IogReloadWindowData"

IOG_PLUGIN_REGISTER_TYPE(IogReloadPlugin, iog_reload_plugin)

typedef struct {
	GtkActionGroup* ui_action_group;
	guint ui_id;
} WindowData;

static void reload_cb(GtkAction* action, IogWindow* window)
{
	iog_window_reload_image(window);
}

static const gchar* const ui_definition = "<ui><menubar name=\"MainMenu\">"
	"<menu name=\"ToolsMenu\" action=\"Tools\"><separator/>"
	"<menuitem name=\"IogPluginReload\" action=\"IogPluginRunReload\"/>"
	"<separator/></menu></menubar>"
	"<popup name=\"ViewPopup\"><separator/>"
	"<menuitem action=\"IogPluginRunReload\"/><separator/>"
	"</popup></ui>";

static const GtkActionEntry action_entries[] = {
	{"IogPluginRunReload", GTK_STOCK_REFRESH, N_("Reload Image"), "R", N_("Reload current image"), G_CALLBACK(reload_cb)}
};

static void free_window_data(WindowData* data)
{
	g_return_if_fail(data != NULL);

	iog_debug(DEBUG_PLUGINS);

	g_object_unref(data->ui_action_group);

	g_free(data);
}

static void iog_reload_plugin_init(IogReloadPlugin* plugin)
{
	iog_debug_message(DEBUG_PLUGINS, "IogReloadPlugin initializing");
}

static void iog_reload_plugin_finalize(GObject* object)
{
	iog_debug_message(DEBUG_PLUGINS, "IogReloadPlugin finalizing");

	G_OBJECT_CLASS(iog_reload_plugin_parent_class)->finalize(object);
}

static void impl_activate(IogPlugin* plugin, IogWindow* window)
{
	GtkUIManager* manager;
	WindowData* data;

	iog_debug(DEBUG_PLUGINS);

	data = g_new(WindowData, 1);

	manager = iog_window_get_ui_manager(window);

	data->ui_action_group = gtk_action_group_new("IogReloadPluginActions");

	gtk_action_group_set_translation_domain(data->ui_action_group, GETTEXT_PACKAGE);

	gtk_action_group_add_actions(data->ui_action_group, action_entries, G_N_ELEMENTS(action_entries), window);

	gtk_ui_manager_insert_action_group(manager, data->ui_action_group, -1);

	g_object_set_data_full(G_OBJECT(window), WINDOW_DATA_KEY, data, (GDestroyNotify) free_window_data);

	data->ui_id = gtk_ui_manager_add_ui_from_string(manager, ui_definition, -1, NULL);
	g_warn_if_fail (data->ui_id != 0);
}

static void impl_deactivate(IogPlugin* plugin, IogWindow* window)
{
	GtkUIManager* manager;
	WindowData* data;

	iog_debug(DEBUG_PLUGINS);

	manager = iog_window_get_ui_manager(window);

	data = (WindowData*) g_object_get_data(G_OBJECT(window), WINDOW_DATA_KEY);
	g_return_if_fail(data != NULL);

	gtk_ui_manager_remove_ui(manager, data->ui_id);

	gtk_ui_manager_remove_action_group(manager, data->ui_action_group);

	g_object_set_data(G_OBJECT(window), WINDOW_DATA_KEY, NULL);
}

static void iog_reload_plugin_class_init(IogReloadPluginClass* klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS(klass);
	IogPluginClass* plugin_class = IOG_PLUGIN_CLASS(klass);

	object_class->finalize = iog_reload_plugin_finalize;

	plugin_class->activate = impl_activate;
	plugin_class->deactivate = impl_deactivate;
}
