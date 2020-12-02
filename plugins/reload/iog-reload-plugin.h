#ifndef __IOG_RELOAD_PLUGIN_H__
#define __IOG_RELOAD_PLUGIN_H__

#include <glib.h>
#include <glib-object.h>
#include <iog-plugin.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define IOG_TYPE_RELOAD_PLUGIN \
	(iog_reload_plugin_get_type())
#define IOG_RELOAD_PLUGIN(o) \
	(G_TYPE_CHECK_INSTANCE_CAST((o), IOG_TYPE_RELOAD_PLUGIN, IogReloadPlugin))
#define IOG_RELOAD_PLUGIN_CLASS(k) \
	(G_TYPE_CHECK_CLASS_CAST((k), IOG_TYPE_RELOAD_PLUGIN, IogReloadPluginClass))
#define IOG_IS_RELOAD_PLUGIN(o) \
	(G_TYPE_CHECK_INSTANCE_TYPE((o), IOG_TYPE_RELOAD_PLUGIN))
#define IOG_IS_RELOAD_PLUGIN_CLASS(k) \
	(G_TYPE_CHECK_CLASS_TYPE((k), IOG_TYPE_RELOAD_PLUGIN))
#define IOG_RELOAD_PLUGIN_GET_CLASS(o) \
	(G_TYPE_INSTANCE_GET_CLASS((o), IOG_TYPE_RELOAD_PLUGIN, IogReloadPluginClass))

/* Private structure type */
typedef struct _IogReloadPluginPrivate IogReloadPluginPrivate;

/*
 * Main object structure
 */
typedef struct _IogReloadPlugin IogReloadPlugin;

struct _IogReloadPlugin {
	IogPlugin parent_instance;
};

/*
 * Class definition
 */
typedef struct _IogReloadPluginClass IogReloadPluginClass;

struct _IogReloadPluginClass {
	IogPluginClass parent_class;
};

/*
 * Public methods
 */
GType iog_reload_plugin_get_type(void) G_GNUC_CONST;

/* All the plugins must implement this function */
G_MODULE_EXPORT GType register_iog_plugin(GTypeModule* module);

G_END_DECLS

#endif /* __IOG_RELOAD_PLUGIN_H__ */
