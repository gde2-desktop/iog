#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <math.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdkkeysyms.h>
#ifdef HAVE_RSVG
#include <librsvg/rsvg.h>
#endif

#include "iog-config-keys.h"
#include "iog-enum-types.h"
#include "iog-marshal.h"
#include "iog-scroll-view.h"
#include "iog-debug.h"
#include "zoom.h"

#include <gdk/gdk.h>

/* Maximum size of delayed repaint rectangles */
#define PAINT_RECT_WIDTH 128
#define PAINT_RECT_HEIGHT 128

/* Scroll step increment */
#define SCROLL_STEP_SIZE 32

/* Maximum zoom factor */
#define MAX_ZOOM_FACTOR 20
#define MIN_ZOOM_FACTOR 0.02

#define CHECK_MEDIUM 8
#define CHECK_BLACK 0x00000000
#define CHECK_DARK 0x00555555
#define CHECK_GRAY 0x00808080
#define CHECK_LIGHT 0x00cccccc
#define CHECK_WHITE 0x00ffffff

/* Default increment for zooming.  The current zoom factor is multiplied or
 * divided by this amount on every zooming step.  For consistency, you should
 * use the same value elsewhere in the program.
 */
#define IMAGE_VIEW_ZOOM_MULTIPLIER 1.05

/* States for automatically adjusting the zoom factor */
typedef enum {
	ZOOM_MODE_FIT,		/* Image is fitted to scroll view even if the latter changes size */
	ZOOM_MODE_FREE		/* The image remains at its current zoom factor even if the scrollview changes size  */
} ZoomMode;

/* Signal IDs */
enum {
	SIGNAL_ZOOM_CHANGED,
	SIGNAL_LAST
};

static guint view_signals [SIGNAL_LAST] = { 0 };

typedef enum {
	IOG_SCROLL_VIEW_CURSOR_NORMAL,
	IOG_SCROLL_VIEW_CURSOR_HIDDEN,
	IOG_SCROLL_VIEW_CURSOR_DRAG
} IogScrollViewCursor;

/* Drag 'n Drop */
static GtkTargetEntry target_table[] = {
	{ "text/uri-list", 0, 0},
};

enum {
	PROP_0,
	PROP_ANTIALIAS_IN,
	PROP_ANTIALIAS_OUT,
	PROP_BACKGROUND_COLOR,
	PROP_SCROLLWHEEL_ZOOM,
	PROP_TRANSP_COLOR,
	PROP_TRANSPARENCY_STYLE,
	PROP_USE_BG_COLOR,
	PROP_ZOOM_MULTIPLIER
};

/* Private part of the IogScrollView structure */
struct _IogScrollViewPrivate {
	/* some widgets we rely on */
	GtkWidget *display;
	GtkAdjustment *hadj;
	GtkAdjustment *vadj;
	GtkWidget *hbar;
	GtkWidget *vbar;
	GtkWidget *menu;

	/* actual image */
	IogImage *image;
	guint image_changed_id;
	guint frame_changed_id;
	GdkPixbuf *pixbuf;
	cairo_surface_t *surface;

	/* zoom mode, either ZOOM_MODE_FIT or ZOOM_MODE_FREE */
	ZoomMode zoom_mode;

	/* whether to allow zoom > 1.0 on zoom fit */
	gboolean upscale;

	/* the actual zoom factor */
	double zoom;

	/* the minimum possible (reasonable) zoom factor */
	double min_zoom;

	/* Current scrolling offsets */
	int xofs, yofs;

	/* handler ID for paint idle callback */
	guint idle_id;

	/* Interpolation type when zoomed in*/
	cairo_filter_t interp_type_in;

	/* Interpolation type when zoomed out*/
	cairo_filter_t interp_type_out;

	/* Scroll wheel zoom */
	gboolean scroll_wheel_zoom;

	/* Scroll wheel zoom */
	gdouble zoom_multiplier;

	/* dragging stuff */
	int drag_anchor_x, drag_anchor_y;
	int drag_ofs_x, drag_ofs_y;
	guint dragging : 1;

	/* how to indicate transparency in images */
	IogTransparencyStyle transp_style;
	guint32 transp_color;

	/* the type of the cursor we are currently showing */
	IogScrollViewCursor cursor;

	gboolean  use_bg_color;
	GdkColor *background_color;
	GdkColor *override_bg_color;

	cairo_surface_t *background_surface;
};

static void scroll_by (IogScrollView *view, int xofs, int yofs);
static void set_zoom_fit (IogScrollView *view);
/* static void request_paint_area (IogScrollView *view, GdkRectangle *area); */
static void set_minimum_zoom_factor (IogScrollView *view);
static void view_on_drag_begin_cb (GtkWidget *widget, GdkDragContext *context,
				   gpointer user_data);
static void view_on_drag_data_get_cb (GtkWidget *widget,
				      GdkDragContext*drag_context,
				      GtkSelectionData *data, guint info,
				      guint time, gpointer user_data);

#define IOG_SCROLL_VIEW_GET_PRIVATE(object) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((object), IOG_TYPE_SCROLL_VIEW, IogScrollViewPrivate))

#if GTK_CHECK_VERSION (3, 4, 0)
G_DEFINE_TYPE (IogScrollView, iog_scroll_view, GTK_TYPE_GRID)
#else
G_DEFINE_TYPE (IogScrollView, iog_scroll_view, GTK_TYPE_TABLE)
#endif

/*===================================
    widget size changing handler &
        util functions
  ---------------------------------*/

static cairo_surface_t *
create_surface_from_pixbuf (IogScrollView *view, GdkPixbuf *pixbuf)
{
	cairo_surface_t *surface;
	cairo_t *cr;

	surface = gdk_window_create_similar_surface (gtk_widget_get_window (view->priv->display),
							CAIRO_CONTENT_COLOR | CAIRO_CONTENT_ALPHA,
							gdk_pixbuf_get_width (pixbuf),
							gdk_pixbuf_get_height (pixbuf));
	cr = cairo_create (surface);
	gdk_cairo_set_source_pixbuf (cr, pixbuf, 0, 0);
	cairo_paint (cr);
	cairo_destroy (cr);

	return surface;
}

/* Disconnects from the IogImage and removes references to it */
static void
free_image_resources (IogScrollView *view)
{
	IogScrollViewPrivate *priv;

	priv = view->priv;

	if (priv->image_changed_id > 0) {
		g_signal_handler_disconnect (G_OBJECT (priv->image), priv->image_changed_id);
		priv->image_changed_id = 0;
	}

	if (priv->frame_changed_id > 0) {
		g_signal_handler_disconnect (G_OBJECT (priv->image), priv->frame_changed_id);
		priv->frame_changed_id = 0;
	}

	if (priv->image != NULL) {
		iog_image_data_unref (priv->image);
		priv->image = NULL;
	}

	if (priv->pixbuf != NULL) {
		g_object_unref (priv->pixbuf);
		priv->pixbuf = NULL;
	}

	if (priv->surface !=NULL) {
		cairo_surface_destroy (priv->surface);
		priv->surface = NULL;
	}
}

/* Computes the size in pixels of the scaled image */
static void
compute_scaled_size (IogScrollView *view, double zoom, int *width, int *height)
{
	IogScrollViewPrivate *priv;

	priv = view->priv;

	if (priv->pixbuf) {
		*width = floor (gdk_pixbuf_get_width (priv->pixbuf) * zoom + 0.5);
		*height = floor (gdk_pixbuf_get_height (priv->pixbuf) * zoom + 0.5);
	} else
		*width = *height = 0;
}

/* Computes the offsets for the new zoom value so that they keep the image
 * centered on the view.
 */
static void
compute_center_zoom_offsets (IogScrollView *view,
			     double old_zoom, double new_zoom,
			     int width, int height,
			     double zoom_x_anchor, double zoom_y_anchor,
			     int *xofs, int *yofs)
{
	IogScrollViewPrivate *priv;
	int old_scaled_width, old_scaled_height;
	int new_scaled_width, new_scaled_height;
	double view_cx, view_cy;

	priv = view->priv;

	compute_scaled_size (view, old_zoom,
			     &old_scaled_width, &old_scaled_height);

	if (old_scaled_width < width)
		view_cx = (zoom_x_anchor * old_scaled_width) / old_zoom;
	else
		view_cx = (priv->xofs + zoom_x_anchor * width) / old_zoom;

	if (old_scaled_height < height)
		view_cy = (zoom_y_anchor * old_scaled_height) / old_zoom;
	else
		view_cy = (priv->yofs + zoom_y_anchor * height) / old_zoom;

	compute_scaled_size (view, new_zoom,
			     &new_scaled_width, &new_scaled_height);

	if (new_scaled_width < width)
		*xofs = 0;
	else {
		*xofs = floor (view_cx * new_zoom - zoom_x_anchor * width + 0.5);
		if (*xofs < 0)
			*xofs = 0;
	}

	if (new_scaled_height < height)
		*yofs = 0;
	else {
		*yofs = floor (view_cy * new_zoom - zoom_y_anchor * height + 0.5);
		if (*yofs < 0)
			*yofs = 0;
	}
}

