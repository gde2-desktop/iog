#ifndef _IOG_SCROLL_VIEW_H_
#define _IOG_SCROLL_VIEW_H_

#include <gtk/gtk.h>
#include "iog-image.h"

G_BEGIN_DECLS

typedef struct _IogScrollView IogScrollView;
typedef struct _IogScrollViewClass IogScrollViewClass;
typedef struct _IogScrollViewPrivate IogScrollViewPrivate;

#define IOG_TYPE_SCROLL_VIEW              (iog_scroll_view_get_type ())
#define IOG_SCROLL_VIEW(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), IOG_TYPE_SCROLL_VIEW, IogScrollView))
#define IOG_SCROLL_VIEW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), IOG_TYPE_SCROLL_VIEW, IogScrollViewClass))
#define IOG_IS_SCROLL_VIEW(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IOG_TYPE_SCROLL_VIEW))
#define IOG_IS_SCROLL_VIEW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), IOG_TYPE_SCROLL_VIEW))


struct _IogScrollView {
#if GTK_CHECK_VERSION (3, 4, 0)
	GtkGrid base_instance;
#else
	GtkTable  widget;
#endif

	IogScrollViewPrivate *priv;
};

struct _IogScrollViewClass {
#if GTK_CHECK_VERSION (3, 4, 0)
	GtkGridClass parent_class;
#else
	GtkTableClass parent_class;
#endif

	void (* zoom_changed) (IogScrollView *view, double zoom);
};

typedef enum {
	IOG_TRANSP_BACKGROUND,
	IOG_TRANSP_CHECKED,
	IOG_TRANSP_COLOR
} IogTransparencyStyle;

GType    iog_scroll_view_get_type         (void) G_GNUC_CONST;
GtkWidget* iog_scroll_view_new            (void);

/* loading stuff */
void     iog_scroll_view_set_image        (IogScrollView *view, IogImage *image);

/* general properties */
void     iog_scroll_view_set_scroll_wheel_zoom (IogScrollView *view, gboolean scroll_wheel_zoom);
void     iog_scroll_view_set_zoom_upscale (IogScrollView *view, gboolean upscale);
void     iog_scroll_view_set_zoom_multiplier (IogScrollView *view, gdouble multiplier);
void     iog_scroll_view_set_antialiasing_in (IogScrollView *view, gboolean state);
void     iog_scroll_view_set_antialiasing_out (IogScrollView *view, gboolean state);
void     iog_scroll_view_set_transparency_color (IogScrollView *view, GdkColor *color);
void     iog_scroll_view_set_transparency (IogScrollView *view, IogTransparencyStyle style);
gboolean iog_scroll_view_scrollbars_visible (IogScrollView *view);
void	 iog_scroll_view_set_popup (IogScrollView *view, GtkMenu *menu);
void	 iog_scroll_view_set_background_color (IogScrollView *view,
					       const GdkColor *color);
void	 iog_scroll_view_override_bg_color (IogScrollView *view,
					    const GdkColor *color);
void     iog_scroll_view_set_use_bg_color (IogScrollView *view, gboolean use);
/* zoom api */
void     iog_scroll_view_zoom_in          (IogScrollView *view, gboolean smooth);
void     iog_scroll_view_zoom_out         (IogScrollView *view, gboolean smooth);
void     iog_scroll_view_zoom_fit         (IogScrollView *view);
void     iog_scroll_view_set_zoom         (IogScrollView *view, double zoom);
double   iog_scroll_view_get_zoom         (IogScrollView *view);
gboolean iog_scroll_view_get_zoom_is_min  (IogScrollView *view);
gboolean iog_scroll_view_get_zoom_is_max  (IogScrollView *view);
void     iog_scroll_view_show_cursor      (IogScrollView *view);
void     iog_scroll_view_hide_cursor      (IogScrollView *view);

G_END_DECLS

#endif /* _IOG_SCROLL_VIEW_H_ */


