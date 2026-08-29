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
	macOS run loop for xsdb: file descriptors are CFFileDescriptor sources on the main CFRunLoop,
	which is also where the mac socket and timer modules deliver their callbacks.
*/

#include "xsdbhost.h"

#include <CoreFoundation/CoreFoundation.h>

#define kMaxWatches 8

typedef struct {
	int fd;
	xsdbWatchCallback callback;
	CFFileDescriptorRef descriptor;
} xsdbWatchRecord;

static xsdbWatchRecord gWatches[kMaxWatches];

const char *xsdbPlatform = "mac";

static void descriptorCallback(CFFileDescriptorRef descriptor, CFOptionFlags callBackTypes, void *info)
{
	xsdbWatchRecord *watch = info;
	watch->callback(watch->fd);
	// CFFileDescriptor callbacks are one-shot; re-arm unless the callback unwatched the descriptor
	if (watch->descriptor)
		CFFileDescriptorEnableCallBacks(watch->descriptor, kCFFileDescriptorReadCallBack);
}

void xsdbWatch(int fd, xsdbWatchCallback callback)
{
	int i;
	for (i = 0; i < kMaxWatches; i++) {
		xsdbWatchRecord *watch = &gWatches[i];
		if (watch->descriptor)
			continue;
		CFFileDescriptorContext context = {0, watch, NULL, NULL, NULL};
		CFRunLoopSourceRef source;
		watch->fd = fd;
		watch->callback = callback;
		watch->descriptor = CFFileDescriptorCreate(kCFAllocatorDefault, fd, false, descriptorCallback, &context);
		source = CFFileDescriptorCreateRunLoopSource(kCFAllocatorDefault, watch->descriptor, 0);
		CFRunLoopAddSource(CFRunLoopGetMain(), source, kCFRunLoopCommonModes);
		CFRelease(source);
		CFFileDescriptorEnableCallBacks(watch->descriptor, kCFFileDescriptorReadCallBack);
		return;
	}
}

void xsdbUnwatch(int fd)
{
	int i;
	for (i = 0; i < kMaxWatches; i++) {
		xsdbWatchRecord *watch = &gWatches[i];
		if (watch->descriptor && (watch->fd == fd)) {
			CFFileDescriptorInvalidate(watch->descriptor);
			CFRelease(watch->descriptor);
			watch->descriptor = NULL;
			return;
		}
	}
}

void xsdbRunLoop(void)
{
	CFRunLoopRun();
}
