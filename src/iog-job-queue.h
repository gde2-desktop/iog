/* Eye Of Gde2 - Jobs Queue
 *
 * Copyright (C) 2006 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 *
 * Based on evince code (shell/ev-job-queue.h) by:
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

#ifndef __IOG_JOB_QUEUE_H__
#define __IOG_JOB_QUEUE_H__

#include "iog-jobs.h"

#include <gtk/gtk.h>

G_BEGIN_DECLS

void     iog_job_queue_init       (void);

void     iog_job_queue_add_job    (IogJob    *job);

gboolean iog_job_queue_remove_job (IogJob    *job);

G_END_DECLS

#endif /* __IOG_JOB_QUEUE_H__ */
