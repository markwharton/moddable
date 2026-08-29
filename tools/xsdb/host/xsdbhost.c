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
	POSIX host for xsdb: main() and the natives behind the "host" module. The run loop is supplied by
	the platform file (mac/xsdbmac.c, lin/xsdblin.c) through xsdbWatch, xsdbUnwatch, and xsdbRunLoop.
*/

#include "xsAll.h"
#include "mc.xs.h"
#include "xsdbhost.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <spawn.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

extern char **environ;

#define kMaxChildren 16

static int gArgc;
static char **gArgv;

static struct termios gTIO;
static int gRawTerminal = 0;

static pid_t gChildren[kMaxChildren];
static int gChildCount = 0;
static int gChildPipe[2] = {-1, -1};

static xsMachine *gMachine = NULL;
static xsSlot gStdinCallback;
static xsSlot gChildCallback;
static int gStdinWatched = 0;
static int gChildWatched = 0;

/*
	process lifetime
*/

static void restoreTerminal(void)
{
	if (gRawTerminal) {
		tcsetattr(STDIN_FILENO, TCSANOW, &gTIO);
		gRawTerminal = 0;
	}
}

static void killChildren(void)
{
	int i;
	for (i = 0; i < gChildCount; i++)
		kill(gChildren[i], SIGTERM);
	gChildCount = 0;
}

static void onTerminatingSignal(int signal)
{
	// atexit handlers restore the terminal and stop the children
	exit(128 + signal);
}

static void trackChild(pid_t pid)
{
	if (gChildCount < kMaxChildren)
		gChildren[gChildCount++] = pid;
}

static void untrackChild(pid_t pid)
{
	int i;
	for (i = 0; i < gChildCount; i++) {
		if (gChildren[i] == pid) {
			gChildren[i] = gChildren[--gChildCount];
			return;
		}
	}
}

void fxAbort(xsMachine* the, int status)
{
	if (XS_DEBUGGER_EXIT == status)
		exit(1);
	fprintf(stderr, "xsdb: %s\n", fxAbortString(status));
	exit(status);
}

/*
	arguments, environment, terminal
*/

void xs_host_arguments(xsMachine *the)
{
	int i;
	xsResult = xsNewArray(0);
	for (i = 0; i < gArgc; i++)
		xsSetIndex(xsResult, i, xsString(gArgv[i]));
}

void xs_host_getenv(xsMachine *the)
{
	char *value = getenv(xsToString(xsArg(0)));
	if (value)
		xsResult = xsString(value);
}

void xs_host_cwd(xsMachine *the)
{
	char path[PATH_MAX];
	if (getcwd(path, sizeof(path)))
		xsResult = xsString(path);
}

void xs_host_tmpdir(xsMachine *the)
{
	char *value = getenv("TMPDIR");
	size_t length;
	if (!value || !value[0])
		value = "/tmp";
	xsResult = xsString(value);
	length = strlen(value);
	if ((length > 1) && ('/' == value[length - 1])) {
		char *copy = xsToString(xsResult);
		copy[length - 1] = 0;
	}
}

void xs_host_exit(xsMachine *the)
{
	exit((xsToInteger(xsArgc) > 0) ? xsToInteger(xsArg(0)) : 0);
}

void xs_host_write(xsMachine *the)
{
	int argc = xsToInteger(xsArgc), i;
	for (i = 0; i < argc; i++) {
		char *string = xsToString(xsArg(i));
		fwrite(string, 1, strlen(string), stdout);
	}
	fflush(stdout);
}

void xs_host_columns(xsMachine *the)
{
	struct winsize size;
	if ((0 == ioctl(STDOUT_FILENO, TIOCGWINSZ, &size)) && size.ws_col)
		xsResult = xsInteger(size.ws_col);
	else
		xsResult = xsInteger(80);
}

void xs_host_isTTY(xsMachine *the)
{
	xsResult = xsBoolean(isatty(STDIN_FILENO));
}

void xs_host_platform(xsMachine *the)
{
	xsResult = xsString(xsdbPlatform);
}

/*
	stdin as a run loop source: the callback receives an ArrayBuffer of bytes, or null at end of input
*/

