#ifndef _IOG_IMAGE_JPEG_H_
#define _IOG_IMAGE_JPEG_H_

#if HAVE_JPEG

#include <glib.h>
#include "iog-image.h"
#include "iog-image-save-info.h"

/* Saves a source jpeg file in an arbitrary format (as specified by
 * target). The target pointer may be NULL, in which case the output
 * file is saved as jpeg too.  This method tries to be as smart as
 * possible. It will save the image as lossless as possible (if the
 * target is a jpeg image too).
 */
G_GNUC_INTERNAL
gboolean iog_image_jpeg_save_file (IogImage *image, const char *file,
				   IogImageSaveInfo *source, IogImageSaveInfo *target,
				   GError **error);
#endif

#endif /* _IOG_IMAGE_JPEG_H_ */
