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
	A line editor over a raw terminal: prompt, history, cursor keys, tab completion, and questions.
	The surface mirrors the parts of Node's readline that xsdb used (prompt, setPrompt, line, question,
	write, close, and the line / close / interrupt events), so the machines are unchanged above it.
	Derived from examples/js/repl/replcore.js.
*/

import Host from "host";

const backspace = "\b";
const newline = "\r\n";
const historyLimit = 100;

export class LineEditor {
	history = [];
	line = "";
	position = 0;
	completionsShown = false;
	#prompt;
	#completer;
	#escape = "";
	#utf8 = [];
	#historyPosition;
	#postHistory;
	#question;
	#tty = true;

	onLine = line => {};
	onClose = () => {};
	onInterrupt = () => {};

	// completer(line, callback) with callback(error, [completions, matched]), as readline
	constructor({ prompt = "> ", completer } = {}) {
		this.#prompt = prompt;
		this.#completer = completer;
	}

	start() {
		this.#tty = Host.isTTY();
		Host.setStdin(bytes => {
			if (null === bytes) {
				this.onClose();
				return;
			}
			this.#receive(new Uint8Array(bytes));
		});
	}

	close() {
	}

	write(...strings) {
		Host.write(...strings);
	}

	// output that only makes sense on a terminal: echo, cursor movement, redraws
	#echo(...strings) {
		if (this.#tty)
			Host.write(...strings);
	}

	setPrompt(prompt) {
		this.#prompt = prompt;
	}

	get promptText() {
		return this.#question ?? this.#prompt;
	}

	// redraw the prompt and, when preserve is true, the line being edited (readline's prompt(true))
	prompt(preserve) {
		if (!preserve) {
			this.line = "";
			this.position = 0;
		}
		if (!this.#tty) {
			this.write(this.promptText);
			return;
		}
		this.write("\r\x1B[2K", this.promptText, this.line);
		const back = this.line.length - this.position;
		if (back > 0)
			this.write(backspace.repeat(back));
	}

	// ask once; the answer goes to callback instead of onLine, and no prompt is redrawn afterwards
	question(text, callback) {
		this.#question = text;
		this.#questionCallback = callback;
		this.line = "";
		this.position = 0;
		this.write(text);
	}
	#questionCallback;

