/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 * 
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 * 
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

export default class Resource @ "Resource_destructor" {
	constructor(path) @ "Resource_constructor";
	static exists(path) @ "Resource_exists";
	slice(begin, end) @ "Resource_slice";
}
Object.freeze(Resource.prototype);

function require_awaitImport(specifier) @ "require_awaitImport";
function require(specifier) {
	return require_awaitImport(specifier);
}
Object.defineProperties(require, {
	cache: { get: function() { debugger; } },
	resolve: { value: function() { debugger; } },
	uri: { get: function() { debugger; } },
	weak: { value: function(specifier) { return require_awaitImport(specifier); } },
});
global.require = Object.freeze(require);
