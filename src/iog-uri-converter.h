#ifndef _IOG_URI_CONVERTER_H_
#define _IOG_URI_CONVERTER_H_

#include <glib-object.h>
#include <glib/gi18n.h>
#include "iog-image.h"

G_BEGIN_DECLS

#define IOG_TYPE_URI_CONVERTER          (iog_uri_converter_get_type ())
#define IOG_URI_CONVERTER(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), IOG_TYPE_URI_CONVERTER, IogURIConverter))
#define IOG_URI_CONVERTER_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST((k), IOG_TYPE_URI_CONVERTER, IogURIConverterClass))
#define IOG_IS_URI_CONVERTER(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), IOG_TYPE_URI_CONVERTER))
#define IOG_IS_URI_CONVERTER_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), IOG_TYPE_URI_CONVERTER))
#define IOG_URI_CONVERTER_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), IOG_TYPE_URI_CONVERTER, IogURIConverterClass))

#ifndef __IOG_URI_CONVERTER_DECLR__
#define __IOG_URI_CONVERTER_DECLR__
typedef struct _IogURIConverter IogURIConverter;
#endif
typedef struct _IogURIConverterClass IogURIConverterClass;
typedef struct _IogURIConverterPrivate IogURIConverterPrivate;

typedef enum {
	IOG_UC_STRING,
	IOG_UC_FILENAME,
	IOG_UC_COUNTER,
	IOG_UC_COMMENT,
	IOG_UC_DATE,
	IOG_UC_TIME,
	IOG_UC_DAY,
	IOG_UC_MONTH,
	IOG_UC_YEAR,
	IOG_UC_HOUR,
	IOG_UC_MINUTE,
	IOG_UC_SECOND,
	IOG_UC_END
} IogUCType;

typedef struct {
	char     *description;
	char     *rep;
	gboolean req_exif;
} IogUCInfo;

typedef enum {
	IOG_UC_ERROR_INVALID_UNICODE,
	IOG_UC_ERROR_INVALID_CHARACTER,
	IOG_UC_ERROR_EQUAL_FILENAMES,
	IOG_UC_ERROR_UNKNOWN
} IogUCError;

#define IOG_UC_ERROR iog_uc_error_quark ()


struct _IogURIConverter {
	GObject parent;

	IogURIConverterPrivate *priv;
};

struct _IogURIConverterClass {
	GObjectClass parent_klass;
};

G_GNUC_INTERNAL
GType              iog_uri_converter_get_type      (void) G_GNUC_CONST;

G_GNUC_INTERNAL
GQuark             iog_uc_error_quark              (void);

G_GNUC_INTERNAL
IogURIConverter*   iog_uri_converter_new           (GFile *base_file,
                                                    GdkPixbufFormat *img_format,
						    const char *format_string);

G_GNUC_INTERNAL
gboolean           iog_uri_converter_check         (IogURIConverter *converter,
                                                    GList *img_list,
                                                    GError **error);

G_GNUC_INTERNAL
gboolean           iog_uri_converter_requires_exif (IogURIConverter *converter);

G_GNUC_INTERNAL
gboolean           iog_uri_converter_do            (IogURIConverter *converter,
                                                    IogImage *image,
                                                    GFile **file,
                                                    GdkPixbufFormat **format,
                                                    GError **error);

G_GNUC_INTERNAL
char*              iog_uri_converter_preview       (const char *format_str,
                                                    IogImage *img,
                                                    GdkPixbufFormat *format,
						    gulong counter,
						    guint n_images,
						    gboolean convert_spaces,
						    gunichar space_char);

/* for debugging purpose only */
G_GNUC_INTERNAL
void                iog_uri_converter_print_list (IogURIConverter *conv);

G_END_DECLS

#endif /* _IOG_URI_CONVERTER_H_ */