	#receive(bytes) {
		for (const byte of bytes)
			this.#byte(byte);
	}

	#byte(c) {
		if (this.#escape) {
			this.#escape += String.fromCharCode(c);
			if (this.#escape === "\x1B[") {
				return;
			}
			const sequence = this.#escape;
			// wait for the final byte of a CSI sequence (parameters are digits and semicolons)
			if (sequence.startsWith("\x1B[") && (c >= 0x30) && (c <= 0x3F))
				return;
			this.#escape = "";
			this.#escapeSequence(sequence);
			return;
		}

		if (c >= 0x80) {
			this.#utf8.push(c);
			const lead = this.#utf8[0];
			const need = (lead >= 0xF0) ? 4 : (lead >= 0xE0) ? 3 : (lead >= 0xC0) ? 2 : 1;
			if (this.#utf8.length < need)
				return;
			const text = String.fromArrayBuffer(new Uint8Array(this.#utf8).buffer);
			this.#utf8.length = 0;
			this.#insert(text);
			return;
		}
		this.#utf8.length = 0;

		if (this.completionsShown && (c !== 9))
			this.#hideCompletions();

		switch (c) {
			case 13:
			case 10:
				this.#enter();
				return;
			case 3:		// ^C
				this.#echo("^C", newline);
				this.line = "";
				this.position = 0;
				this.#dismissQuestion();
				this.onInterrupt();
				return;
			case 4:		// ^D
				if (this.line.length === 0) {
					this.#echo(newline);
					this.onClose();
				}
				return;
			case 9:		// tab
				this.#complete();
				return;
			case 27:
				this.#escape = "\x1B";
				return;
			case 8:
			case 127:
				this.#delete();
				return;
			case 1:		// ^A
				this.#moveTo(0);
				return;
			case 5:		// ^E
				this.#moveTo(this.line.length);
				return;
			case 21:	// ^U
				this.#moveTo(0);
				this.#echo("\x1B[K");
				this.line = "";
				return;
			case 11:	// ^K
				this.#echo("\x1B[K");
				this.line = this.line.slice(0, this.position);
				return;
		}
		if (c < 32)
			return;
		this.#insert(String.fromCharCode(c));
	}

	#escapeSequence(sequence) {
		switch (sequence) {
			case "\x1B[A":
				this.#historyStep(+1);
				break;
			case "\x1B[B":
				this.#historyStep(-1);
				break;
			case "\x1B[C":
				if (this.position < this.line.length)
					this.#moveTo(this.position + 1);
				break;
			case "\x1B[D":
				if (this.position > 0)
					this.#moveTo(this.position - 1);
				break;
			case "\x1B[H":
				this.#moveTo(0);
				break;
			case "\x1B[F":
				this.#moveTo(this.line.length);
				break;
			case "\x1B[3~":		// forward delete
				if (this.position < this.line.length) {
					this.#moveTo(this.position + 1);
					this.#delete();
				}
				break;
		}
	}

	#moveTo(position) {
		const delta = position - this.position;
		if (delta < 0)
			this.#echo(backspace.repeat(-delta));
		else if (delta > 0)
			this.#echo(this.line.slice(this.position, position));
		this.position = position;
	}

	#insert(text) {
		const tail = this.line.slice(this.position);
		this.line = this.line.slice(0, this.position) + text + tail;
		this.#echo(text, tail, backspace.repeat(tail.length));
		this.position += text.length;
	}

	#delete() {
		if (this.position === 0)
			return;
		const tail = this.line.slice(this.position);
		this.line = this.line.slice(0, this.position - 1) + tail;
		this.position -= 1;
		this.#echo(backspace, tail, " ", backspace.repeat(tail.length + 1));
	}

	#replaceLine(text) {
		this.#moveTo(0);
		this.#echo("\x1B[K", text);
		this.line = text;
		this.position = text.length;
	}

	#historyStep(direction) {
		if (!this.history.length)
			return;
		if (undefined === this.#historyPosition) {
			this.#historyPosition = 0;
			this.#postHistory = this.line;
		}
		this.#historyPosition += direction;
		if (this.#historyPosition < 0)
			this.#historyPosition = 0;
		else if (this.#historyPosition > this.history.length)
			this.#historyPosition = this.history.length;
		if (0 === this.#historyPosition)
			this.#replaceLine(this.#postHistory);
		else
			this.#replaceLine(this.history[this.history.length - this.#historyPosition]);
	}

	#enter() {
		const line = this.line;
		this.#echo(newline);
		this.line = "";
		this.position = 0;
		this.#historyPosition = undefined;
		this.#postHistory = undefined;

		if (this.#question !== undefined) {
			const callback = this.#questionCallback;
			this.#question = undefined;
			this.#questionCallback = undefined;
			callback(line);
			return;
		}

		if (line.trim() && (this.history[this.history.length - 1] !== line)) {
			this.history.push(line);
			while (this.history.length > historyLimit)
				this.history.shift();
		}
		this.onLine(line);
	}

	#dismissQuestion() {
		this.#question = undefined;
		this.#questionCallback = undefined;
	}

	// tab: complete a unique match, extend to the common prefix, or show the candidates below the line
	#complete() {
		if (!this.#completer || this.completionsShown || (this.#question !== undefined))
			return;
		const line = this.line.slice(0, this.position);
		this.#completer(line, (error, result) => {
			if (error || !result)
				return;
			let [completions, matched] = result;
			if (!completions.length)
				return;
			if (completions.length === 1) {
				if (completions[0] !== matched)
					this.#insert(completions[0].slice(matched.length));
				return;
			}
			const prefix = commonPrefix(completions);
			if (prefix.length > matched.length) {
				this.#insert(prefix.slice(matched.length));
				return;
			}
			this.#showCompletions(completions);
		});
	}

	#showCompletions(completions) {
		const width = Host.columns();
		const maxLength = Math.max(...completions.map(c => c.length)) + 2;
		const columns = Math.max(1, Math.floor(width / maxLength));
		let out = "";
		for (let i = 0; i < completions.length; i++) {
			out += completions[i].padEnd(maxLength);
			if ((i + 1) % columns === 0)
				out += newline;
		}
		if (completions.length % columns !== 0)
			out += newline;
		const linesDown = (out.match(/\n/g) || []).length + 1;
		this.write(newline, out, `\x1B[${linesDown}A`);
		this.write(`\r\x1B[${this.#column()}C`);
		this.completionsShown = true;
	}

	#hideCompletions() {
		this.write(`${newline}\x1B[J\x1B[A\r\x1B[${this.#column()}C`);
		this.completionsShown = false;
	}

	#column() {
		return this.promptText.length + this.position;
	}
}

function commonPrefix(strings) {
	let prefix = strings[0];
	for (const string of strings) {
		while (!string.startsWith(prefix))
			prefix = prefix.slice(0, -1);
	}
	return prefix;
}

export default LineEditor;