/* Sets the scrollbar values based on the current scrolling offset */
static void
update_scrollbar_values (IogScrollView *view)
{
	IogScrollViewPrivate *priv;
	int scaled_width, scaled_height;
	gdouble page_size,page_increment,step_increment;
	gdouble lower, upper;
	GtkAllocation allocation;

	priv = view->priv;

	if (!gtk_widget_get_visible (GTK_WIDGET (priv->hbar))
	    && !gtk_widget_get_visible (GTK_WIDGET (priv->vbar)))
		return;

	compute_scaled_size (view, priv->zoom, &scaled_width, &scaled_height);
	gtk_widget_get_allocation (GTK_WIDGET (priv->display), &allocation);

	if (gtk_widget_get_visible (GTK_WIDGET (priv->hbar))) {
		/* Set scroll increments */
		page_size = MIN (scaled_width, allocation.width);

		page_increment = allocation.width / 2;
		step_increment = SCROLL_STEP_SIZE;

		/* Set scroll bounds and new offsets */
		lower = 0;
		upper = scaled_width;
		priv->xofs = CLAMP (priv->xofs, 0, upper - page_size);

		g_signal_handlers_block_matched (
			priv->hadj, G_SIGNAL_MATCH_DATA,
			0, 0, NULL, NULL, view);

		gtk_adjustment_configure (priv->hadj, priv->xofs, lower,
					  upper, step_increment,
					  page_increment, page_size);

		g_signal_handlers_unblock_matched (
			priv->hadj, G_SIGNAL_MATCH_DATA,
			0, 0, NULL, NULL, view);
	}

	if (gtk_widget_get_visible (GTK_WIDGET (priv->vbar))) {
		page_size = MIN (scaled_height, allocation.height);
		page_increment = allocation.height / 2;
		step_increment = SCROLL_STEP_SIZE;

		lower = 0;
		upper = scaled_height;
		priv->yofs = CLAMP (priv->yofs, 0, upper - page_size);

		g_signal_handlers_block_matched (
			priv->vadj, G_SIGNAL_MATCH_DATA,
			0, 0, NULL, NULL, view);

		gtk_adjustment_configure (priv->vadj, priv->yofs, lower,
					  upper, step_increment,
					  page_increment, page_size);

		g_signal_handlers_unblock_matched (
			priv->vadj, G_SIGNAL_MATCH_DATA,
			0, 0, NULL, NULL, view);
	}
}

static void
iog_scroll_view_set_cursor (IogScrollView *view, IogScrollViewCursor new_cursor)
{
	GdkCursor *cursor = NULL;
	GdkDisplay *display;
	GtkWidget *widget;

	if (view->priv->cursor == new_cursor) {
		return;
	}

	widget = gtk_widget_get_toplevel (GTK_WIDGET (view));
	display = gtk_widget_get_display (widget);
	view->priv->cursor = new_cursor;

	switch (new_cursor) {
		case IOG_SCROLL_VIEW_CURSOR_NORMAL:
			gdk_window_set_cursor (gtk_widget_get_window (widget), NULL);
			break;
                case IOG_SCROLL_VIEW_CURSOR_HIDDEN:
                        cursor = gdk_cursor_new_for_display (display, GDK_BLANK_CURSOR);
                        break;
		case IOG_SCROLL_VIEW_CURSOR_DRAG:
			cursor = gdk_cursor_new_for_display (display, GDK_FLEUR);
			break;
	}

	if (cursor) {
		gdk_window_set_cursor (gtk_widget_get_window (widget), cursor);
#if GTK_CHECK_VERSION (3, 0, 0)
		g_object_unref (cursor);
#else
		gdk_cursor_unref (cursor);
#endif
		gdk_flush();
	}
}

/* Changes visibility of the scrollbars based on the zoom factor and the
 * specified allocation, or the current allocation if NULL is specified.
 */
static void
check_scrollbar_visibility (IogScrollView *view, GtkAllocation *alloc)
{
	IogScrollViewPrivate *priv;
	int bar_height;
	int bar_width;
	int img_width;
	int img_height;
	GtkRequisition req;
	int width, height;
	gboolean hbar_visible, vbar_visible;

	priv = view->priv;

	if (alloc) {
		width = alloc->width;
		height = alloc->height;
	} else {
		GtkAllocation allocation;

		gtk_widget_get_allocation (GTK_WIDGET (view), &allocation);
		width = allocation.width;
		height = allocation.height;
	}

	compute_scaled_size (view, priv->zoom, &img_width, &img_height);

	/* this should work fairly well in this special case for scrollbars */
#if GTK_CHECK_VERSION (3, 0, 0)
	gtk_widget_get_preferred_size (priv->hbar, &req, NULL);
	bar_height = req.height;
	gtk_widget_get_preferred_size (priv->vbar, &req, NULL);
#else
	gtk_widget_size_request (priv->hbar, &req);
	bar_height = req.height;
	gtk_widget_size_request (priv->vbar, &req);
#endif
	bar_width = req.width;

	iog_debug_message (DEBUG_WINDOW, "Widget Size allocate: %i, %i   Bar: %i, %i\n",
			   width, height, bar_width, bar_height);

	hbar_visible = vbar_visible = FALSE;
	if (priv->zoom_mode == ZOOM_MODE_FIT)
		hbar_visible = vbar_visible = FALSE;
	else if (img_width <= width && img_height <= height)
		hbar_visible = vbar_visible = FALSE;
	else if (img_width > width && img_height > height)
		hbar_visible = vbar_visible = TRUE;
	else if (img_width > width) {
		hbar_visible = TRUE;
		if (img_height <= (height - bar_height))
			vbar_visible = FALSE;
		else
			vbar_visible = TRUE;
	}
        else if (img_height > height) {
		vbar_visible = TRUE;
		if (img_width <= (width - bar_width))
			hbar_visible = FALSE;
		else
			hbar_visible = TRUE;
	}

	if (hbar_visible != gtk_widget_get_visible (GTK_WIDGET (priv->hbar)))
		g_object_set (G_OBJECT (priv->hbar), "visible", hbar_visible, NULL);

	if (vbar_visible != gtk_widget_get_visible (GTK_WIDGET (priv->vbar)))
		g_object_set (G_OBJECT (priv->vbar), "visible", vbar_visible, NULL);
}

#define DOUBLE_EQUAL_MAX_DIFF 1e-6
#define DOUBLE_EQUAL(a,b) (fabs (a - b) < DOUBLE_EQUAL_MAX_DIFF)

/* Returns whether the image is zoomed in */
static gboolean
is_zoomed_in (IogScrollView *view)
{
	IogScrollViewPrivate *priv;

	priv = view->priv;
	return priv->zoom - 1.0 > DOUBLE_EQUAL_MAX_DIFF;
}

/* Returns whether the image is zoomed out */
static gboolean
is_zoomed_out (IogScrollView *view)
{
	IogScrollViewPrivate *priv;

	priv = view->priv;
	return DOUBLE_EQUAL_MAX_DIFF + priv->zoom - 1.0 < 0.0;
}

/* Returns wether the image is movable, that means if it is larger then
 * the actual visible area.
 */
static gboolean
is_image_movable (IogScrollView *view)
{
	IogScrollViewPrivate *priv;

	priv = view->priv;

	return (gtk_widget_get_visible (priv->hbar) || gtk_widget_get_visible (priv->vbar));
}

/*===================================
          drawing core
  ---------------------------------*/

static void
get_transparency_params (IogScrollView *view, int *size, guint32 *color1, guint32 *color2)
{
	IogScrollViewPrivate *priv;

	priv = view->priv;

	/* Compute transparency parameters */
	switch (priv->transp_style) {
	case IOG_TRANSP_BACKGROUND: {
		GdkColor color = gtk_widget_get_style (GTK_WIDGET (priv->display))->bg[GTK_STATE_NORMAL];

		*color1 = *color2 = (((color.red & 0xff00) << 8)
				       | (color.green & 0xff00)
				       | ((color.blue & 0xff00) >> 8));
		break; }

	case IOG_TRANSP_CHECKED:
		*color1 = CHECK_GRAY;
		*color2 = CHECK_LIGHT;
		break;

	case IOG_TRANSP_COLOR:
		*color1 = *color2 = priv->transp_color;
		break;

	default:
		g_assert_not_reached ();
	};

	*size = CHECK_MEDIUM;
}

static cairo_surface_t *
create_background_surface (IogScrollView *view)
{
	int check_size;
	guint32 check_1 = 0;
	guint32 check_2 = 0;
	cairo_surface_t *surface;

	get_transparency_params (view, &check_size, &check_1, &check_2);
	surface = gdk_window_create_similar_surface (gtk_widget_get_window (view->priv->display),
						     CAIRO_CONTENT_COLOR,
						     check_size * 2, check_size * 2);
	cairo_t* cr = cairo_create (surface);
	cairo_set_source_rgba (cr,
			       ((check_1 & 0xff0000) >> 16) / 255.,
			       ((check_1 & 0x00ff00) >> 8)  / 255.,
			       (check_1 & 0x0000ff)         / 255.,
			       1.);
	cairo_rectangle (cr, 0, 0, check_size, check_size);
	cairo_rectangle (cr, check_size, check_size, check_size, check_size);
	cairo_fill (cr);

	cairo_set_source_rgba (cr,
			       ((check_2 & 0xff0000) >> 16) / 255.,
			       ((check_2 & 0x00ff00) >> 8)  / 255.,
			       (check_2 & 0x0000ff)         / 255.,
			       1.);
	cairo_rectangle (cr, 0, check_size, check_size, check_size);
	cairo_rectangle (cr, check_size, 0, check_size, check_size);
	cairo_fill (cr);

	cairo_destroy (cr);

	return surface;
}

/* =======================================

    scrolling stuff

    --------------------------------------*/


