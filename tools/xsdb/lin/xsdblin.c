/*
 * Copyright (c) 2026  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Tools.
 *
 *   The Moddable SDK Tools is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   The Moddable SDK Tools is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
	Linux run loop for xsdb: file descriptors are GLib unix fd sources on the default main context,
	which is also where lin_xs.c queues promise jobs and the lin socket and timer modules run.
*/

#include "xsdbhost.h"

#include <glib.h>
#include <glib-unix.h>

#define kMaxWatches 8

typedef struct {
	int fd;
	xsdbWatchCallback callback;
	guint source;
} xsdbWatchRecord;

static xsdbWatchRecord gWatches[kMaxWatches];
static GMainLoop *gLoop = NULL;

const char *xsdbPlatform = "lin";

static gboolean descriptorCallback(gint fd, GIOCondition condition, gpointer data)
{
	xsdbWatchRecord *watch = data;
	watch->callback(watch->fd);
	return watch->source ? G_SOURCE_CONTINUE : G_SOURCE_REMOVE;
}

void xsdbWatch(int fd, xsdbWatchCallback callback)
{
	int i;
	for (i = 0; i < kMaxWatches; i++) {
		xsdbWatchRecord *watch = &gWatches[i];
		if (watch->source)
			continue;
		watch->fd = fd;
		watch->callback = callback;
		watch->source = g_unix_fd_add(fd, G_IO_IN | G_IO_HUP | G_IO_ERR, descriptorCallback, watch);
		return;
	}
}

void xsdbUnwatch(int fd)
{
	int i;
	for (i = 0; i < kMaxWatches; i++) {
		xsdbWatchRecord *watch = &gWatches[i];
		if (watch->source && (watch->fd == fd)) {
			// clearing source makes the running callback return G_SOURCE_REMOVE; remove explicitly otherwise
			guint source = watch->source;
			watch->source = 0;
			if (!g_main_current_source() || (g_source_get_id(g_main_current_source()) != source))
				g_source_remove(source);
			return;
		}
	}
}

void xsdbRunLoop(void)
{
	gLoop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(gLoop);
}
