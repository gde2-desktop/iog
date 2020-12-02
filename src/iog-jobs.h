/* Eye Of Gde2 - Jobs
 *
 * Copyright (C) 2006 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 *
 * Based on evince code (shell/ev-jobs.h) by:
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

#ifndef __IOG_JOBS_H__
#define __IOG_JOBS_H__

#include "iog-list-store.h"
#include "iog-transform.h"
#include "iog-enums.h"

#include <glib.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

#ifndef __IOG_IMAGE_DECLR__
#define __IOG_IMAGE_DECLR__
  typedef struct _IogImage IogImage;
#endif

#ifndef __IOG_URI_CONVERTER_DECLR__
#define __IOG_URI_CONVERTER_DECLR__
typedef struct _IogURIConverter IogURIConverter;
#endif

#ifndef __IOG_JOB_DECLR__
#define __IOG_JOB_DECLR__
typedef struct _IogJob IogJob;
#endif
typedef struct _IogJobClass IogJobClass;

typedef struct _IogJobThumbnail IogJobThumbnail;
typedef struct _IogJobThumbnailClass IogJobThumbnailClass;

typedef struct _IogJobLoad IogJobLoad;
typedef struct _IogJobLoadClass IogJobLoadClass;

typedef struct _IogJobModel IogJobModel;
typedef struct _IogJobModelClass IogJobModelClass;

typedef struct _IogJobTransform IogJobTransform;
typedef struct _IogJobTransformClass IogJobTransformClass;

typedef struct _IogJobSave IogJobSave;
typedef struct _IogJobSaveClass IogJobSaveClass;

typedef struct _IogJobSaveAs IogJobSaveAs;
typedef struct _IogJobSaveAsClass IogJobSaveAsClass;

typedef struct _IogJobCopy IogJobCopy;
typedef struct _IogJobCopyClass IogJobCopyClass;

#define IOG_TYPE_JOB		       (iog_job_get_type())
#define IOG_JOB(obj)		       (G_TYPE_CHECK_INSTANCE_CAST((obj), IOG_TYPE_JOB, IogJob))
#define IOG_JOB_CLASS(klass)	       (G_TYPE_CHECK_CLASS_CAST((klass),  IOG_TYPE_JOB, IogJobClass))
#define IOG_IS_JOB(obj)	               (G_TYPE_CHECK_INSTANCE_TYPE((obj), IOG_TYPE_JOB))
#define IOG_JOB_GET_CLASS(obj)         (G_TYPE_INSTANCE_GET_CLASS ((obj), IOG_TYPE_JOB, IogJobClass))

#define IOG_TYPE_JOB_THUMBNAIL	       (iog_job_thumbnail_get_type())
#define IOG_JOB_THUMBNAIL(obj)	       (G_TYPE_CHECK_INSTANCE_CAST((obj), IOG_TYPE_JOB_THUMBNAIL, IogJobThumbnail))
#define IOG_JOB_THUMBNAIL_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),  IOG_TYPE_JOB_THUMBNAIL, IogJobThumbnailClass))
#define IOG_IS_JOB_THUMBNAIL(obj)      (G_TYPE_CHECK_INSTANCE_TYPE((obj), IOG_TYPE_JOB_THUMBNAIL))

#define IOG_TYPE_JOB_LOAD	       (iog_job_load_get_type())
#define IOG_JOB_LOAD(obj)	       (G_TYPE_CHECK_INSTANCE_CAST((obj), IOG_TYPE_JOB_LOAD, IogJobLoad))
#define IOG_JOB_LOAD_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass),  IOG_TYPE_JOB_LOAD, IogJobLoadClass))
#define IOG_IS_JOB_LOAD(obj)	       (G_TYPE_CHECK_INSTANCE_TYPE((obj), IOG_TYPE_JOB_LOAD))

#define IOG_TYPE_JOB_MODEL	       (iog_job_model_get_type())
#define IOG_JOB_MODEL(obj)	       (G_TYPE_CHECK_INSTANCE_CAST((obj), IOG_TYPE_JOB_MODEL, IogJobModel))
#define IOG_JOB_MODEL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),  IOG_TYPE_JOB_MODEL, IogJobModelClass))
#define IOG_IS_JOB_MODEL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj), IOG_TYPE_JOB_MODEL))

#define IOG_TYPE_JOB_TRANSFORM	       (iog_job_transform_get_type())
#define IOG_JOB_TRANSFORM(obj)	       (G_TYPE_CHECK_INSTANCE_CAST((obj), IOG_TYPE_JOB_TRANSFORM, IogJobTransform))
#define IOG_JOB_TRANSFORM_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),  IOG_TYPE_JOB_TRANSFORM, IogJobTransformClass))
#define IOG_IS_JOB_TRANSFORM(obj)      (G_TYPE_CHECK_INSTANCE_TYPE((obj), IOG_TYPE_JOB_TRANSFORM))

#define IOG_TYPE_JOB_SAVE              (iog_job_save_get_type())
#define IOG_JOB_SAVE(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), IOG_TYPE_JOB_SAVE, IogJobSave))
#define IOG_JOB_SAVE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), IOG_TYPE_JOB_SAVE, IogJobSaveClass))
#define IOG_IS_JOB_SAVE(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), IOG_TYPE_JOB_SAVE))
#define IOG_JOB_SAVE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), IOG_TYPE_JOB_SAVE, IogJobSaveClass))

#define IOG_TYPE_JOB_SAVE_AS           (iog_job_save_as_get_type())
#define IOG_JOB_SAVE_AS(obj)           (G_TYPE_CHECK_INSTANCE_CAST((obj), IOG_TYPE_JOB_SAVE_AS, IogJobSaveAs))
#define IOG_JOB_SAVE_AS_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass), IOG_TYPE_JOB_SAVE_AS, IogJobSaveAsClass))
#define IOG_IS_JOB_SAVE_AS(obj)        (G_TYPE_CHECK_INSTANCE_TYPE((obj), IOG_TYPE_JOB_SAVE_AS))

#define IOG_TYPE_JOB_COPY	       (iog_job_copy_get_type())
#define IOG_JOB_COPY(obj)	       (G_TYPE_CHECK_INSTANCE_CAST((obj), IOG_TYPE_JOB_COPY, IogJobCopy))
#define IOG_JOB_COPY_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),  IOG_TYPE_JOB_COPY, IogJobCopyClass))
#define IOG_IS_JOB_COPY(obj)      (G_TYPE_CHECK_INSTANCE_TYPE((obj), IOG_TYPE_JOB_COPY))


struct _IogJob
{
	GObject  parent;

	GError   *error;
	GMutex   *mutex;
	float     progress;
	gboolean  finished;
};

struct _IogJobClass
{
	GObjectClass parent_class;

	void    (* finished) (IogJob *job);
	void    (* progress) (IogJob *job, float progress);
	void    (*run)       (IogJob *job);
};

struct _IogJobThumbnail
{
	IogJob       parent;
	IogImage    *image;
	GdkPixbuf   *thumbnail;
};

struct _IogJobThumbnailClass
{
	IogJobClass parent_class;
};

struct _IogJobLoad
{
	IogJob        parent;
	IogImage     *image;
	IogImageData  data;
};

struct _IogJobLoadClass
{
	IogJobClass parent_class;
};

struct _IogJobModel
{
	IogJob        parent;
	IogListStore *store;
	GSList       *file_list;
};

struct _IogJobModelClass
{
        IogJobClass parent_class;
};

struct _IogJobTransform
{
	IogJob        parent;
	GList        *images;
	IogTransform *trans;
};

struct _IogJobTransformClass
{
        IogJobClass parent_class;
};

typedef enum {
	IOG_SAVE_RESPONSE_NONE,
	IOG_SAVE_RESPONSE_RETRY,
	IOG_SAVE_RESPONSE_SKIP,
	IOG_SAVE_RESPONSE_OVERWRITE,
	IOG_SAVE_RESPONSE_CANCEL,
	IOG_SAVE_RESPONSE_LAST
} IogJobSaveResponse;

struct _IogJobSave
{
	IogJob    parent;
	GList	 *images;
	guint      current_pos;
	IogImage *current_image;
};

struct _IogJobSaveClass
{
	IogJobClass parent_class;
};

struct _IogJobSaveAs
{
	IogJobSave       parent;
	IogURIConverter *converter;
	GFile           *file;
};

struct _IogJobSaveAsClass
{
	IogJobSaveClass parent;
};

struct _IogJobCopy
{
	IogJob parent;
	GList *images;
	guint current_pos;
	gchar *dest;
};

struct _IogJobCopyClass
{
	IogJobClass parent_class;
};

/* base job class */
GType           iog_job_get_type           (void) G_GNUC_CONST;
void            iog_job_finished           (IogJob          *job);
void            iog_job_run                (IogJob          *job);
void            iog_job_set_progress       (IogJob          *job,
					    float            progress);