/* Scrolls the view to the specified offsets.  */
static void
scroll_to (IogScrollView *view, int x, int y, gboolean change_adjustments)
{
	IogScrollViewPrivate *priv;
	GtkAllocation allocation;
	int xofs, yofs;
	GdkWindow *window;

	priv = view->priv;

	/* Check bounds & Compute offsets */
	if (gtk_widget_get_visible (priv->hbar)) {
		x = CLAMP (x, 0, gtk_adjustment_get_upper (priv->hadj)
				 - gtk_adjustment_get_page_size (priv->hadj));
		xofs = x - priv->xofs;
	} else
		xofs = 0;

	if (gtk_widget_get_visible (priv->vbar)) {
		y = CLAMP (y, 0, gtk_adjustment_get_upper (priv->vadj)
				 - gtk_adjustment_get_page_size (priv->vadj));
		yofs = y - priv->yofs;
	} else
		yofs = 0;

	if (xofs == 0 && yofs == 0)
		return;

	priv->xofs = x;
	priv->yofs = y;

	if (!gtk_widget_is_drawable (priv->display))
		goto out;

	gtk_widget_get_allocation (GTK_WIDGET (priv->display), &allocation);

	if (abs (xofs) >= allocation.width || abs (yofs) >= allocation.height) {
		gtk_widget_queue_draw (GTK_WIDGET (priv->display));
		goto out;
	}

	window = gtk_widget_get_window (GTK_WIDGET (priv->display));

	/* Scroll the window area and process exposure synchronously. */

	gdk_window_scroll (window, -xofs, -yofs);
	gdk_window_process_updates (window, TRUE);

 out:
	if (!change_adjustments)
		return;

	g_signal_handlers_block_matched (
		priv->hadj, G_SIGNAL_MATCH_DATA,
		0, 0, NULL, NULL, view);
	g_signal_handlers_block_matched (
		priv->vadj, G_SIGNAL_MATCH_DATA,
		0, 0, NULL, NULL, view);

	gtk_adjustment_set_value (priv->hadj, x);
	gtk_adjustment_set_value (priv->vadj, y);

	g_signal_handlers_unblock_matched (
		priv->hadj, G_SIGNAL_MATCH_DATA,
		0, 0, NULL, NULL, view);
	g_signal_handlers_unblock_matched (
		priv->vadj, G_SIGNAL_MATCH_DATA,
		0, 0, NULL, NULL, view);
}

/* Scrolls the image view by the specified offsets.  Notifies the adjustments
 * about their new values.
 */
static void
scroll_by (IogScrollView *view, int xofs, int yofs)
{
	IogScrollViewPrivate *priv;

	priv = view->priv;

	scroll_to (view, priv->xofs + xofs, priv->yofs + yofs, TRUE);
}


/* Callback used when an adjustment is changed */
static void
adjustment_changed_cb (GtkAdjustment *adj, gpointer data)
{
	IogScrollView *view;
	IogScrollViewPrivate *priv;

	view = IOG_SCROLL_VIEW (data);
	priv = view->priv;

	scroll_to (view, gtk_adjustment_get_value (priv->hadj),
		   gtk_adjustment_get_value (priv->vadj), FALSE);
}


/* Drags the image to the specified position */
static void
drag_to (IogScrollView *view, int x, int y)
{
	IogScrollViewPrivate *priv;
	int dx, dy;

	priv = view->priv;

	dx = priv->drag_anchor_x - x;
	dy = priv->drag_anchor_y - y;

	x = priv->drag_ofs_x + dx;
	y = priv->drag_ofs_y + dy;

	scroll_to (view, x, y, TRUE);
}

static void
set_minimum_zoom_factor (IogScrollView *view)
{
	g_return_if_fail (IOG_IS_SCROLL_VIEW (view));

	view->priv->min_zoom = MAX (1.0 / gdk_pixbuf_get_width (view->priv->pixbuf),
				    MAX(1.0 / gdk_pixbuf_get_height (view->priv->pixbuf),
					MIN_ZOOM_FACTOR) );
	return;
}

/**
 * set_zoom:
 * @view: A scroll view.
 * @zoom: Zoom factor.
 * @have_anchor: Whether the anchor point specified by (@anchorx, @anchory)
 * should be used.
 * @anchorx: Horizontal anchor point in pixels.
 * @anchory: Vertical anchor point in pixels.
 *
 * Sets the zoom factor for an image view.  The anchor point can be used to
 * specify the point that stays fixed when the image is zoomed.  If @have_anchor
 * is %TRUE, then (@anchorx, @anchory) specify the point relative to the image
 * view widget's allocation that will stay fixed when zooming.  If @have_anchor
 * is %FALSE, then the center point of the image view will be used.
 **/
static void
set_zoom (IogScrollView *view, double zoom,
	  gboolean have_anchor, int anchorx, int anchory)
{
	IogScrollViewPrivate *priv;
	GtkAllocation allocation;
	int xofs, yofs;
	double x_rel, y_rel;

	priv = view->priv;

	if (priv->pixbuf == NULL)
		return;

	if (zoom > MAX_ZOOM_FACTOR)
		zoom = MAX_ZOOM_FACTOR;
	else if (zoom < MIN_ZOOM_FACTOR)
		zoom = MIN_ZOOM_FACTOR;

	if (DOUBLE_EQUAL (priv->zoom, zoom))
		return;
	if (DOUBLE_EQUAL (priv->zoom, priv->min_zoom) && zoom < priv->zoom)
		return;

	priv->zoom_mode = ZOOM_MODE_FREE;

	gtk_widget_get_allocation (GTK_WIDGET (priv->display), &allocation);

	/* compute new xofs/yofs values */
	if (have_anchor) {
		x_rel = (double) anchorx / allocation.width;
		y_rel = (double) anchory / allocation.height;
	} else {
		x_rel = 0.5;
		y_rel = 0.5;
	}

	compute_center_zoom_offsets (view, priv->zoom, zoom,
				     allocation.width, allocation.height,
				     x_rel, y_rel,
				     &xofs, &yofs);

	/* set new values */
	priv->xofs = xofs; /* (img_width * x_rel * zoom) - anchorx; */
	priv->yofs = yofs; /* (img_height * y_rel * zoom) - anchory; */
#if 0
	g_print ("xofs: %i  yofs: %i\n", priv->xofs, priv->yofs);
#endif
	if (zoom <= priv->min_zoom)
		priv->zoom = priv->min_zoom;
	else
		priv->zoom = zoom;

	/* we make use of the new values here */
	check_scrollbar_visibility (view, NULL);
	update_scrollbar_values (view);

	/* repaint the whole image */
	gtk_widget_queue_draw (GTK_WIDGET (priv->display));

	g_signal_emit (view, view_signals [SIGNAL_ZOOM_CHANGED], 0, priv->zoom);
}

/* Zooms the image to fit the available allocation */
static void
set_zoom_fit (IogScrollView *view)
{
	IogScrollViewPrivate *priv;
	GtkAllocation allocation;
	double new_zoom;

	priv = view->priv;

	priv->zoom_mode = ZOOM_MODE_FIT;

	if (!gtk_widget_get_mapped (GTK_WIDGET (view)))
		return;

	if (priv->pixbuf == NULL)
		return;

	gtk_widget_get_allocation (GTK_WIDGET(priv->display), &allocation);

	new_zoom = zoom_fit_scale (allocation.width, allocation.height,
				   gdk_pixbuf_get_width (priv->pixbuf),
				   gdk_pixbuf_get_height (priv->pixbuf),
				   priv->upscale);

	if (new_zoom > MAX_ZOOM_FACTOR)
		new_zoom = MAX_ZOOM_FACTOR;
	else if (new_zoom < MIN_ZOOM_FACTOR)
		new_zoom = MIN_ZOOM_FACTOR;

	priv->zoom = new_zoom;
	priv->xofs = 0;
	priv->yofs = 0;

	g_signal_emit (view, view_signals [SIGNAL_ZOOM_CHANGED], 0, priv->zoom);
}

/*===================================

   internal signal callbacks

  ---------------------------------*/