static void stdinCallback(int fd)
{
	char buffer[4096];
	ssize_t count = read(STDIN_FILENO, buffer, sizeof(buffer));
	if (count < 0) {
		if ((EAGAIN == errno) || (EINTR == errno))
			return;
		count = 0;
	}
	if (count <= 0) {
		xsdbUnwatch(STDIN_FILENO);
		gStdinWatched = 0;
	}
	xsBeginHost(gMachine);
	{
		xsVars(1);
		if (count > 0)
			xsVar(0) = xsArrayBuffer(buffer, (xsIntegerValue)count);
		else
			xsVar(0) = xsNull;
		xsCallFunction1(gStdinCallback, xsUndefined, xsVar(0));
	}
	xsEndHost(gMachine);
}

void xs_host_setStdin(xsMachine *the)
{
	gMachine = the;
	if (gStdinWatched) {
		// replace the callback; the descriptor and its run loop source stay
		xsForget(gStdinCallback);
		gStdinCallback = xsArg(0);
		xsRemember(gStdinCallback);
		return;
	}
	gStdinCallback = xsArg(0);
	xsRemember(gStdinCallback);

	xsdbWatch(STDIN_FILENO, stdinCallback);
	gStdinWatched = 1;
}

/*
	child processes
*/

static void onChildSignal(int signal)
{
	char byte = 0;
	int error = errno;
	if ((gChildPipe[1] >= 0) && (write(gChildPipe[1], &byte, 1) < 0))
		; // nothing to do; the reaper runs on the next wakeup
	errno = error;
}

static void childCallback(int fd)
{
	char drain[64];
	pid_t pid;
	int status;

	while (read(gChildPipe[0], drain, sizeof(drain)) > 0)
		;

	while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
		int code;
		untrackChild(pid);
		if (WIFEXITED(status))
			code = WEXITSTATUS(status);
		else if (WIFSIGNALED(status))
			code = 128 + WTERMSIG(status);
		else
			continue;
		xsBeginHost(gMachine);
		{
			xsCallFunction2(gChildCallback, xsUndefined, xsInteger(pid), xsInteger(code));
		}
		xsEndHost(gMachine);
	}
}

void xs_host_setChildExit(xsMachine *the)
{
	struct sigaction action;

	if (gChildWatched)
		xsUnknownError("child exit callback already set");

	gMachine = the;
	gChildCallback = xsArg(0);
	xsRemember(gChildCallback);

	if (pipe(gChildPipe) < 0)
		xsUnknownError("pipe failed");
	fcntl(gChildPipe[0], F_SETFL, fcntl(gChildPipe[0], F_GETFL) | O_NONBLOCK);
	fcntl(gChildPipe[1], F_SETFL, fcntl(gChildPipe[1], F_GETFL) | O_NONBLOCK);

	xsdbWatch(gChildPipe[0], childCallback);
	gChildWatched = 1;

	memset(&action, 0, sizeof(action));
	action.sa_handler = onChildSignal;
	action.sa_flags = SA_RESTART | SA_NOCLDSTOP;
	sigemptyset(&action.sa_mask);
	sigaction(SIGCHLD, &action, NULL);
}

// copies argv out of the machine so the strings are stable across allocations
static char **copyArguments(xsMachine *the, xsSlot path, xsSlot array, int *argcOut)
{
	int count, i;
	char **argv;

	count = xsToInteger(xsGet(array, xsID("length")));
	argv = calloc(count + 2, sizeof(char *));
	if (!argv)
		xsUnknownError("not enough memory");
	argv[0] = strdup(xsToString(path));
	for (i = 0; i < count; i++)
		argv[i + 1] = strdup(xsToString(xsGetIndex(array, i)));
	argv[count + 1] = NULL;
	*argcOut = count + 1;
	return argv;
}

static void freeArguments(char **argv)
{
	int i;
	for (i = 0; argv[i]; i++)
		free(argv[i]);
	free(argv);
}

