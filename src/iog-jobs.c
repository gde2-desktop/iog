/* Eye Of Gde2 - Jobs
 *
 * Copyright (C) 2006 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 *
 * Based on evince code (shell/ev-jobs.c) by:
 * 	- Martin Kretzschmar <martink@gnome.org>
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

#include "iog-uri-converter.h"
#include "iog-jobs.h"
#include "iog-job-queue.h"
#include "iog-image.h"
#include "iog-transform.h"
#include "iog-list-store.h"
#include "iog-thumbnail.h"
#include "iog-pixbuf-util.h"

#include <gdk-pixbuf/gdk-pixbuf.h>

#define IOG_JOB_GET_PRIVATE(object) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((object), IOG_TYPE_JOB, IogJobPrivate))

G_DEFINE_TYPE (IogJob, iog_job, G_TYPE_OBJECT);
G_DEFINE_TYPE (IogJobThumbnail, iog_job_thumbnail, IOG_TYPE_JOB);
G_DEFINE_TYPE (IogJobLoad, iog_job_load, IOG_TYPE_JOB);
G_DEFINE_TYPE (IogJobModel, iog_job_model, IOG_TYPE_JOB);
G_DEFINE_TYPE (IogJobTransform, iog_job_transform, IOG_TYPE_JOB);
G_DEFINE_TYPE (IogJobSave, iog_job_save, IOG_TYPE_JOB);
G_DEFINE_TYPE (IogJobSaveAs, iog_job_save_as, IOG_TYPE_JOB_SAVE);
G_DEFINE_TYPE (IogJobCopy, iog_job_copy, IOG_TYPE_JOB);

enum
{
	SIGNAL_FINISHED,
	SIGNAL_PROGRESS,
	SIGNAL_LAST_SIGNAL
};

static guint job_signals[SIGNAL_LAST_SIGNAL] = { 0 };

static void iog_job_copy_run      (IogJob *ejob);
static void iog_job_load_run 	  (IogJob *ejob);
static void iog_job_model_run     (IogJob *ejob);
static void iog_job_save_run      (IogJob *job);
static void iog_job_save_as_run   (IogJob *job);
static void iog_job_thumbnail_run (IogJob *ejob);
static void iog_job_transform_run (IogJob *ejob);

static void iog_job_init (IogJob *job)
{
	/* NOTE: We need to allocate the mutex here so the ABI stays the same when it used to use g_mutex_new */
	job->mutex = g_malloc (sizeof (GMutex));
	g_mutex_init (job->mutex);
	job->progress = 0.0;
}

static void
iog_job_dispose (GObject *object)
{
	IogJob *job;

	job = IOG_JOB (object);

	if (job->error) {
		g_error_free (job->error);
		job->error = NULL;
	}

	if (job->mutex) {
		g_mutex_clear (job->mutex);
		g_free (job->mutex);
	}

	(* G_OBJECT_CLASS (iog_job_parent_class)->dispose) (object);
}

static void
iog_job_run_default (IogJob *job)
{
	g_critical ("Class \"%s\" does not implement the required run action",
		    G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (job)));
}

