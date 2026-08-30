#!/usr/bin/env python3
#
# Copyright (c) 2026  Moddable Tech, Inc.
#
#   This file is part of the Moddable SDK Tools.
#
#   The Moddable SDK Tools is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   The Moddable SDK Tools is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
#
"""
Smoke test for xsdb without a device: start xsdb, connect as a fake debuggee, and check
what comes out of both ends.

    python3 fakedebuggee.py <path-to-xsdb> [port]

Exercises: the listener, launching a child, protocol framing across arbitrary sends,
character reference decoding, whitespace in log text, a malformed message, thread listing,
breakpoints with a condition compiled through xsc (xsc must be on PATH), disconnect, quit.
Exit status is non-zero when an expectation fails.
"""
import os
import select
import socket
import subprocess
import sys
import tempfile
import threading
import time

xsdb = os.path.abspath(sys.argv[1])
port = int(sys.argv[2]) if len(sys.argv) > 2 else 5580
work = tempfile.mkdtemp(prefix="xsdb-test-")

# a child that outlives the session, to check that xsdb stops it on exit
child = [sys.executable, "-c", "import time; time.sleep(60)"]

env = dict(os.environ, XSBUG_LOG_PORT=str(port), XSBUG_PORT=str(port), XSBUG_HOST="localhost", XSBUG_PROJECT=work)
proc = subprocess.Popen([xsdb] + child, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=env, cwd=work)

output = []
def pump():
	for line in iter(proc.stdout.readline, b""):
		output.append(line.decode(errors="replace"))
		sys.stdout.write("xsdb> " + output[-1])
		sys.stdout.flush()
threading.Thread(target=pump, daemon=True).start()

def send(text):
	proc.stdin.write(text.encode())
	proc.stdin.flush()

def connect():
	deadline = time.time() + 10
	while time.time() < deadline:
		try:
			return socket.create_connection(("127.0.0.1", port))
		except OSError:
			time.sleep(0.2)
	raise SystemExit("xsdb did not listen on port %d" % port)

s = connect()

def recv(timeout=1.0):
	data = b""
	while True:
		ready, _, _ = select.select([s], [], [], timeout)
		if not ready:
			break
		chunk = s.recv(4096)
		if not chunk:
			break
		data += chunk
	return data.decode(errors="replace")

sent = []

# login, split across sends to exercise the framer
message = b'\r\n<xsbug><login name="fake" value="9.0.0 8"/></xsbug>\r\n'
s.sendall(message[:7]); time.sleep(0.05); s.sendall(message[7:20]); time.sleep(0.05); s.sendall(message[20:])
sent.append(recv())

# a log with leading spaces and character references, terminator split from the body
message = b'\r\n<xsbug><log path="/x/main.js" line="3">  hello &#60;world&#62; &#38; friends\n</log></xsbug>\r\n'
s.sendall(message[:-4]); time.sleep(0.05); s.sendall(message[-4:])
time.sleep(0.3)

# two messages in one send
s.sendall(b'\r\n<xsbug><log>one\n</log></xsbug>\r\n\r\n<xsbug><log>two\n</log></xsbug>\r\n')
time.sleep(0.3)

# a malformed message, then a good one
s.sendall(b'\r\n<xsbug><log>bad</xsbug>\r\n\r\n<xsbug><log>after\n</log></xsbug>\r\n')
time.sleep(0.3)

send("info threads\n"); time.sleep(0.3)
send("break /x/main.js:3\n"); time.sleep(0.3)
send("condition 1 x > 1\n"); time.sleep(3)
sent.append(recv(0.5))
send("info break\n"); time.sleep(0.3)
send("break /x/main.js:9\r\n"); time.sleep(0.3)
s.close(); time.sleep(0.5)
send("quit\n")
try:
	code = proc.wait(timeout=5)
except subprocess.TimeoutExpired:
	proc.kill()
	code = "killed"

text = "".join(output)
commands = "".join(sent)
failures = []
def expect(condition, what):
	if not condition:
		failures.append(what)

expect("listening on port %d" % port in text, "listening message")
expect('Connected to "fake"' in text, "login handled")
expect("<go/>" in commands and "<set-all-breakpoints>" in commands, "go and set-all-breakpoints sent after login")
expect("/x/main.js (3)   hello <world> & friends" in text, "log text decoded with leading spaces kept")
expect("[Thread 1] one" in text and "[Thread 1] two" in text, "two messages in one read")
expect("bad message" in text and "[Thread 1] after" in text, "malformed message reported and the session continues")
expect("fake" in text and "Running" in text, "info threads")
expect("stop only if x > 1" in text, "breakpoint condition accepted")
expect("<breakpoint-condition path=" in commands, "condition compiled through xsc and sent")
expect('disconnected from "fake"' in text, "disconnect reported")
expect(text.count("Breakpoint 2 set at main.js:9") == 1, "a CRLF-terminated command runs exactly once (got %d)" % text.count("Breakpoint 2 set at main.js:9"))
expect(code == 0, "exit status 0 (got %r)" % code)
expect(os.path.exists(os.path.join(work, ".xsdb.json")), ".xsdb.json written")

if failures:
	print("FAILED:")
	for failure in failures:
		print("  " + failure)
	sys.exit(1)
print("fakedebuggee: all expectations met")