/* Key press event handler for the image view */
static gboolean
display_key_press_event (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	IogScrollView *view;
	IogScrollViewPrivate *priv;
	GtkAllocation allocation;
	gboolean do_zoom;
	double zoom;
	gboolean do_scroll;
	int xofs, yofs;

	view = IOG_SCROLL_VIEW (data);
	priv = view->priv;

	do_zoom = FALSE;
	do_scroll = FALSE;
	xofs = yofs = 0;
	zoom = 1.0;

	gtk_widget_get_allocation (GTK_WIDGET (priv->display), &allocation);

	/* IogScrollView doesn't handle/have any Alt+Key combos */
	if (event->state & GDK_MOD1_MASK) {
		return FALSE;
	}

	switch (event->keyval) {
	case GDK_KEY_Up:
		do_scroll = TRUE;
		xofs = 0;
		yofs = -SCROLL_STEP_SIZE;
		break;

	case GDK_KEY_Page_Up:
		do_scroll = TRUE;
		if (event->state & GDK_CONTROL_MASK) {
			xofs = -(allocation.width * 3) / 4;
			yofs = 0;
		} else {
			xofs = 0;
			yofs = -(allocation.height * 3) / 4;
		}
		break;

	case GDK_KEY_Down:
		do_scroll = TRUE;
		xofs = 0;
		yofs = SCROLL_STEP_SIZE;
		break;

	case GDK_KEY_Page_Down:
		do_scroll = TRUE;
		if (event->state & GDK_CONTROL_MASK) {
			xofs = (allocation.width * 3) / 4;
			yofs = 0;
		} else {
			xofs = 0;
			yofs = (allocation.height * 3) / 4;
		}
		break;

	case GDK_KEY_Left:
		do_scroll = TRUE;
		xofs = -SCROLL_STEP_SIZE;
		yofs = 0;
		break;

	case GDK_KEY_Right:
		do_scroll = TRUE;
		xofs = SCROLL_STEP_SIZE;
		yofs = 0;
		break;

	case GDK_KEY_plus:
	case GDK_KEY_equal:
	case GDK_KEY_KP_Add:
		do_zoom = TRUE;
		zoom = priv->zoom * priv->zoom_multiplier;
		break;

	case GDK_KEY_minus:
	case GDK_KEY_KP_Subtract:
		do_zoom = TRUE;
		zoom = priv->zoom / priv->zoom_multiplier;
		break;

	case GDK_KEY_1:
		do_zoom = TRUE;
		zoom = 1.0;
		break;

	default:
		return FALSE;
	}

	if (do_zoom) {
#if GTK_CHECK_VERSION (3, 0, 0)
		GdkDeviceManager *device_manager;
		GdkDevice *device;
#endif
		gint x, y;

#if GTK_CHECK_VERSION (3, 0, 0)
		device_manager = gdk_display_get_device_manager (gtk_widget_get_display(widget));
		device = gdk_device_manager_get_client_pointer (device_manager);

		gdk_window_get_device_position (gtk_widget_get_window (widget), device,
#else
		gdk_window_get_pointer (gtk_widget_get_window (widget),
#endif
					&x, &y, NULL);
		set_zoom (view, zoom, TRUE, x, y);
	}

	if (do_scroll)
		scroll_by (view, xofs, yofs);

	return TRUE;
}


/* Button press event handler for the image view */
static gboolean
iog_scroll_view_button_press_event (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	IogScrollView *view;
	IogScrollViewPrivate *priv;

	view = IOG_SCROLL_VIEW (data);
	priv = view->priv;

	if (!gtk_widget_has_focus (priv->display))
		gtk_widget_grab_focus (GTK_WIDGET (priv->display));

	if (priv->dragging)
		return FALSE;

	switch (event->button) {
		case 1:
		case 2:
                        if (event->button == 1 && !priv->scroll_wheel_zoom &&
			    !(event->state & GDK_CONTROL_MASK))
				break;

			if (is_image_movable (view)) {
				iog_scroll_view_set_cursor (view, IOG_SCROLL_VIEW_CURSOR_DRAG);

				priv->dragging = TRUE;
				priv->drag_anchor_x = event->x;
				priv->drag_anchor_y = event->y;

				priv->drag_ofs_x = priv->xofs;
				priv->drag_ofs_y = priv->yofs;

				return TRUE;
			}
		default:
			break;
	}

	return FALSE;
}

static void
iog_scroll_view_style_set (GtkWidget *widget, GtkStyle *old_style)
{
	GtkStyle *style;
	IogScrollViewPrivate *priv;

	style = gtk_widget_get_style (widget);
	priv = IOG_SCROLL_VIEW (widget)->priv;

	gtk_widget_set_style (priv->display, style);
}


/* Button release event handler for the image view */
static gboolean
iog_scroll_view_button_release_event (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	IogScrollView *view;
	IogScrollViewPrivate *priv;

	view = IOG_SCROLL_VIEW (data);
	priv = view->priv;

	if (!priv->dragging)
		return FALSE;

	switch (event->button) {
		case 1:
		case 2:
			drag_to (view, event->x, event->y);
			priv->dragging = FALSE;

			iog_scroll_view_set_cursor (view, IOG_SCROLL_VIEW_CURSOR_NORMAL);
			break;

		default:
			break;
	}

	return TRUE;
}

/* Scroll event handler for the image view.  We zoom with an event without
 * modifiers rather than scroll; we use the Shift modifier to scroll.
 * Rationale: images are not primarily vertical, and in IOG you scan scroll by
 * dragging the image with button 1 anyways.
 */
static gboolean
iog_scroll_view_scroll_event (GtkWidget *widget, GdkEventScroll *event, gpointer data)
{
	IogScrollView *view;
	IogScrollViewPrivate *priv;
	double zoom_factor;
	int xofs, yofs;

	view = IOG_SCROLL_VIEW (data);
	priv = view->priv;

	/* Compute zoom factor and scrolling offsets; we'll only use either of them */
	/* same as in gtkscrolledwindow.c */
	xofs = gtk_adjustment_get_page_increment (priv->hadj) / 2;
	yofs = gtk_adjustment_get_page_increment (priv->vadj) / 2;

	switch (event->direction) {
	case GDK_SCROLL_UP:
		zoom_factor = priv->zoom_multiplier;
		xofs = 0;
		yofs = -yofs;
		break;

	case GDK_SCROLL_LEFT:
		zoom_factor = 1.0 / priv->zoom_multiplier;
		xofs = -xofs;
		yofs = 0;
		break;

	case GDK_SCROLL_DOWN:
		zoom_factor = 1.0 / priv->zoom_multiplier;
		xofs = 0;
		yofs = yofs;
		break;

	case GDK_SCROLL_RIGHT:
		zoom_factor = priv->zoom_multiplier;
		xofs = xofs;
		yofs = 0;
		break;

	default:
		g_assert_not_reached ();
		return FALSE;
	}

        if (priv->scroll_wheel_zoom) {
		if (event->state & GDK_SHIFT_MASK)
			scroll_by (view, yofs, xofs);
		else if (event->state & GDK_CONTROL_MASK)
			scroll_by (view, xofs, yofs);
		else
			set_zoom (view, priv->zoom * zoom_factor,
				  TRUE, event->x, event->y);
	} else {
		if (event->state & GDK_SHIFT_MASK)
			scroll_by (view, yofs, xofs);
		else if (event->state & GDK_CONTROL_MASK)
			set_zoom (view, priv->zoom * zoom_factor,
				  TRUE, event->x, event->y);
		else
			scroll_by (view, xofs, yofs);
        }

	return TRUE;
}

/* Motion event handler for the image view */
static gboolean
iog_scroll_view_motion_event (GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
	IogScrollView *view;
	IogScrollViewPrivate *priv;
	gint x, y;
	GdkModifierType mods;

	view = IOG_SCROLL_VIEW (data);
	priv = view->priv;

	if (!priv->dragging)
		return FALSE;

	if (event->is_hint)
#if GTK_CHECK_VERSION (3, 0, 0)
		gdk_window_get_device_position (gtk_widget_get_window (GTK_WIDGET (priv->display)), event->device, &x, &y, &mods);
#else
		gdk_window_get_pointer (gtk_widget_get_window (GTK_WIDGET (priv->display)), &x, &y, &mods);
#endif
	else {
		x = event->x;
		y = event->y;
	}

	drag_to (view, x, y);
	return TRUE;
}

static void
display_map_event (GtkWidget *widget, GdkEvent *event, gpointer data)
{
	IogScrollView *view;
	IogScrollViewPrivate *priv;

	view = IOG_SCROLL_VIEW (data);
	priv = view->priv;

	iog_debug (DEBUG_WINDOW);

	set_zoom_fit (view);
	check_scrollbar_visibility (view, NULL);
	gtk_widget_queue_draw (GTK_WIDGET (priv->display));
}

static void
iog_scroll_view_size_allocate (GtkWidget *widget, GtkAllocation *alloc)
{
	IogScrollView *view;

	view = IOG_SCROLL_VIEW (widget);
	check_scrollbar_visibility (view, alloc);

	GTK_WIDGET_CLASS (iog_scroll_view_parent_class)->size_allocate (widget
									,alloc);
}

static void
display_size_change (GtkWidget *widget, GdkEventConfigure *event, gpointer data)
{
	IogScrollView *view;
	IogScrollViewPrivate *priv;

	view = IOG_SCROLL_VIEW (data);
	priv = view->priv;

	if (priv->zoom_mode == ZOOM_MODE_FIT) {
		GtkAllocation alloc;

		alloc.width = event->width;
		alloc.height = event->height;

		set_zoom_fit (view);
		check_scrollbar_visibility (view, &alloc);
		gtk_widget_queue_draw (GTK_WIDGET (priv->display));
	} else {
		int scaled_width, scaled_height;
		int x_offset = 0;
		int y_offset = 0;

		compute_scaled_size (view, priv->zoom, &scaled_width, &scaled_height);

		if (priv->xofs + event->width > scaled_width)
			x_offset = scaled_width - event->width - priv->xofs;

		if (priv->yofs + event->height > scaled_height)
			y_offset = scaled_height - event->height - priv->yofs;

		scroll_by (view, x_offset, y_offset);
	}

	update_scrollbar_values (view);
}


static gboolean
iog_scroll_view_focus_in_event (GtkWidget     *widget,
			    GdkEventFocus *event,
			    gpointer data)
{
	g_signal_stop_emission_by_name (G_OBJECT (widget), "focus_in_event");
	return FALSE;
}

static gboolean
iog_scroll_view_focus_out_event (GtkWidget     *widget,
			     GdkEventFocus *event,
			     gpointer data)
{
	g_signal_stop_emission_by_name (G_OBJECT (widget), "focus_out_event");
	return FALSE;
}

static gboolean
#if GTK_CHECK_VERSION (3, 0, 0)
display_draw (GtkWidget *widget, cairo_t *cr, gpointer data)
#else
display_expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer data)
#endif
{
	IogScrollView *view;
	IogScrollViewPrivate *priv;
#if !GTK_CHECK_VERSION (3, 0, 0)
	cairo_t *cr;
#endif
	GtkAllocation allocation;
	int scaled_width, scaled_height;
	int xofs, yofs;

	g_return_val_if_fail (GTK_IS_DRAWING_AREA (widget), FALSE);
#if !GTK_CHECK_VERSION (3, 0, 0)
	g_return_val_if_fail (event != NULL, FALSE);
#endif
	g_return_val_if_fail (IOG_IS_SCROLL_VIEW (data), FALSE);

	view = IOG_SCROLL_VIEW (data);

	priv = view->priv;

	if (priv->pixbuf == NULL)
		return TRUE;

	compute_scaled_size (view, priv->zoom, &scaled_width, &scaled_height);

	gtk_widget_get_allocation (GTK_WIDGET (priv->display), &allocation);

	/* Compute image offsets with respect to the window */

	if (scaled_width <= allocation.width)
		xofs = (allocation.width - scaled_width) / 2;
	else
		xofs = -priv->xofs;

	if (scaled_height <= allocation.height)
		yofs = (allocation.height - scaled_height) / 2;
	else
		yofs = -priv->yofs;

	iog_debug_message (DEBUG_WINDOW, "zoom %.2f, xofs: %i, yofs: %i scaled w: %i h: %i\n",
	priv->zoom, xofs, yofs, scaled_width, scaled_height);

#if !GTK_CHECK_VERSION (3, 0, 0)
	cr = gdk_cairo_create (GDK_DRAWABLE (gtk_widget_get_window (GTK_WIDGET (view->priv->display))));
	gdk_cairo_region (cr, event->region);
	cairo_clip (cr);
#endif

	/* Paint the background */
	cairo_set_source (cr, gdk_window_get_background_pattern (gtk_widget_get_window (priv->display)));
	cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);
	cairo_rectangle (cr, MAX (0, xofs), MAX (0, yofs),
	scaled_width, scaled_height);
	cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);
	cairo_fill (cr);

	if (gdk_pixbuf_get_has_alpha (priv->pixbuf)) {
		if (priv->background_surface == NULL) {
			priv->background_surface = create_background_surface (view);
		}
		cairo_set_source_surface (cr, priv->background_surface, xofs, yofs);
		cairo_pattern_set_extend (cairo_get_source (cr), CAIRO_EXTEND_REPEAT);
		cairo_rectangle (cr, xofs, yofs, scaled_width, scaled_height);
		cairo_fill (cr);
	}

	/* Make sure the image is only drawn as large as needed.
	 * This is especially necessary for SVGs where there might
	 * be more image data available outside the image boundaries.
	 */
	cairo_rectangle (cr, xofs, yofs, scaled_width, scaled_height);
	cairo_clip (cr);