/* IogJobThumbnail */
GType           iog_job_thumbnail_get_type (void) G_GNUC_CONST;
IogJob         *iog_job_thumbnail_new      (IogImage     *image);

/* IogJobLoad */
GType           iog_job_load_get_type      (void) G_GNUC_CONST;
IogJob 	       *iog_job_load_new 	   (IogImage        *image,
					    IogImageData     data);

/* IogJobModel */
GType 		iog_job_model_get_type     (void) G_GNUC_CONST;
IogJob 	       *iog_job_model_new          (GSList          *file_list);

/* IogJobTransform */
GType 		iog_job_transform_get_type (void) G_GNUC_CONST;
IogJob 	       *iog_job_transform_new      (GList           *images,
					    IogTransform    *trans);

/* IogJobSave */
GType		iog_job_save_get_type      (void) G_GNUC_CONST;
IogJob         *iog_job_save_new           (GList           *images);

/* IogJobSaveAs */
GType		iog_job_save_as_get_type   (void) G_GNUC_CONST;
IogJob         *iog_job_save_as_new        (GList           *images,
					    IogURIConverter *converter,
					    GFile           *file);

/* IogJobCopy */
GType          iog_job_copy_get_type      (void) G_GNUC_CONST;
IogJob        *iog_job_copy_new           (GList            *images,
					   const gchar      *dest);

G_END_DECLS

#endif /* __IOG_JOBS_H__ */
