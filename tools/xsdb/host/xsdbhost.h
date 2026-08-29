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
	What a platform supplies to the POSIX host (host/xsdbhost.c): a way to be called when a file
	descriptor is readable, and the run loop that delivers those calls, timers, and sockets.
*/

#ifndef __XSDBHOST_H__
#define __XSDBHOST_H__

typedef void (*xsdbWatchCallback)(int fd);

extern const char *xsdbPlatform;

// call callback on the run loop thread whenever fd is readable (or at end of file), until unwatched
extern void xsdbWatch(int fd, xsdbWatchCallback callback);
extern void xsdbUnwatch(int fd);

// run the platform's main loop; returns only if the loop is stopped (xsdb normally exits from inside it)
extern void xsdbRunLoop(void);

#endif
