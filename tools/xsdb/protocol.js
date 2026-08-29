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
	The xsbug wire protocol, debuggee to debugger: a stream of "\r\n<xsbug>...</xsbug>\r\n" documents,
	each arriving in pieces of arbitrary size. Framer collects the bytes, cuts one document at a time
	at its terminator, parses it with modxml, and replays the tree into the Machine's SAX-style
	callbacks (onStartElement, onCharacterData, onEndElement) so the Machine is unchanged from the
	streaming-parser days.
*/

import XML from "xml";

const terminator = new Uint8Array(ArrayBuffer.fromString("</xsbug>\r\n"));

export class Framer {
	#machine;
	#buffer = new Uint8Array(0);

	constructor(machine) {
		this.#machine = machine;
	}

	write(bytes) {
		const chunk = new Uint8Array(bytes);
		const scanFrom = Math.max(0, this.#buffer.length - terminator.length + 1);
		const joined = new Uint8Array(this.#buffer.length + chunk.length);
		joined.set(this.#buffer);
		joined.set(chunk, this.#buffer.length);
		this.#buffer = joined;

		let start = 0;
		let end = this.#find(scanFrom);
		while (end >= 0) {
			const message = String.fromArrayBuffer(this.#buffer.slice(start, end + terminator.length).buffer);
			this.#deliver(message);
			start = end + terminator.length;
			end = this.#find(start);
		}
		if (start > 0)
			this.#buffer = this.#buffer.slice(start);
	}

	// index of the next terminator at or after from, or -1
	#find(from) {
		const buffer = this.#buffer;
		const last = buffer.length - terminator.length;
		outer: for (let i = from; i <= last; i++) {
			if (buffer[i] !== terminator[0])
				continue;
			for (let j = 1; j < terminator.length; j++) {
				if (buffer[i + j] !== terminator[j])
					continue outer;
			}
			return i;
		}
		return -1;
	}

	#deliver(message) {
		let document;
		try {
			document = XML.parse(message, false);
		}
		catch (e) {
			this.#machine.onError(e, message);
			return;
		}
		replay(document, this.#machine);
	}
}

// walk a modxml tree in document order, firing the SAX-style callbacks
function replay(element, machine) {
	const attributes = {};
	if (element.attributes) {
		for (const attribute of element.attributes)
			attributes[attribute.name] = attribute.value ?? "";
	}
	machine.onStartElement(element.name, attributes);
	if (element.elements) {
		for (const child of element.elements) {
			if (child.name === undefined)
				machine.onCharacterData(child.text);
			else
				replay(child, machine);
		}
	}
	machine.onEndElement(element.name);
}

export default Framer;
