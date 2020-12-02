#ifndef _IOG_TRANSFORM_H_
#define _IOG_TRANSFORM_H_

#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

#ifndef __IOG_JOB_DECLR__
#define __IOG_JOB_DECLR__
typedef struct _IogJob IogJob;
#endif

typedef enum {
	IOG_TRANSFORM_NONE,
	IOG_TRANSFORM_ROT_90,
	IOG_TRANSFORM_ROT_180,
	IOG_TRANSFORM_ROT_270,
	IOG_TRANSFORM_FLIP_HORIZONTAL,
	IOG_TRANSFORM_FLIP_VERTICAL,
	IOG_TRANSFORM_TRANSPOSE,
	IOG_TRANSFORM_TRANSVERSE
} IogTransformType;

#define IOG_TYPE_TRANSFORM          (iog_transform_get_type ())
#define IOG_TRANSFORM(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), IOG_TYPE_TRANSFORM, IogTransform))
#define IOG_TRANSFORM_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST((k), IOG_TYPE_TRANSFORM, IogTransformClass))
#define IOG_IS_TRANSFORM(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), IOG_TYPE_TRANSFORM))
#define IOG_IS_TRANSFORM_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), IOG_TYPE_TRANSFORM))
#define IOG_TRANSFORM_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), IOG_TYPE_TRANSFORM, IogTransformClass))

/* =========================================

    GObjecat wrapper around an affine transformation

   ----------------------------------------*/

typedef struct _IogTransform IogTransform;
typedef struct _IogTransformClass IogTransformClass;
typedef struct _IogTransformPrivate IogTransformPrivate;

struct _IogTransform {
	GObject parent;

	IogTransformPrivate *priv;
};

struct _IogTransformClass {
	GObjectClass parent_klass;
};

GType         iog_transform_get_type (void) G_GNUC_CONST;

GdkPixbuf*    iog_transform_apply   (IogTransform *trans, GdkPixbuf *pixbuf, IogJob *job);
IogTransform* iog_transform_reverse (IogTransform *trans);
IogTransform* iog_transform_compose (IogTransform *trans, IogTransform *compose);
gboolean      iog_transform_is_identity (IogTransform *trans);

IogTransform* iog_transform_identity_new (void);
IogTransform* iog_transform_rotate_new (int degree);
IogTransform* iog_transform_flip_new   (IogTransformType type /* only IOG_TRANSFORM_FLIP_* are valid */);
#if 0
IogTransform* iog_transform_scale_new  (double sx, double sy);
#endif
IogTransform* iog_transform_new (IogTransformType trans);

IogTransformType iog_transform_get_transform_type (IogTransform *trans);

gboolean         iog_transform_get_affine (IogTransform *trans, cairo_matrix_t *affine);

G_END_DECLS

#endif /* _IOG_TRANSFORM_H_ */


