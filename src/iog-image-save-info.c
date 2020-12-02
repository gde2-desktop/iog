#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include "iog-image-save-info.h"
#include "iog-image-private.h"
#include "iog-pixbuf-util.h"
#include "iog-image.h"

G_DEFINE_TYPE (IogImageSaveInfo, iog_image_save_info, G_TYPE_OBJECT)

static void
iog_image_save_info_dispose (GObject *object)
{
	IogImageSaveInfo *info = IOG_IMAGE_SAVE_INFO (object);

	if (info->file != NULL) {
		g_object_unref (info->file);
		info->file = NULL;
	}

	if (info->format != NULL) {
		g_free (info->format);
		info->format = NULL;
	}

	G_OBJECT_CLASS (iog_image_save_info_parent_class)->dispose (object);
}

static void
iog_image_save_info_init (IogImageSaveInfo *obj)
{

}

static void
iog_image_save_info_class_init (IogImageSaveInfoClass *klass)
{
	GObjectClass *object_class = (GObjectClass*) klass;

	object_class->dispose = iog_image_save_info_dispose;
}

/* is_local_uri:
 *
 * Checks if the URI points to a local file system. This tests simply
 * if the URI scheme is 'file'. This function is used to ensure that
 * we can write to the path-part of the URI with non-VFS aware
 * filesystem calls.
 */
static gboolean
is_local_file (GFile *file)
{
	char *scheme;
	gboolean ret;

	g_return_val_if_fail (file != NULL, FALSE);

	scheme = g_file_get_uri_scheme (file);

	ret = (g_ascii_strcasecmp (scheme, "file") == 0);
	g_free (scheme);
	return ret;
}

static char*
get_save_file_type_by_file (GFile *file)
{
	GdkPixbufFormat *format;
	char *type = NULL;

	format = iog_pixbuf_get_format (file);
	if (format != NULL) {
		type = gdk_pixbuf_format_get_name (format);
	}

	return type;
}

IogImageSaveInfo*
iog_image_save_info_new_from_image (IogImage *image)
{
	IogImageSaveInfo *info = NULL;

	g_return_val_if_fail (IOG_IS_IMAGE (image), NULL);

	info = g_object_new (IOG_TYPE_IMAGE_SAVE_INFO, NULL);

	info->file         = iog_image_get_file (image);
	info->format       = g_strdup (image->priv->file_type);
	info->exists       = g_file_query_exists (info->file, NULL);
	info->local        = is_local_file (info->file);
        info->has_metadata = iog_image_has_data (image, IOG_IMAGE_DATA_EXIF);
	info->modified     = iog_image_is_modified (image);
	info->overwrite    = FALSE;

	info->jpeg_quality = -1.0;

	return info;
}

IogImageSaveInfo*
iog_image_save_info_new_from_uri (const char *txt_uri, GdkPixbufFormat *format)
{
	GFile *file;
	IogImageSaveInfo *info;

	g_return_val_if_fail (txt_uri != NULL, NULL);

	file = g_file_new_for_uri (txt_uri);

	info = iog_image_save_info_new_from_file (file, format);

	g_object_unref (file);

	return info;
}

IogImageSaveInfo*
iog_image_save_info_new_from_file (GFile *file, GdkPixbufFormat *format)
{
	IogImageSaveInfo *info;

	g_return_val_if_fail (file != NULL, NULL);

	info = g_object_new (IOG_TYPE_IMAGE_SAVE_INFO, NULL);

	info->file = g_object_ref (file);
	if (format == NULL) {
		info->format = get_save_file_type_by_file (info->file);
	}
	else {
		info->format = gdk_pixbuf_format_get_name (format);
	}
	info->exists       = g_file_query_exists (file, NULL);
	info->local        = is_local_file (file);
        info->has_metadata = FALSE;
	info->modified     = FALSE;
	info->overwrite    = FALSE;

	info->jpeg_quality = -1.0;

	g_assert (info->format != NULL);

	return info;
}