#ifdef HAVE_RSVG
	if (iog_image_is_svg (view->priv->image)) {
		cairo_matrix_t matrix, translate, scale, original;
		IogTransform *transform = iog_image_get_transform (priv->image);
		cairo_matrix_init_identity (&matrix);
		if (transform) {
			cairo_matrix_t affine;
			double image_offset_x = 0., image_offset_y = 0.;

			iog_transform_get_affine (transform, &affine);
			cairo_matrix_multiply (&matrix, &affine, &matrix);

			switch (iog_transform_get_transform_type (transform)) {
			case IOG_TRANSFORM_ROT_90:
			case IOG_TRANSFORM_FLIP_HORIZONTAL:
				image_offset_x = (double) gdk_pixbuf_get_width (priv->pixbuf);
				break;
			case IOG_TRANSFORM_ROT_270:
			case IOG_TRANSFORM_FLIP_VERTICAL:
				image_offset_y = (double) gdk_pixbuf_get_height (priv->pixbuf);
				break;
			case IOG_TRANSFORM_ROT_180:
			case IOG_TRANSFORM_TRANSPOSE:
			case IOG_TRANSFORM_TRANSVERSE:
				image_offset_x = (double) gdk_pixbuf_get_width (priv->pixbuf);
				image_offset_y = (double) gdk_pixbuf_get_height (priv->pixbuf);
				break;
			case IOG_TRANSFORM_NONE:
				default:
				break;
			}
			cairo_matrix_init_translate (&translate, image_offset_x, image_offset_y);
			cairo_matrix_multiply (&matrix, &matrix, &translate);
		}
		cairo_matrix_init_scale (&scale, priv->zoom, priv->zoom);
		cairo_matrix_multiply (&matrix, &matrix, &scale);
		cairo_matrix_init_translate (&translate, xofs, yofs);
		cairo_matrix_multiply (&matrix, &matrix, &translate);

		cairo_get_matrix (cr, &original);
		cairo_matrix_multiply (&matrix, &matrix, &original);
		cairo_set_matrix (cr, &matrix);

		rsvg_handle_render_cairo (iog_image_get_svg (priv->image), cr);

	} else
#endif /* HAVE_RSVG */
	{
		cairo_scale (cr, priv->zoom, priv->zoom);
		cairo_set_source_surface (cr, priv->surface, xofs/priv->zoom, yofs/priv->zoom);
		cairo_pattern_set_extend (cairo_get_source (cr), CAIRO_EXTEND_PAD);
		if (is_zoomed_in (view))
			cairo_pattern_set_filter (cairo_get_source (cr), priv->interp_type_in);
		else if (is_zoomed_out (view))
			cairo_pattern_set_filter (cairo_get_source (cr), priv->interp_type_out);
		cairo_paint (cr);
	}

#if !GTK_CHECK_VERSION (3, 0, 0)
	cairo_destroy (cr);
#endif
	return TRUE;
}


/*==================================

   image loading callbacks

   -----------------------------------*/

/* Use when the pixbuf in the view is changed, to keep a
   reference to it and create its cairo surface. */
static void
update_pixbuf (IogScrollView *view, GdkPixbuf *pixbuf)
{
	IogScrollViewPrivate *priv;

	priv = view->priv;

	if (priv->pixbuf != NULL) {
		g_object_unref (priv->pixbuf);
		priv->pixbuf = NULL;
	}

	priv->pixbuf = pixbuf;

	if (priv->surface) {
		cairo_surface_destroy (priv->surface);
	}
	priv->surface = create_surface_from_pixbuf (view, priv->pixbuf);
}

static void
image_changed_cb (IogImage *img, gpointer data)
{
	IogScrollViewPrivate *priv;

	priv = IOG_SCROLL_VIEW (data)->priv;

	update_pixbuf (IOG_SCROLL_VIEW (data), iog_image_get_pixbuf (img));

	set_zoom_fit (IOG_SCROLL_VIEW (data));
	check_scrollbar_visibility (IOG_SCROLL_VIEW (data), NULL);

	gtk_widget_queue_draw (GTK_WIDGET (priv->display));
}

/*===================================
         public API
  ---------------------------------*/

void
iog_scroll_view_hide_cursor (IogScrollView *view)
{
       iog_scroll_view_set_cursor (view, IOG_SCROLL_VIEW_CURSOR_HIDDEN);
}

void
iog_scroll_view_show_cursor (IogScrollView *view)
{
       iog_scroll_view_set_cursor (view, IOG_SCROLL_VIEW_CURSOR_NORMAL);
}

/* general properties */
void
iog_scroll_view_set_zoom_upscale (IogScrollView *view, gboolean upscale)
{
	IogScrollViewPrivate *priv;

	g_return_if_fail (IOG_IS_SCROLL_VIEW (view));

	priv = view->priv;

	if (priv->upscale != upscale) {
		priv->upscale = upscale;

		if (priv->zoom_mode == ZOOM_MODE_FIT) {
			set_zoom_fit (view);
			gtk_widget_queue_draw (GTK_WIDGET (priv->display));
		}
	}
}

void
iog_scroll_view_set_antialiasing_in (IogScrollView *view, gboolean state)
{
	IogScrollViewPrivate *priv;
	cairo_filter_t new_interp_type;

	g_return_if_fail (IOG_IS_SCROLL_VIEW (view));

	priv = view->priv;

	new_interp_type = state ? CAIRO_FILTER_BILINEAR : CAIRO_FILTER_NEAREST;

	if (priv->interp_type_in != new_interp_type) {
		priv->interp_type_in = new_interp_type;
		gtk_widget_queue_draw (GTK_WIDGET (priv->display));
		g_object_notify (G_OBJECT (view), "antialiasing-in");
	}
}

void
iog_scroll_view_set_antialiasing_out (IogScrollView *view, gboolean state)
{
	IogScrollViewPrivate *priv;
	cairo_filter_t new_interp_type;

	g_return_if_fail (IOG_IS_SCROLL_VIEW (view));

	priv = view->priv;

	new_interp_type = state ? CAIRO_FILTER_BILINEAR : CAIRO_FILTER_NEAREST;

	if (priv->interp_type_out != new_interp_type) {
		priv->interp_type_out = new_interp_type;
		gtk_widget_queue_draw (GTK_WIDGET (priv->display));
		g_object_notify (G_OBJECT (view), "antialiasing-out");

	}
}

static void
_transp_background_changed (IogScrollView *view)
{
	IogScrollViewPrivate *priv = view->priv;

	if (priv->pixbuf != NULL && gdk_pixbuf_get_has_alpha (priv->pixbuf)) {
		if (priv->background_surface) {
			cairo_surface_destroy (priv->background_surface);
			/* Will be recreated if needed during redraw */
			priv->background_surface = NULL;
		}
		gtk_widget_queue_draw (GTK_WIDGET (priv->display));
	}

}

void
iog_scroll_view_set_transparency_color (IogScrollView *view, GdkColor *color)
{
	guint32 col = 0;
	guint32 red, green, blue;
	IogScrollViewPrivate *priv;

	g_return_if_fail (IOG_IS_SCROLL_VIEW (view));

	priv = view->priv;

	if (color != NULL) {
		red = (color->red >> 8) << 16;
		green = (color->green >> 8) << 8;
		blue = (color->blue >> 8);
		col = red + green + blue;
	}

	if (priv->transp_style != col) {
		priv->transp_color = col;
		if (priv->transp_style == IOG_TRANSP_COLOR)
		    _transp_background_changed (view);

		g_object_notify (G_OBJECT (view), "transparency-color");
	}
}

void
iog_scroll_view_set_transparency (IogScrollView        *view,
				  IogTransparencyStyle  style)
{
	IogScrollViewPrivate *priv;

	g_return_if_fail (IOG_IS_SCROLL_VIEW (view));

	priv = view->priv;

	if (priv->transp_style != style) {
		priv->transp_style = style;
		_transp_background_changed (view);
		g_object_notify (G_OBJECT (view), "transparency-style");
	}
}

