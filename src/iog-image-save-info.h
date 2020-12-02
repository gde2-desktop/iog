#ifndef _IOG_IMAGE_SAVE_INFO_H_
#define _IOG_IMAGE_SAVE_INFO_H_

#include <glib-object.h>
#include <gio/gio.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

#ifndef __IOG_IMAGE_DECLR__
#define __IOG_IMAGE_DECLR__
typedef struct _IogImage IogImage;
#endif

#define IOG_TYPE_IMAGE_SAVE_INFO            (iog_image_save_info_get_type ())
#define IOG_IMAGE_SAVE_INFO(o)         (G_TYPE_CHECK_INSTANCE_CAST ((o), IOG_TYPE_IMAGE_SAVE_INFO, IogImageSaveInfo))
#define IOG_IMAGE_SAVE_INFO_CLASS(k)   (G_TYPE_CHECK_CLASS_CAST((k), IOG_TYPE_IMAGE_SAVE_INFO, IogImageSaveInfoClass))
#define IOG_IS_IMAGE_SAVE_INFO(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), IOG_TYPE_IMAGE_SAVE_INFO))
#define IOG_IS_IMAGE_SAVE_INFO_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), IOG_TYPE_IMAGE_SAVE_INFO))
#define IOG_IMAGE_SAVE_INFO_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), IOG_TYPE_IMAGE_SAVE_INFO, IogImageSaveInfoClass))

typedef struct _IogImageSaveInfo IogImageSaveInfo;
typedef struct _IogImageSaveInfoClass IogImageSaveInfoClass;

struct _IogImageSaveInfo {
	GObject parent;

	GFile       *file;
	char        *format;
	gboolean     exists;
	gboolean     local;
	gboolean     has_metadata;
	gboolean     modified;
	gboolean     overwrite;

	float        jpeg_quality; /* valid range: [0.0 ... 1.0] */
};

struct _IogImageSaveInfoClass {
	GObjectClass parent_klass;
};

#define IOG_FILE_FORMAT_JPEG   "jpeg"

GType             iog_image_save_info_get_type         (void) G_GNUC_CONST;

IogImageSaveInfo *iog_image_save_info_new_from_image   (IogImage        *image);

IogImageSaveInfo *iog_image_save_info_new_from_uri     (const char      *uri,
						       GdkPixbufFormat  *format);

IogImageSaveInfo *iog_image_save_info_new_from_file    (GFile           *file,
						       GdkPixbufFormat  *format);

G_END_DECLS

#endif /* _IOG_IMAGE_SAVE_INFO_H_ */