// spawn(path, [args], "ignore" | "inherit") -> pid. stdin is /dev/null; stderr is inherited.
void xs_host_spawn(xsMachine *the)
{
	posix_spawn_file_actions_t actions;
	char **argv;
	int argc, error, ignoreStdout = 1;
	pid_t pid;

	if (xsToInteger(xsArgc) > 2)
		ignoreStdout = (0 == strcmp(xsToString(xsArg(2)), "ignore"));

	argv = copyArguments(the, xsArg(0), xsArg(1), &argc);

	posix_spawn_file_actions_init(&actions);
	posix_spawn_file_actions_addopen(&actions, STDIN_FILENO, "/dev/null", O_RDONLY, 0);
	if (ignoreStdout)
		posix_spawn_file_actions_addopen(&actions, STDOUT_FILENO, "/dev/null", O_WRONLY, 0);

	error = posix_spawnp(&pid, argv[0], &actions, NULL, argv, environ);

	posix_spawn_file_actions_destroy(&actions);
	freeArguments(argv);

	if (error)
		xsUnknownError("spawn failed: %s", strerror(error));

	trackChild(pid);
	xsResult = xsInteger(pid);
}

void xs_host_kill(xsMachine *the)
{
	pid_t pid = xsToInteger(xsArg(0));
	int signal = (xsToInteger(xsArgc) > 1) ? xsToInteger(xsArg(1)) : SIGTERM;
	if (0 == kill(pid, signal))
		untrackChild(pid);
}

static double now(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec + (ts.tv_nsec / 1e9);
}

typedef struct {
	char *data;
	size_t length;
	size_t size;
} txBuffer;

static void appendBuffer(txBuffer *buffer, const char *data, size_t length)
{
	if (buffer->length + length + 1 > buffer->size) {
		size_t size = (buffer->size ? buffer->size * 2 : 1024);
		while (size < buffer->length + length + 1)
			size *= 2;
		buffer->data = realloc(buffer->data, size);
		buffer->size = size;
	}
	memcpy(buffer->data + buffer->length, data, length);
	buffer->length += length;
	buffer->data[buffer->length] = 0;
}

// execute(path, [args], timeoutMilliseconds) -> { status, stdout, stderr }. Blocks until the child exits.
// status is the exit code, 128 + signal, or -1 when the timeout killed the child.
void xs_host_execute(xsMachine *the)
{
	posix_spawn_file_actions_t actions;
	char **argv;
	int argc, error, status = -1, timeout = -1;
	int outPipe[2], errPipe[2];
	pid_t pid;
	txBuffer out = {NULL, 0, 0}, err = {NULL, 0, 0};
	struct pollfd descriptors[2];
	int open = 2;
	int timedOut = 0;
	double deadline = 0;

	if (xsToInteger(xsArgc) > 2)
		timeout = xsToInteger(xsArg(2));

	argv = copyArguments(the, xsArg(0), xsArg(1), &argc);

	if ((pipe(outPipe) < 0) || (pipe(errPipe) < 0)) {
		freeArguments(argv);
		xsUnknownError("pipe failed");
	}

	posix_spawn_file_actions_init(&actions);
	posix_spawn_file_actions_addopen(&actions, STDIN_FILENO, "/dev/null", O_RDONLY, 0);
	posix_spawn_file_actions_adddup2(&actions, outPipe[1], STDOUT_FILENO);
	posix_spawn_file_actions_adddup2(&actions, errPipe[1], STDERR_FILENO);
	posix_spawn_file_actions_addclose(&actions, outPipe[0]);
	posix_spawn_file_actions_addclose(&actions, errPipe[0]);

	error = posix_spawnp(&pid, argv[0], &actions, NULL, argv, environ);

	posix_spawn_file_actions_destroy(&actions);
	freeArguments(argv);
	close(outPipe[1]);
	close(errPipe[1]);

	if (error) {
		close(outPipe[0]);
		close(errPipe[0]);
		xsUnknownError("execute failed: %s", strerror(error));
	}

	descriptors[0].fd = outPipe[0];
	descriptors[0].events = POLLIN;
	descriptors[1].fd = errPipe[0];
	descriptors[1].events = POLLIN;
	if (timeout >= 0)
		deadline = now() + (timeout / 1000.0);

	while (open > 0) {
		int remaining = -1, ready, i;
		if (timeout >= 0) {
			remaining = (int)((deadline - now()) * 1000);
			if (remaining <= 0) {
				timedOut = 1;
				break;
			}
		}
		ready = poll(descriptors, 2, remaining);
		if (ready < 0) {
			if (EINTR == errno)
				continue;
			break;
		}
		if (0 == ready) {
			timedOut = 1;
			break;
		}
		for (i = 0; i < 2; i++) {
			char chunk[4096];
			ssize_t count;
			if (descriptors[i].fd < 0)
				continue;
			if (!(descriptors[i].revents & (POLLIN | POLLHUP | POLLERR)))
				continue;
			count = read(descriptors[i].fd, chunk, sizeof(chunk));
			if (count > 0)
				appendBuffer(i ? &err : &out, chunk, count);
			else {
				close(descriptors[i].fd);
				descriptors[i].fd = -1;
				open--;
			}
		}
	}

	if (descriptors[0].fd >= 0) close(descriptors[0].fd);
	if (descriptors[1].fd >= 0) close(descriptors[1].fd);

	if (timedOut)
		kill(pid, SIGKILL);

	while (waitpid(pid, &status, 0) < 0) {
		if (EINTR != errno)
			break;
	}

	if (timedOut)
		status = -1;
	else if (WIFEXITED(status))
		status = WEXITSTATUS(status);
	else if (WIFSIGNALED(status))
		status = 128 + WTERMSIG(status);
	else
		status = -1;

	xsResult = xsNewObject();
	xsSet(xsResult, xsID("status"), xsInteger(status));
	xsSet(xsResult, xsID("stdout"), xsString(out.data ? out.data : ""));
	xsSet(xsResult, xsID("stderr"), xsString(err.data ? err.data : ""));

	free(out.data);
	free(err.data);
}