/* zoom api */

static double preferred_zoom_levels[] = {
	1.0 / 100, 1.0 / 50, 1.0 / 20,
	1.0 / 10.0, 1.0 / 5.0, 1.0 / 3.0, 1.0 / 2.0, 1.0 / 1.5,
        1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0,
        11.0, 12.0, 13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0, 20.0
};
static const gint n_zoom_levels = (sizeof (preferred_zoom_levels) / sizeof (double));

void
iog_scroll_view_zoom_in (IogScrollView *view, gboolean smooth)
{
	IogScrollViewPrivate *priv;
	double zoom;

	g_return_if_fail (IOG_IS_SCROLL_VIEW (view));

	priv = view->priv;

	if (smooth) {
		zoom = priv->zoom * priv->zoom_multiplier;
	}
	else {
		int i;
		int index = -1;

		for (i = 0; i < n_zoom_levels; i++) {
			if (preferred_zoom_levels [i] - priv->zoom
					> DOUBLE_EQUAL_MAX_DIFF) {
				index = i;
				break;
			}
		}

		if (index == -1) {
			zoom = priv->zoom;
		}
		else {
			zoom = preferred_zoom_levels [i];
		}
	}
	set_zoom (view, zoom, FALSE, 0, 0);

}

void
iog_scroll_view_zoom_out (IogScrollView *view, gboolean smooth)
{
	IogScrollViewPrivate *priv;
	double zoom;

	g_return_if_fail (IOG_IS_SCROLL_VIEW (view));

	priv = view->priv;

	if (smooth) {
		zoom = priv->zoom / priv->zoom_multiplier;
	}
	else {
		int i;
		int index = -1;

		for (i = n_zoom_levels - 1; i >= 0; i--) {
			if (priv->zoom - preferred_zoom_levels [i]
					> DOUBLE_EQUAL_MAX_DIFF) {
				index = i;
				break;
			}
		}
		if (index == -1) {
			zoom = priv->zoom;
		}
		else {
			zoom = preferred_zoom_levels [i];
		}
	}
	set_zoom (view, zoom, FALSE, 0, 0);
}

void
iog_scroll_view_zoom_fit (IogScrollView *view)
{
	g_return_if_fail (IOG_IS_SCROLL_VIEW (view));

	set_zoom_fit (view);
	check_scrollbar_visibility (view, NULL);
	gtk_widget_queue_draw (GTK_WIDGET (view->priv->display));
}

void
iog_scroll_view_set_zoom (IogScrollView *view, double zoom)
{
	g_return_if_fail (IOG_IS_SCROLL_VIEW (view));

	set_zoom (view, zoom, FALSE, 0, 0);
}

double
iog_scroll_view_get_zoom (IogScrollView *view)
{
	g_return_val_if_fail (IOG_IS_SCROLL_VIEW (view), 0.0);

	return view->priv->zoom;
}

gboolean
iog_scroll_view_get_zoom_is_min (IogScrollView *view)
{
	g_return_val_if_fail (IOG_IS_SCROLL_VIEW (view), FALSE);

	set_minimum_zoom_factor (view);

	return DOUBLE_EQUAL (view->priv->zoom, MIN_ZOOM_FACTOR) ||
	       DOUBLE_EQUAL (view->priv->zoom, view->priv->min_zoom);
}

gboolean
iog_scroll_view_get_zoom_is_max (IogScrollView *view)
{
	g_return_val_if_fail (IOG_IS_SCROLL_VIEW (view), FALSE);

	return DOUBLE_EQUAL (view->priv->zoom, MAX_ZOOM_FACTOR);
}

static void
display_next_frame_cb (IogImage *image, gint delay, gpointer data)
{
 	IogScrollViewPrivate *priv;
	IogScrollView *view;

	if (!IOG_IS_SCROLL_VIEW (data))
		return;

	view = IOG_SCROLL_VIEW (data);
	priv = view->priv;

	update_pixbuf (view, iog_image_get_pixbuf (image));
	gtk_widget_queue_draw (GTK_WIDGET (priv->display)); 
}

void
iog_scroll_view_set_image (IogScrollView *view, IogImage *image)
{
	IogScrollViewPrivate *priv;

	g_return_if_fail (IOG_IS_SCROLL_VIEW (view));

	priv = view->priv;

	if (priv->image == image) {
		return;
	}

	if (priv->image != NULL) {
		free_image_resources (view);
		#if !GTK_CHECK_VERSION(3, 0, 0)
		if (gtk_widget_is_drawable (priv->display) && image == NULL) {
			gdk_window_clear (gtk_widget_get_window (priv->display));
		}
		#endif
	}
	g_assert (priv->image == NULL);
	g_assert (priv->pixbuf == NULL);

	if (image != NULL) {
		iog_image_data_ref (image);

		if (priv->pixbuf == NULL) {
			update_pixbuf (view, iog_image_get_pixbuf (image));
			set_zoom_fit (view);
			check_scrollbar_visibility (view, NULL);
			gtk_widget_queue_draw (GTK_WIDGET (priv->display));

		}

		priv->image_changed_id = g_signal_connect (image, "changed",
							   (GCallback) image_changed_cb, view);
		if (iog_image_is_animation (image) == TRUE ) {
			iog_image_start_animation (image);
			priv->frame_changed_id = g_signal_connect (image, "next-frame", 
								    (GCallback) display_next_frame_cb, view);
		}
	}

	priv->image = image;
}

gboolean
iog_scroll_view_scrollbars_visible (IogScrollView *view)
{
	if (!gtk_widget_get_visible (GTK_WIDGET (view->priv->hbar)) &&
	    !gtk_widget_get_visible (GTK_WIDGET (view->priv->vbar)))
		return FALSE;

	return TRUE;
}

/*===================================
    object creation/freeing
  ---------------------------------*/

static gboolean
sv_string_to_color_mapping (GValue   *value,
			    GVariant *variant,
			    gpointer  user_data)
{
	GdkColor color;

	g_return_val_if_fail (g_variant_is_of_type (variant, G_VARIANT_TYPE_STRING), FALSE);

	if (gdk_color_parse (g_variant_get_string (variant, NULL), &color)) {
		g_value_set_boxed (value, &color);
		return TRUE;
	}

	return FALSE;
}

static GVariant*
sv_color_to_string_mapping (const GValue       *value,
			    const GVariantType *expected_type,
			    gpointer            user_data)
{
	GVariant *variant = NULL;
	GdkColor *color;
	gchar *hex_val;

	g_return_val_if_fail (G_VALUE_TYPE (value) == GDK_TYPE_COLOR, NULL);
	g_return_val_if_fail (g_variant_type_equal (expected_type, G_VARIANT_TYPE_STRING), NULL);

	color = g_value_get_boxed (value);
	hex_val = g_strdup_printf ("#%02X%02X%02X",
				   color->red / 256,
				   color->green / 256,
				   color->blue / 256);
	variant = g_variant_new_string (hex_val);
	g_free (hex_val);

	return variant;
}

