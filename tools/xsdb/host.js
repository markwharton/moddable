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
	The host seam: everything xsdb needs from the operating system, implemented natively in host/xsdbhost.c
	(POSIX) with a run loop per platform (mac/, lin/). Keep this surface small; a new platform implements exactly these functions.
*/

export class Host {
	static get arguments() @ "xs_host_arguments";		// argv, including the executable
	static getenv(name) @ "xs_host_getenv";			// string or undefined
	static cwd() @ "xs_host_cwd";
	static tmpdir() @ "xs_host_tmpdir";
	static exit(code) @ "xs_host_exit";				// restores the terminal, stops children
	static write(...strings) @ "xs_host_write";		// stdout
	static columns() @ "xs_host_columns";
	static isTTY() @ "xs_host_isTTY";
	static setStdin(callback) @ "xs_host_setStdin";		// callback(ArrayBuffer | null at end of input)
	static setChildExit(callback) @ "xs_host_setChildExit";	// callback(pid, code)
	static spawn(path, args, stdout) @ "xs_host_spawn";	// stdout: "ignore" | "inherit"; returns pid
	static kill(pid, signal) @ "xs_host_kill";
	static execute(path, args, timeout) @ "xs_host_execute";	// blocks; { status, stdout, stderr }

	static get platform() @ "xs_host_platform";		// "mac" | "lin" | "win"

	static join(...parts) {
		return parts.filter(part => part !== undefined && part !== "").join("/").replaceAll(/\/+/g, "/");
	}

	// absolute path for input, resolved against the working directory, with "." and ".." removed;
	// forward slashes throughout, which Windows accepts too
	static resolve(input) {
		input = input.replaceAll("\\", "/");
		const absolute = input.startsWith("/") || /^[A-Za-z]:/.test(input);
		let path = absolute ? input : Host.join(Host.cwd().replaceAll("\\", "/"), input);
		const drive = /^[A-Za-z]:/.exec(path)?.[0] ?? "";
		path = path.slice(drive.length);
		const result = [];
		for (const part of path.split("/")) {
			if ((part === "") || (part === "."))
				continue;
			if (part === "..")
				result.pop();
			else
				result.push(part);
		}
		return drive + "/" + result.join("/");
	}
}

export default Host;
