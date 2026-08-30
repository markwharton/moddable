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
	each arriving in pieces of arbitrary size. Framer keeps the pieces, cuts one document at a time
	at its terminator (copying each byte once), parses it with modxml, and replays the tree into the Machine's SAX-style
	callbacks (onStartElement, onCharacterData, onEndElement) so the Machine is unchanged from the
	streaming-parser days.
*/

import XML from "xml";

const terminator = new Uint8Array(ArrayBuffer.fromString("</xsbug>\r\n"));

export class Framer {
	#machine;
	#chunks = [];		// the pieces of the message in progress, each copied once into the final buffer
	#length = 0;
	#tail = new Uint8Array(0);	// the last bytes of the pending data, so a terminator split across reads is found

	constructor(machine) {
		this.#machine = machine;
	}

	write(bytes) {
		let chunk = new Uint8Array(bytes);
		while (chunk.length) {
			const carry = this.#tail;
			const window = new Uint8Array(carry.length + chunk.length);
			window.set(carry);
			window.set(chunk, carry.length);
			const hit = find(window);
			if (hit < 0) {
				this.#push(chunk);
				this.#tail = window.slice(Math.max(0, window.length - (terminator.length - 1)));
				return;
			}
			// the terminator starts at hit in window; carry is shorter than the terminator, so it ends inside chunk
			const end = hit + terminator.length - carry.length;
			this.#push(chunk.subarray(0, end));
			this.#deliver(this.#take());
			this.#tail = new Uint8Array(0);
			chunk = chunk.subarray(end);
		}
	}

	#push(chunk) {
		this.#chunks.push(chunk);
		this.#length += chunk.length;
	}

	// the pending chunks as one buffer, and reset
	#take() {
		const buffer = new Uint8Array(this.#length);
		let offset = 0;
		for (const chunk of this.#chunks) {
			buffer.set(chunk, offset);
			offset += chunk.length;
		}
		this.#chunks = [];
		this.#length = 0;
		return buffer;
	}

	#deliver(bytes) {
		const message = String.fromArrayBuffer(bytes.buffer);
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

// index of the terminator in buffer, or -1
function find(buffer) {
	const last = buffer.length - terminator.length;
	outer: for (let i = 0; i <= last; i++) {
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
