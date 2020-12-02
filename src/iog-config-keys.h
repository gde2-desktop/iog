/* Eye Of Gde2 - GSettings Keys and Schemas definitions
 *
 * Copyright (C) 2000-2006 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 *         Stefano Karapetsas <stefano@karapetsas.com>
 *
 * Based on code by:
 *  - Federico Mena-Quintero <federico@gnu.org>
 *  - Jens Finke <jens@gnome.org>
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

#ifndef __IOG_CONFIG_KEYS_H__
#define __IOG_CONFIG_KEYS_H__

#define IOG_CONF_DOMAIN				"org.gde2.iog"
#define IOG_CONF_FULLSCREEN			IOG_CONF_DOMAIN".full-screen"
#define IOG_CONF_PLUGINS			IOG_CONF_DOMAIN".plugins"
#define IOG_CONF_UI				IOG_CONF_DOMAIN".ui"
#define IOG_CONF_VIEW				IOG_CONF_DOMAIN".view"

#define IOG_CONF_BACKGROUND_SCHEMA              "org.gde2.background"
#define IOG_CONF_BACKGROUND_FILE                "picture-filename"

#define IOG_CONF_LOCKDOWN_SCHEMA                "org.gde2.lockdown"
#define IOG_CONF_LOCKDOWN_CAN_SAVE              "disable-save-to-disk"
#define IOG_CONF_LOCKDOWN_CAN_PRINT             "disable-printing"
#define IOG_CONF_LOCKDOWN_CAN_SETUP_PAGE        "disable-print-setup"

#define IOG_CONF_VIEW_BACKGROUND_COLOR          "background-color"
#define IOG_CONF_VIEW_INTERPOLATE               "interpolate"
#define IOG_CONF_VIEW_EXTRAPOLATE               "extrapolate"
#define IOG_CONF_VIEW_SCROLL_WHEEL_ZOOM         "scroll-wheel-zoom"
#define IOG_CONF_VIEW_ZOOM_MULTIPLIER           "zoom-multiplier"
#define IOG_CONF_VIEW_AUTOROTATE                "autorotate"
#define IOG_CONF_VIEW_TRANSPARENCY              "transparency"
#define IOG_CONF_VIEW_TRANS_COLOR               "trans-color"
#define IOG_CONF_VIEW_USE_BG_COLOR              "use-background-color"

#define IOG_CONF_FULLSCREEN_RANDOM              "random"
#define IOG_CONF_FULLSCREEN_LOOP                "loop"
#define IOG_CONF_FULLSCREEN_UPSCALE             "upscale"
#define IOG_CONF_FULLSCREEN_SECONDS             "seconds"

#define IOG_CONF_UI_TOOLBAR                     "toolbar"
#define IOG_CONF_UI_STATUSBAR                   "statusbar"
#define IOG_CONF_UI_IMAGE_COLLECTION            "image-collection"
#define IOG_CONF_UI_IMAGE_COLLECTION_POSITION   "image-collection-position"
#define IOG_CONF_UI_IMAGE_COLLECTION_RESIZABLE  "image-collection-resizable"
#define IOG_CONF_UI_SIDEBAR                     "sidebar"
#define IOG_CONF_UI_SCROLL_BUTTONS              "scroll-buttons"
#define IOG_CONF_UI_DISABLE_CLOSE_CONFIRMATION	"disable-close-confirmation"
#define IOG_CONF_UI_DISABLE_TRASH_CONFIRMATION  "disable-trash-confirmation"
#define IOG_CONF_UI_FILECHOOSER_XDG_FALLBACK    "filechooser-xdg-fallback"
#define IOG_CONF_UI_PROPSDIALOG_NETBOOK_MODE    "propsdialog-netbook-mode"
#define IOG_CONF_UI_EXTERNAL_EDITOR             "external-editor"

#define IOG_CONF_PLUGINS_ACTIVE_PLUGINS         "active-plugins"

#endif /* __IOG_CONFIG_KEYS_H__ */