static void
iog_scroll_view_init (IogScrollView *view)
{
	GSettings *settings;
	IogScrollViewPrivate *priv;

	priv = view->priv = IOG_SCROLL_VIEW_GET_PRIVATE (view);
	settings = g_settings_new (IOG_CONF_VIEW);

	priv->zoom = 1.0;
	priv->min_zoom = MIN_ZOOM_FACTOR;
	priv->zoom_mode = ZOOM_MODE_FIT;
	priv->upscale = FALSE;
	priv->interp_type_in = CAIRO_FILTER_BILINEAR;
	priv->interp_type_out = CAIRO_FILTER_BILINEAR;
	priv->scroll_wheel_zoom = FALSE;
	priv->zoom_multiplier = IMAGE_VIEW_ZOOM_MULTIPLIER;
	priv->image = NULL;
	priv->pixbuf = NULL;
	priv->surface = NULL;
	priv->transp_style = IOG_TRANSP_BACKGROUND;
	priv->transp_color = 0;
	priv->cursor = IOG_SCROLL_VIEW_CURSOR_NORMAL;
	priv->menu = NULL;
	priv->background_color = NULL;

	priv->hadj = GTK_ADJUSTMENT (gtk_adjustment_new (0, 100, 0, 10, 10, 100));
	g_signal_connect (priv->hadj, "value_changed",
			  G_CALLBACK (adjustment_changed_cb),
			  view);

#if GTK_CHECK_VERSION (3, 0, 0)
	priv->hbar = gtk_scrollbar_new (GTK_ORIENTATION_HORIZONTAL, priv->hadj);
#else
	priv->hbar = gtk_hscrollbar_new (priv->hadj);
#endif
	priv->vadj = GTK_ADJUSTMENT (gtk_adjustment_new (0, 100, 0, 10, 10, 100));
	g_signal_connect (priv->vadj, "value_changed",
			  G_CALLBACK (adjustment_changed_cb),
			  view);

#if GTK_CHECK_VERSION (3, 2, 0)
	priv->vbar = gtk_scrollbar_new (GTK_ORIENTATION_VERTICAL, priv->vadj);
#else
	priv->vbar = gtk_vscrollbar_new (priv->vadj);
#endif
	priv->display = g_object_new (GTK_TYPE_DRAWING_AREA,
				      "can-focus", TRUE,
				      NULL);

	gtk_widget_add_events (GTK_WIDGET (priv->display),
			       GDK_EXPOSURE_MASK
			       | GDK_BUTTON_PRESS_MASK
			       | GDK_BUTTON_RELEASE_MASK
			       | GDK_POINTER_MOTION_MASK
			       | GDK_POINTER_MOTION_HINT_MASK
			       | GDK_SCROLL_MASK
			       | GDK_KEY_PRESS_MASK);
	g_signal_connect (G_OBJECT (priv->display), "configure_event",
			  G_CALLBACK (display_size_change), view);
#if GTK_CHECK_VERSION (3, 0, 0)
	g_signal_connect (G_OBJECT (priv->display), "draw", G_CALLBACK (display_draw), view);
#else
	g_signal_connect (G_OBJECT (priv->display), "expose_event",
			  G_CALLBACK (display_expose_event), view);
#endif
	g_signal_connect (G_OBJECT (priv->display), "map_event",
			  G_CALLBACK (display_map_event), view);
	g_signal_connect (G_OBJECT (priv->display), "button_press_event",
			  G_CALLBACK (iog_scroll_view_button_press_event),
			  view);
	g_signal_connect (G_OBJECT (priv->display), "motion_notify_event",
			  G_CALLBACK (iog_scroll_view_motion_event), view);
	g_signal_connect (G_OBJECT (priv->display), "button_release_event",
			  G_CALLBACK (iog_scroll_view_button_release_event),
			  view);
	g_signal_connect (G_OBJECT (priv->display), "scroll_event",
			  G_CALLBACK (iog_scroll_view_scroll_event), view);
	g_signal_connect (G_OBJECT (priv->display), "focus_in_event",
			  G_CALLBACK (iog_scroll_view_focus_in_event), NULL);
	g_signal_connect (G_OBJECT (priv->display), "focus_out_event",
			  G_CALLBACK (iog_scroll_view_focus_out_event), NULL);

	g_signal_connect (G_OBJECT (view), "key_press_event",
			  G_CALLBACK (display_key_press_event), view);

	gtk_drag_source_set (priv->display, GDK_BUTTON1_MASK,
			     target_table, G_N_ELEMENTS (target_table),
			     GDK_ACTION_COPY);
	g_signal_connect (G_OBJECT (priv->display), "drag-data-get",
			  G_CALLBACK (view_on_drag_data_get_cb), view);
	g_signal_connect (G_OBJECT (priv->display), "drag-begin",
			  G_CALLBACK (view_on_drag_begin_cb), view);

#if GTK_CHECK_VERSION (3, 4, 0)
	gtk_grid_attach (GTK_GRID (view), priv->display,
					 0, 0, 1, 1);
	gtk_widget_set_hexpand (priv->display, TRUE);
	gtk_widget_set_vexpand (priv->display, TRUE);
	gtk_grid_attach (GTK_GRID (view), priv->hbar,
					 0, 1, 1, 1);
	gtk_widget_set_hexpand (priv->hbar, TRUE);
	gtk_grid_attach (GTK_GRID (view), priv->vbar,
					 1, 0, 1, 1);
	gtk_widget_set_vexpand (priv->vbar, TRUE);
#else
	gtk_table_attach (GTK_TABLE (view), priv->display,
			  0, 1, 0, 1,
			  GTK_EXPAND | GTK_FILL,
			  GTK_EXPAND | GTK_FILL,
			  0,0);
	gtk_table_attach (GTK_TABLE (view), priv->hbar,
			  0, 1, 1, 2,
			  GTK_FILL,
			  GTK_FILL,
			  0, 0);
	gtk_table_attach (GTK_TABLE (view), priv->vbar,
			  1, 2, 0, 1,
			  GTK_FILL, GTK_FILL,
			  0, 0);
#endif

	g_settings_bind (settings, IOG_CONF_VIEW_USE_BG_COLOR, view,
			 "use-background-color", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind_with_mapping (settings, IOG_CONF_VIEW_BACKGROUND_COLOR,
				      view, "background-color",
				      G_SETTINGS_BIND_DEFAULT,
				      sv_string_to_color_mapping,
				      sv_color_to_string_mapping, NULL, NULL);
	g_settings_bind (settings, IOG_CONF_VIEW_EXTRAPOLATE, view,
			 "antialiasing-in", G_SETTINGS_BIND_GET);
	g_settings_bind (settings, IOG_CONF_VIEW_INTERPOLATE, view,
			 "antialiasing-out", G_SETTINGS_BIND_GET);
	g_settings_bind_with_mapping (settings, IOG_CONF_VIEW_TRANS_COLOR,
				      view, "transparency-color",
				      G_SETTINGS_BIND_GET,
				      sv_string_to_color_mapping,
				      sv_color_to_string_mapping, NULL, NULL);
	g_settings_bind (settings, IOG_CONF_VIEW_TRANSPARENCY, view,
			 "transparency-style", G_SETTINGS_BIND_GET);

	g_object_unref (settings);

	priv->override_bg_color = NULL;
	priv->background_surface = NULL;
}

static void
iog_scroll_view_dispose (GObject *object)
{
	IogScrollView *view;
	IogScrollViewPrivate *priv;

	g_return_if_fail (IOG_IS_SCROLL_VIEW (object));

	view = IOG_SCROLL_VIEW (object);
	priv = view->priv;

	if (priv->idle_id != 0) {
		g_source_remove (priv->idle_id);
		priv->idle_id = 0;
	}

	if (priv->background_color != NULL) {
		gdk_color_free (priv->background_color);
		priv->background_color = NULL;
	}

	if (priv->override_bg_color != NULL) {
		gdk_color_free (priv->override_bg_color);
		priv->override_bg_color = NULL;
	}

	if (priv->background_surface != NULL) {
		cairo_surface_destroy (priv->background_surface);
		priv->background_surface = NULL;
	}

	free_image_resources (view);

	G_OBJECT_CLASS (iog_scroll_view_parent_class)->dispose (object);
}

static void
iog_scroll_view_get_property (GObject *object, guint property_id,
			      GValue *value, GParamSpec *pspec)
{
	IogScrollView *view;
	IogScrollViewPrivate *priv;

	g_return_if_fail (IOG_IS_SCROLL_VIEW (object));

	view = IOG_SCROLL_VIEW (object);
	priv = view->priv;

	switch (property_id) {
	case PROP_ANTIALIAS_IN:
	{
		gboolean filter = (priv->interp_type_in != CAIRO_FILTER_NEAREST);
		g_value_set_boolean (value, filter);
		break;
	}
	case PROP_ANTIALIAS_OUT:
	{
		gboolean filter = (priv->interp_type_out != CAIRO_FILTER_NEAREST);
		g_value_set_boolean (value, filter);
		break;
	}
	case PROP_USE_BG_COLOR:
		g_value_set_boolean (value, priv->use_bg_color);
		break;
	case PROP_BACKGROUND_COLOR:
		//FIXME: This doesn't really handle the NULL color.
		g_value_set_boxed (value, priv->background_color);
		break;
	case PROP_SCROLLWHEEL_ZOOM:
		g_value_set_boolean (value, priv->scroll_wheel_zoom);
		break;
	case PROP_TRANSPARENCY_STYLE:
		g_value_set_enum (value, priv->transp_style);
		break;
	case PROP_ZOOM_MULTIPLIER:
		g_value_set_double (value, priv->zoom_multiplier);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
iog_scroll_view_set_property (GObject *object, guint property_id,
			      const GValue *value, GParamSpec *pspec)
{
	IogScrollView *view;

	g_return_if_fail (IOG_IS_SCROLL_VIEW (object));

	view = IOG_SCROLL_VIEW (object);

	switch (property_id) {
	case PROP_ANTIALIAS_IN:
		iog_scroll_view_set_antialiasing_in (view, g_value_get_boolean (value));
		break;
	case PROP_ANTIALIAS_OUT:
		iog_scroll_view_set_antialiasing_out (view, g_value_get_boolean (value));
		break;
	case PROP_USE_BG_COLOR:
		iog_scroll_view_set_use_bg_color (view, g_value_get_boolean (value));
		break;
	case PROP_BACKGROUND_COLOR:
	{
		const GdkColor *color = g_value_get_boxed (value);
		iog_scroll_view_set_background_color (view, color);
		break;
	}
	case PROP_SCROLLWHEEL_ZOOM:
		iog_scroll_view_set_scroll_wheel_zoom (view, g_value_get_boolean (value));
		break;
	case PROP_TRANSP_COLOR:
		iog_scroll_view_set_transparency_color (view, g_value_get_boxed (value));
		break;
	case PROP_TRANSPARENCY_STYLE:
		iog_scroll_view_set_transparency (view, g_value_get_enum (value));
		break;
	case PROP_ZOOM_MULTIPLIER:
		iog_scroll_view_set_zoom_multiplier (view, g_value_get_double (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}


static void
iog_scroll_view_class_init (IogScrollViewClass *klass)
{
	GObjectClass *gobject_class;
	GtkWidgetClass *widget_class;

	gobject_class = (GObjectClass*) klass;
	widget_class = (GtkWidgetClass*) klass;

	gobject_class->dispose = iog_scroll_view_dispose;
        gobject_class->set_property = iog_scroll_view_set_property;
        gobject_class->get_property = iog_scroll_view_get_property;

	g_object_class_install_property (
		gobject_class, PROP_ANTIALIAS_IN,
		g_param_spec_boolean ("antialiasing-in", NULL, NULL, TRUE,
				      G_PARAM_READWRITE | G_PARAM_STATIC_NAME));

	g_object_class_install_property (
		gobject_class, PROP_ANTIALIAS_OUT,
		g_param_spec_boolean ("antialiasing-out", NULL, NULL, TRUE,
				      G_PARAM_READWRITE | G_PARAM_STATIC_NAME));

	/**
	 * IogScrollView:background-color:
	 *
	 * This is the default background color used for painting the background
	 * of the image view. If set to %NULL the color is determined by the
	 * active GTK theme.
	 */
	g_object_class_install_property (
		gobject_class, PROP_BACKGROUND_COLOR,
		g_param_spec_boxed ("background-color", NULL, NULL,
				    GDK_TYPE_COLOR,
				    G_PARAM_READWRITE | G_PARAM_STATIC_NAME));

	g_object_class_install_property (
		gobject_class, PROP_USE_BG_COLOR,
		g_param_spec_boolean ("use-background-color", NULL, NULL, FALSE,
				      G_PARAM_READWRITE | G_PARAM_STATIC_NAME));

	/**
	 * IogScrollView:zoom-multiplier:
	 *
	 * The current zoom factor is multiplied with this value + 1.0 when
	 * scrolling with the scrollwheel to determine the next zoom factor.
	 */
	g_object_class_install_property (
		gobject_class, PROP_ZOOM_MULTIPLIER,
		g_param_spec_double ("zoom-multiplier", NULL, NULL,
				     -G_MAXDOUBLE, G_MAXDOUBLE -1.0, 0.05,
				     G_PARAM_READWRITE | G_PARAM_STATIC_NAME));

	/**
	 * IogScrollView:scrollwheel-zoom:
	 *
	 * If %TRUE the scrollwheel will zoom the view, otherwise it will be
	 * used for scrolling a zoomed image.
	 */
	g_object_class_install_property (
		gobject_class, PROP_SCROLLWHEEL_ZOOM,
		g_param_spec_boolean ("scrollwheel-zoom", NULL, NULL, TRUE,
		G_PARAM_READWRITE | G_PARAM_STATIC_NAME));

	/**
	 * IogScrollView:transparency-color:
	 *
	 * This is the color used to fill the transparent parts of an image
	 * if :transparency-style is set to use a custom color.
	 */
	g_object_class_install_property (
		gobject_class, PROP_TRANSP_COLOR,
		g_param_spec_boxed ("transparency-color", NULL, NULL,
				    GDK_TYPE_COLOR,
				    G_PARAM_WRITABLE | G_PARAM_STATIC_NAME));

	/**
	 * IogScrollView:transparency-style:
	 *
	 * Determines how to fill the shown image's transparent areas.
	 */
	g_object_class_install_property (
		gobject_class, PROP_TRANSPARENCY_STYLE,
		g_param_spec_enum ("transparency-style", NULL, NULL,
				   IOG_TYPE_TRANSPARENCY_STYLE,
				   IOG_TRANSP_CHECKED,
				   G_PARAM_READWRITE | G_PARAM_STATIC_NAME));

	view_signals [SIGNAL_ZOOM_CHANGED] =
		g_signal_new ("zoom_changed",
			      IOG_TYPE_SCROLL_VIEW,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (IogScrollViewClass, zoom_changed),
			      NULL, NULL,
			      iog_marshal_VOID__DOUBLE,
			      G_TYPE_NONE, 1,
			      G_TYPE_DOUBLE);

	widget_class->size_allocate = iog_scroll_view_size_allocate;
	widget_class->style_set = iog_scroll_view_style_set;

	g_type_class_add_private (klass, sizeof (IogScrollViewPrivate));
}

static void
view_on_drag_begin_cb (GtkWidget        *widget,
		       GdkDragContext   *context,
		       gpointer          user_data)
{
	IogScrollView *view;
	IogImage *image;
	GdkPixbuf *thumbnail;
	gint width, height;

	view = IOG_SCROLL_VIEW (user_data);
	image = view->priv->image;

	thumbnail = iog_image_get_thumbnail (image);

	if  (thumbnail) {
		width = gdk_pixbuf_get_width (thumbnail);
		height = gdk_pixbuf_get_height (thumbnail);
		gtk_drag_set_icon_pixbuf (context, thumbnail, width/2, height/2);
		g_object_unref (thumbnail);
	}
}

static void
view_on_drag_data_get_cb (GtkWidget        *widget,
			  GdkDragContext   *drag_context,
			  GtkSelectionData *data,
			  guint             info,
			  guint             time,
			  gpointer          user_data)
{
	IogScrollView *view;
	IogImage *image;
	gchar *uris[2];
	GFile *file;

	view = IOG_SCROLL_VIEW (user_data);

	image = view->priv->image;

	file = iog_image_get_file (image);
	uris[0] = g_file_get_uri (file);
	uris[1] = NULL;

	gtk_selection_data_set_uris (data, uris);

	g_free (uris[0]);
	g_object_unref (file);
}

GtkWidget*
iog_scroll_view_new (void)
{
	GtkWidget *widget;

	widget = g_object_new (IOG_TYPE_SCROLL_VIEW,
			       "can-focus", TRUE,
#if GTK_CHECK_VERSION (3, 4, 0)
			       "row-homogeneous", FALSE,
			       "column-homogeneous", FALSE,
#else
			       "n_rows", 2,
			       "n_columns", 2,
			       "homogeneous", FALSE,
#endif
			       NULL);


	return widget;
}

static void
iog_scroll_view_popup_menu (IogScrollView *view, GdkEventButton *event)
{
	GtkWidget *popup;
	int button, event_time;

	popup = view->priv->menu;

	if (event) {
		button = event->button;
		event_time = event->time;
	} else {
		button = 0;
		event_time = gtk_get_current_event_time ();
	}

	gtk_menu_popup (GTK_MENU (popup), NULL, NULL, NULL, NULL,
			button, event_time);
}

static gboolean
view_on_button_press_event_cb (GtkWidget *view, GdkEventButton *event,
			       gpointer user_data)
{
    /* Ignore double-clicks and triple-clicks */
    if (event->button == 3 && event->type == GDK_BUTTON_PRESS)
    {
	    iog_scroll_view_popup_menu (IOG_SCROLL_VIEW (view), event);

	    return TRUE;
    }

    return FALSE;
}

void
iog_scroll_view_set_popup (IogScrollView *view,
			   GtkMenu *menu)
{
	g_return_if_fail (IOG_IS_SCROLL_VIEW (view));
	g_return_if_fail (view->priv->menu == NULL);

	view->priv->menu = g_object_ref (menu);

	gtk_menu_attach_to_widget (GTK_MENU (view->priv->menu),
				   GTK_WIDGET (view),
				   NULL);

	g_signal_connect (G_OBJECT (view), "button_press_event",
			  G_CALLBACK (view_on_button_press_event_cb), NULL);
}

static gboolean
_iog_gdk_color_equal0 (const GdkColor *a, const GdkColor *b)
{
	if (a == NULL || b == NULL)
		return (a == b);

	return gdk_color_equal (a, b);
}

static gboolean
_iog_replace_gdk_color (GdkColor **dest, const GdkColor *new)
{
	GdkColor *old = *dest;

	if (_iog_gdk_color_equal0 (old, new))
		return FALSE;

	if (old != NULL)
		gdk_color_free (old);

	*dest = (new) ? gdk_color_copy (new) : NULL;

	return TRUE;
}

static void
_iog_scroll_view_update_bg_color (IogScrollView *view)
{
	const GdkColor *selected;
	IogScrollViewPrivate *priv = view->priv;

	if (!priv->use_bg_color)
		selected = NULL;
	else if (priv->override_bg_color)
		selected = priv->override_bg_color;
	else
		selected = priv->background_color;

	if (priv->transp_style == IOG_TRANSP_BACKGROUND
	    && priv->background_surface != NULL) {
		/* Delete the SVG background to have it recreated with
		 * the correct color during the next SVG redraw */
		cairo_surface_destroy (priv->background_surface);
		priv->background_surface = NULL;
	}

#if GTK_CHECK_VERSION (3, 0, 0)
	gtk_widget_modify_bg (GTK_WIDGET (priv->display),
#else
	gtk_widget_modify_bg (GTK_WIDGET (view),
#endif
			      GTK_STATE_NORMAL,
			      selected);
}

void
iog_scroll_view_set_background_color (IogScrollView *view,
				      const GdkColor *color)
{
	g_return_if_fail (IOG_IS_SCROLL_VIEW (view));

	if (_iog_replace_gdk_color (&view->priv->background_color, color))
		_iog_scroll_view_update_bg_color (view);
}

void
iog_scroll_view_override_bg_color (IogScrollView *view,
				   const GdkColor *color)
{
	g_return_if_fail (IOG_IS_SCROLL_VIEW (view));

	if (_iog_replace_gdk_color (&view->priv->override_bg_color, color))
		_iog_scroll_view_update_bg_color (view);
}

void
iog_scroll_view_set_use_bg_color (IogScrollView *view, gboolean use)
{
	IogScrollViewPrivate *priv;

	g_return_if_fail (IOG_IS_SCROLL_VIEW (view));

	priv = view->priv;

	if (use != priv->use_bg_color) {
		priv->use_bg_color = use;

		_iog_scroll_view_update_bg_color (view);

		g_object_notify (G_OBJECT (view), "use-background-color");
	}
}

void
iog_scroll_view_set_scroll_wheel_zoom (IogScrollView *view,
				       gboolean       scroll_wheel_zoom)
{
	g_return_if_fail (IOG_IS_SCROLL_VIEW (view));

	if (view->priv->scroll_wheel_zoom != scroll_wheel_zoom) {
		view->priv->scroll_wheel_zoom = scroll_wheel_zoom;
		g_object_notify (G_OBJECT (view), "scrollwheel-zoom");
	}
}

void
iog_scroll_view_set_zoom_multiplier (IogScrollView *view,
				     gdouble        zoom_multiplier)
{
	g_return_if_fail (IOG_IS_SCROLL_VIEW (view));

        view->priv->zoom_multiplier = 1.0 + zoom_multiplier;

	g_object_notify (G_OBJECT (view), "zoom-multiplier");
}