static void
iog_job_class_init (IogJobClass *class)
{
	GObjectClass *oclass;

	oclass = G_OBJECT_CLASS (class);

	oclass->dispose = iog_job_dispose;

	class->run = iog_job_run_default;

	job_signals [SIGNAL_FINISHED] =
		g_signal_new ("finished",
			      IOG_TYPE_JOB,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (IogJobClass, finished),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	job_signals [SIGNAL_PROGRESS] =
		g_signal_new ("progress",
			      IOG_TYPE_JOB,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (IogJobClass, progress),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__FLOAT,
			      G_TYPE_NONE, 1,
			      G_TYPE_FLOAT);
}

void
iog_job_finished (IogJob *job)
{
	g_return_if_fail (IOG_IS_JOB (job));

	g_signal_emit (job, job_signals[SIGNAL_FINISHED], 0);
}

/**
 * iog_job_run:
 * @job: the job to execute.
 *
 * Executes the job passed as @job. Usually there is no need to call this
 * on your own. Jobs should be executed by using the IogJobQueue.
 **/
void
iog_job_run (IogJob *job)
{
	IogJobClass *class;

	g_return_if_fail (IOG_IS_JOB (job));

	class = IOG_JOB_GET_CLASS (job);
	if (class->run)
		class->run (job);
	else
		iog_job_run_default (job);
}
static gboolean
notify_progress (gpointer data)
{
	IogJob *job = IOG_JOB (data);

	g_signal_emit (job, job_signals[SIGNAL_PROGRESS], 0, job->progress);

	return FALSE;
}

void
iog_job_set_progress (IogJob *job, float progress)
{
	g_return_if_fail (IOG_IS_JOB (job));
	g_return_if_fail (progress >= 0.0 && progress <= 1.0);

	g_mutex_lock (job->mutex);
	job->progress = progress;
	g_mutex_unlock (job->mutex);

	g_idle_add (notify_progress, job);
}

static void iog_job_thumbnail_init (IogJobThumbnail *job) { /* Do Nothing */ }

static void
iog_job_thumbnail_dispose (GObject *object)
{
	IogJobThumbnail *job;

	job = IOG_JOB_THUMBNAIL (object);

	if (job->image) {
		g_object_unref (job->image);
		job->image = NULL;
	}

	if (job->thumbnail) {
		g_object_unref (job->thumbnail);
		job->thumbnail = NULL;
	}

	(* G_OBJECT_CLASS (iog_job_thumbnail_parent_class)->dispose) (object);
}

static void
iog_job_thumbnail_class_init (IogJobThumbnailClass *class)
{
	GObjectClass *oclass;

	oclass = G_OBJECT_CLASS (class);

	oclass->dispose = iog_job_thumbnail_dispose;

	IOG_JOB_CLASS (class)->run = iog_job_thumbnail_run;
}

IogJob *
iog_job_thumbnail_new (IogImage *image)
{
	IogJobThumbnail *job;

	job = g_object_new (IOG_TYPE_JOB_THUMBNAIL, NULL);

	if (image) {
		job->image = g_object_ref (image);
	}

	return IOG_JOB (job);
}

static void
iog_job_thumbnail_run (IogJob *ejob)
{
	gchar *orig_width, *orig_height;
	gint width, height;
	GdkPixbuf *pixbuf;
	IogJobThumbnail *job;

	g_return_if_fail (IOG_IS_JOB_THUMBNAIL (ejob));

	job = IOG_JOB_THUMBNAIL (ejob);

	if (ejob->error) {
	        g_error_free (ejob->error);
		ejob->error = NULL;
	}

	job->thumbnail = iog_thumbnail_load (job->image,
					     &ejob->error);

	if (!job->thumbnail) {
		ejob->finished = TRUE;
		return;
	}

	orig_width = g_strdup (gdk_pixbuf_get_option (job->thumbnail, "tEXt::Thumb::Image::Width"));
	orig_height = g_strdup (gdk_pixbuf_get_option (job->thumbnail, "tEXt::Thumb::Image::Height"));

	pixbuf = iog_thumbnail_fit_to_size (job->thumbnail, IOG_LIST_STORE_THUMB_SIZE);
	g_object_unref (job->thumbnail);
	job->thumbnail = iog_thumbnail_add_frame (pixbuf);
	g_object_unref (pixbuf);

	if (orig_width) {
		sscanf (orig_width, "%i", &width);
		g_object_set_data (G_OBJECT (job->thumbnail),
				   IOG_THUMBNAIL_ORIGINAL_WIDTH,
				   GINT_TO_POINTER (width));
		g_free (orig_width);
	}
	if (orig_height) {
		sscanf (orig_height, "%i", &height);
		g_object_set_data (G_OBJECT (job->thumbnail),
				   IOG_THUMBNAIL_ORIGINAL_HEIGHT,
				   GINT_TO_POINTER (height));
		g_free (orig_height);
	}

	if (ejob->error) {
		g_warning ("%s", ejob->error->message);
	}

	ejob->finished = TRUE;
}

static void iog_job_load_init (IogJobLoad *job) { /* Do Nothing */ }

static void
iog_job_load_dispose (GObject *object)
{
	IogJobLoad *job;

	job = IOG_JOB_LOAD (object);

	if (job->image) {
		g_object_unref (job->image);
		job->image = NULL;
	}

	(* G_OBJECT_CLASS (iog_job_load_parent_class)->dispose) (object);
}

static void
iog_job_load_class_init (IogJobLoadClass *class)
{
	GObjectClass *oclass;

	oclass = G_OBJECT_CLASS (class);

	oclass->dispose = iog_job_load_dispose;
	IOG_JOB_CLASS (class)->run = iog_job_load_run;
}

IogJob *
iog_job_load_new (IogImage *image, IogImageData data)
{
	IogJobLoad *job;

	job = g_object_new (IOG_TYPE_JOB_LOAD, NULL);

	if (image) {
		job->image = g_object_ref (image);
	}
	job->data = data;

	return IOG_JOB (job);
}

static void
iog_job_load_run (IogJob *job)
{
	g_return_if_fail (IOG_IS_JOB_LOAD (job));

	if (job->error) {
	        g_error_free (job->error);
		job->error = NULL;
	}

	iog_image_load (IOG_IMAGE (IOG_JOB_LOAD (job)->image),
			IOG_JOB_LOAD (job)->data,
			job,
			&job->error);

	job->finished = TRUE;
}

static void iog_job_model_init (IogJobModel *job) { /* Do Nothing */ }

static void
iog_job_model_class_init (IogJobModelClass *class)
{
	IOG_JOB_CLASS (class)->run = iog_job_model_run;
}

/**
 * iog_job_model_new:
 * @file_list: (element-type GFile): a #GFile list
 *
 * Creates a new #IogJob model.
 *
 * Returns: A #IogJob.
 */

IogJob *
iog_job_model_new (GSList *file_list)
{
	IogJobModel *job;

	job = g_object_new (IOG_TYPE_JOB_MODEL, NULL);

	job->file_list = file_list;

	return IOG_JOB (job);
}

static void
filter_files (GSList *files, GList **file_list, GList **error_list)
{
	GSList *it;
	GFileInfo *file_info;

	for (it = files; it != NULL; it = it->next) {
		GFile *file;
		GFileType type = G_FILE_TYPE_UNKNOWN;

		file = (GFile *) it->data;

		if (file != NULL) {
			file_info = g_file_query_info (file,
						       G_FILE_ATTRIBUTE_STANDARD_TYPE","G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
						       0, NULL, NULL);
			if (file_info == NULL) {
				type = G_FILE_TYPE_UNKNOWN;
			} else {
				type = g_file_info_get_file_type (file_info);

				/* Workaround for gvfs backends that
				   don't set the GFileType. */
				if (G_UNLIKELY (type == G_FILE_TYPE_UNKNOWN)) {
					const gchar *ctype;

					ctype = g_file_info_get_content_type (file_info);

					/* If the content type is supported
					   adjust the file_type */
					if (iog_image_is_supported_mime_type (ctype))
						type = G_FILE_TYPE_REGULAR;
				}

				g_object_unref (file_info);
			}
		}

		switch (type) {
		case G_FILE_TYPE_REGULAR:
		case G_FILE_TYPE_DIRECTORY:
			*file_list = g_list_prepend (*file_list, g_object_ref (file));
			break;
		default:
			*error_list = g_list_prepend (*error_list,
						      g_file_get_uri (file));
			break;
		}

		g_object_unref (file);
	}

	*file_list  = g_list_reverse (*file_list);
	*error_list = g_list_reverse (*error_list);
}

static void
iog_job_model_run (IogJob *ejob)
{
	GList *filtered_list = NULL;
	GList *error_list = NULL;
	IogJobModel *job;

	g_return_if_fail (IOG_IS_JOB_MODEL (ejob));

	job = IOG_JOB_MODEL (ejob);

	filter_files (job->file_list, &filtered_list, &error_list);

	job->store = IOG_LIST_STORE (iog_list_store_new ());

	iog_list_store_add_files (job->store, filtered_list);

	g_list_foreach (filtered_list, (GFunc) g_object_unref, NULL);
	g_list_free (filtered_list);

	g_list_foreach (error_list, (GFunc) g_free, NULL);
	g_list_free (error_list);

	ejob->finished = TRUE;
}

static void iog_job_transform_init (IogJobTransform *job) { /* Do Nothing */ }

static void
iog_job_transform_dispose (GObject *object)
{
	IogJobTransform *job;

	job = IOG_JOB_TRANSFORM (object);

	if (job->trans) {
		g_object_unref (job->trans);
		job->trans = NULL;
	}

	g_list_foreach (job->images, (GFunc) g_object_unref, NULL);
	g_list_free (job->images);

	(* G_OBJECT_CLASS (iog_job_transform_parent_class)->dispose) (object);
}

static void
iog_job_transform_class_init (IogJobTransformClass *class)
{
	GObjectClass *oclass;

	oclass = G_OBJECT_CLASS (class);

	oclass->dispose = iog_job_transform_dispose;

	IOG_JOB_CLASS (class)->run = iog_job_transform_run;
}

/**
 * iog_job_transform_new:
 * @images: (element-type IogImage) (transfer full): a #IogImage list
 * @trans: a #EogTransform
 *
 * Create a new #IogJob for image transformation.
 *
 * Returns: A #IogJob.
 */

IogJob *
iog_job_transform_new (GList *images, IogTransform *trans)
{
	IogJobTransform *job;

	job = g_object_new (IOG_TYPE_JOB_TRANSFORM, NULL);

	if (trans) {
		job->trans = g_object_ref (trans);
	} else {
		job->trans = NULL;
	}

	job->images = images;

	return IOG_JOB (job);
}

static gboolean
iog_job_transform_image_modified (gpointer data)
{
	g_return_val_if_fail (IOG_IS_IMAGE (data), FALSE);

	iog_image_modified (IOG_IMAGE (data));
	g_object_unref (G_OBJECT (data));

	return FALSE;
}

void
iog_job_transform_run (IogJob *ejob)
{
	IogJobTransform *job;
	GList *it;

	g_return_if_fail (IOG_IS_JOB_TRANSFORM (ejob));

	job = IOG_JOB_TRANSFORM (ejob);

	if (ejob->error) {
	        g_error_free (ejob->error);
		ejob->error = NULL;
	}

	for (it = job->images; it != NULL; it = it->next) {
		IogImage *image = IOG_IMAGE (it->data);

		if (job->trans == NULL) {
			iog_image_undo (image);
		} else {
			iog_image_transform (image, job->trans, ejob);
		}

		if (iog_image_is_modified (image) || job->trans == NULL) {
			g_object_ref (image);
			g_idle_add (iog_job_transform_image_modified, image);
		}
	}

	ejob->finished = TRUE;
}

static void iog_job_save_init (IogJobSave *job) { /* do nothing */ }

static void
iog_job_save_dispose (GObject *object)
{
	IogJobSave *job;

	job = IOG_JOB_SAVE (object);

	if (job->images) {
		g_list_foreach (job->images, (GFunc) g_object_unref, NULL);
		g_list_free (job->images);
		job->images = NULL;
	}

	(* G_OBJECT_CLASS (iog_job_save_parent_class)->dispose) (object);
}

static void
iog_job_save_class_init (IogJobSaveClass *class)
{
	G_OBJECT_CLASS (class)->dispose = iog_job_save_dispose;
	IOG_JOB_CLASS (class)->run = iog_job_save_run;
}

/**
 * iog_job_save_new:
 * @images: (element-type IogImage) (transfer full): a #IogImage list
 *
 * Creates a new #IogJob for image saving.
 *
 * Returns: A #IogJob.
 */

IogJob *
iog_job_save_new (GList *images)
{
	IogJobSave *job;

	job = g_object_new (IOG_TYPE_JOB_SAVE, NULL);

	job->images = images;
	job->current_image = NULL;

	return IOG_JOB (job);
}

static void
save_progress_handler (IogImage *image, gfloat progress, gpointer data)
{
	IogJobSave *job = IOG_JOB_SAVE (data);
	guint n_images = g_list_length (job->images);
	gfloat job_progress;

	job_progress = (job->current_pos / (gfloat) n_images) + (progress / n_images);

	iog_job_set_progress (IOG_JOB (job), job_progress);
}

static void
iog_job_save_run (IogJob *ejob)
{
	IogJobSave *job;
	GList *it;

	g_return_if_fail (IOG_IS_JOB_SAVE (ejob));

	job = IOG_JOB_SAVE (ejob);

	job->current_pos = 0;

	for (it = job->images; it != NULL; it = it->next, job->current_pos++) {
		IogImage *image = IOG_IMAGE (it->data);
		IogImageSaveInfo *save_info = NULL;
		gulong handler_id = 0;
		gboolean success = FALSE;

		job->current_image = image;

		/* Make sure the image doesn't go away while saving */
		iog_image_data_ref (image);

		if (!iog_image_has_data (image, IOG_IMAGE_DATA_ALL)) {
			IogImageMetadataStatus m_status;
			gint data2load = 0;

			m_status = iog_image_get_metadata_status (image);
			if (!iog_image_has_data (image, IOG_IMAGE_DATA_IMAGE)) {
				// Queue full read in this case
				data2load = IOG_IMAGE_DATA_ALL;
			} else if (m_status == IOG_IMAGE_METADATA_NOT_READ) {
				// Load only if we haven't read it yet
				data2load = IOG_IMAGE_DATA_EXIF | IOG_IMAGE_DATA_XMP;
			}

			if (data2load != 0) {
				iog_image_load (image,
						data2load,
						NULL,
						&ejob->error);
			}
		}

		handler_id = g_signal_connect (G_OBJECT (image),
					       "save-progress",
				               G_CALLBACK (save_progress_handler),
					       job);

		save_info = iog_image_save_info_new_from_image (image);

		success = iog_image_save_by_info (image,
						  save_info,
						  &ejob->error);

		if (save_info)
			g_object_unref (save_info);

		if (handler_id != 0)
			g_signal_handler_disconnect (G_OBJECT (image), handler_id);

		iog_image_data_unref (image);

		if (!success) break;
	}

	ejob->finished = TRUE;
}

static void iog_job_save_as_init (IogJobSaveAs *job) { /* do nothing */ }

static void iog_job_save_as_dispose (GObject *object)
{
	IogJobSaveAs *job = IOG_JOB_SAVE_AS (object);

	if (job->converter != NULL) {
		g_object_unref (job->converter);
		job->converter = NULL;
	}

	if (job->file != NULL) {
		g_object_unref (job->file);
		job->file = NULL;
	}

	(* G_OBJECT_CLASS (iog_job_save_as_parent_class)->dispose) (object);
}

static void
iog_job_save_as_class_init (IogJobSaveAsClass *class)
{
	G_OBJECT_CLASS (class)->dispose = iog_job_save_as_dispose;
	IOG_JOB_CLASS (class)->run = iog_job_save_as_run;
}

/**
 * iog_job_save_as_new:
 * @images: (element-type IogImage) (transfer full): a #IogImage list
 * @converter: a URI converter
 * file: a #GFile
 *
 * Creates a new #IogJob for save as.
 *
 * Returns: A #IogJob.
 */

IogJob *
iog_job_save_as_new (GList *images, IogURIConverter *converter, GFile *file)
{
	IogJobSaveAs *job;

	g_assert (converter != NULL || g_list_length (images) == 1);

	job = g_object_new (IOG_TYPE_JOB_SAVE_AS, NULL);

	IOG_JOB_SAVE(job)->images = images;

	job->converter = converter ? g_object_ref (converter) : NULL;
	job->file = file ? g_object_ref (file) : NULL;

	return IOG_JOB (job);
}

static void
iog_job_save_as_run (IogJob *ejob)
{
	IogJobSave *job;
	IogJobSaveAs *saveas_job;
	GList *it;
	guint n_images;

	g_return_if_fail (IOG_IS_JOB_SAVE_AS (ejob));

	job = IOG_JOB_SAVE (ejob);

	n_images = g_list_length (job->images);

	saveas_job = IOG_JOB_SAVE_AS (job);

	job->current_pos = 0;

	for (it = job->images; it != NULL; it = it->next, job->current_pos++) {
		GdkPixbufFormat *format;
		IogImageSaveInfo *src_info, *dest_info;
		IogImage *image = IOG_IMAGE (it->data);
		gboolean success = FALSE;
		gulong handler_id = 0;

		job->current_image = image;

		iog_image_data_ref (image);

		if (!iog_image_has_data (image, IOG_IMAGE_DATA_ALL)) {
			IogImageMetadataStatus m_status;
			gint data2load = 0;

			m_status = iog_image_get_metadata_status (image);
			if (!iog_image_has_data (image, IOG_IMAGE_DATA_IMAGE)) {
				// Queue full read in this case
				data2load = IOG_IMAGE_DATA_ALL;
			} else if (m_status == IOG_IMAGE_METADATA_NOT_READ) {
				// Load only if we haven't read it yet
				data2load = IOG_IMAGE_DATA_EXIF | IOG_IMAGE_DATA_XMP;
			}

			if (data2load != 0) {
				iog_image_load (image,
						data2load,
						NULL,
						&ejob->error);
			}
		}

		g_assert (ejob->error == NULL);

		handler_id = g_signal_connect (G_OBJECT (image),
					       "save-progress",
				               G_CALLBACK (save_progress_handler),
					       job);

		src_info = iog_image_save_info_new_from_image (image);

		if (n_images == 1) {
			g_assert (saveas_job->file != NULL);

			format = iog_pixbuf_get_format (saveas_job->file);

			dest_info = iog_image_save_info_new_from_file (saveas_job->file,
								   format);

		/* SaveAsDialog has already secured permission to overwrite */
			if (dest_info->exists) {
				dest_info->overwrite = TRUE;
			}
		} else {
			GFile *dest_file;
			gboolean result;

			result = iog_uri_converter_do (saveas_job->converter,
						       image,
						       &dest_file,
						       &format,
						       NULL);

			g_assert (result);

			dest_info = iog_image_save_info_new_from_file (dest_file,
								   format);
		}

		success = iog_image_save_as_by_info (image,
						     src_info,
						     dest_info,
						     &ejob->error);

		if (src_info)
			g_object_unref (src_info);

		if (dest_info)
			g_object_unref (dest_info);

		if (handler_id != 0)
			g_signal_handler_disconnect (G_OBJECT (image), handler_id);

		iog_image_data_unref (image);

		if (!success)
			break;
	}

	ejob->finished = TRUE;
}

static void iog_job_copy_init (IogJobCopy *job) { /* do nothing */};

static void
iog_job_copy_dispose (GObject *object)
{
	IogJobCopy *job = IOG_JOB_COPY (object);

	if (job->dest) {
		g_free (job->dest);
		job->dest = NULL;
	}

	(* G_OBJECT_CLASS (iog_job_copy_parent_class)->dispose) (object);
}

static void
iog_job_copy_class_init (IogJobCopyClass *class)
{
	G_OBJECT_CLASS (class)->dispose = iog_job_copy_dispose;
	IOG_JOB_CLASS (class)->run = iog_job_copy_run;
}

/**
 * iog_job_copy_new:
 * @images: (element-type IogImage) (transfer full): a #IogImage list
 * @dest: destination path for the copy
 *
 * Creates a new #IogJob.
 *
 * Returns: A #IogJob.
 */

IogJob *
iog_job_copy_new (GList *images, const gchar *dest)
{
	IogJobCopy *job;

	g_assert (images != NULL && dest != NULL);

	job = g_object_new (IOG_TYPE_JOB_COPY, NULL);

	job->images = images;
	job->dest = g_strdup (dest);

	return IOG_JOB (job);
}

static void
iog_job_copy_progress_callback (goffset current_num_bytes,
				goffset total_num_bytes,
				gpointer user_data)
{
	gfloat job_progress;
	guint n_images;
	IogJobCopy *job;

	job = IOG_JOB_COPY (user_data);
	n_images = g_list_length (job->images);

	job_progress =  ((current_num_bytes / (gfloat) total_num_bytes) + job->current_pos)/n_images;

	iog_job_set_progress (IOG_JOB (job), job_progress);
}

void
iog_job_copy_run (IogJob *ejob)
{
	IogJobCopy *job;
	GList *it;
	GFile *src, *dest;
	gchar *filename, *dest_filename;

	g_return_if_fail (IOG_IS_JOB_COPY (ejob));

	job = IOG_JOB_COPY (ejob);

	job->current_pos = 0;

	for (it = job->images; it != NULL; it = g_list_next (it), job->current_pos++) {
		src = (GFile *) it->data;
		filename = g_file_get_basename (src);
		dest_filename = g_build_filename (job->dest, filename, NULL);
		dest = g_file_new_for_path (dest_filename);

		g_file_copy (src, dest,
			     G_FILE_COPY_OVERWRITE, NULL,
			     iog_job_copy_progress_callback, job,
			     &ejob->error);
		g_free (filename);
		g_free (dest_filename);
	}

	ejob->finished = TRUE;
}