/*
	main
*/

int main(int argc, char* argv[])
{
	static txMachine root;
	int error = 0;
	txMachine* machine = &root;
	txPreparation* preparation = xsPreparation();

	gArgc = argc;
	gArgv = argv;

	if (isatty(STDIN_FILENO)) {
		struct termios tio;
		tcgetattr(STDIN_FILENO, &gTIO);
		tio = gTIO;
		tio.c_lflag &= ~(ICANON | ECHO | ISIG);
		tio.c_cc[VMIN] = 1;
		tio.c_cc[VTIME] = 0;
		tcsetattr(STDIN_FILENO, TCSANOW, &tio);
		gRawTerminal = 1;
	}
	atexit(killChildren);
	atexit(restoreTerminal);

	signal(SIGPIPE, SIG_IGN);
	signal(SIGINT, onTerminatingSignal);
	signal(SIGTERM, onTerminatingSignal);
	signal(SIGHUP, onTerminatingSignal);

	c_memset(machine, 0, sizeof(txMachine));
	machine->preparation = preparation;
	machine->keyArray = preparation->keys;
	machine->keyCount = (txID)preparation->keyCount + (txID)preparation->creation.initialKeyCount;
	machine->keyIndex = (txID)preparation->keyCount;
	machine->nameModulo = preparation->nameModulo;
	machine->nameTable = preparation->names;
	machine->symbolModulo = preparation->symbolModulo;
	machine->symbolTable = preparation->symbols;

	machine->stack = &preparation->stack[0];
	machine->stackBottom = &preparation->stack[0];
	machine->stackTop = &preparation->stack[preparation->stackCount];

	machine->firstHeap = &preparation->heap[0];
	machine->freeHeap = &preparation->heap[preparation->heapCount - 1];
	machine->aliasCount = (txID)preparation->aliasCount;

	machine = fxCloneMachine(&preparation->creation, machine, "xsdb", NULL);

	xsBeginHost(machine);
	{
		xsVars(2);
		xsTry {
			xsResult = xsAwaitImport("main", XS_IMPORT_NAMESPACE);
		}
		xsCatch {
			xsStringValue message = xsToString(xsException);
			fprintf(stderr, "xsdb: %s\n", message);
			error = 1;
		}
	}
	xsEndHost(machine);

	if (!error)
		xsdbRunLoop();

	xsDeleteMachine(machine);
	return error;
}
