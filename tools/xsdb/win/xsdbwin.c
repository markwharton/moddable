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
	Windows host for xsdb: main(), the message pump, and the natives behind the "host" module.
	win_xs.c, the win socket module, and the win timer module all deliver through message-only
	windows, so the run loop is the ordinary GetMessage / DispatchMessage pump. Host events
	(stdin bytes, child exits) arrive from worker threads as messages on a window of our own,
	so JavaScript always runs on the main thread.

	Written against the conventions of examples/js/repl/win/repl.c and modules/network/socket/win;
	it has not yet been built on Windows.
*/

#include "xsAll.h"
#include "mc.xs.h"

#include <windows.h>
#include <io.h>
#include <process.h>
#include <direct.h>

#define kHostWindowClass "xsdbHostWindowClass"
#define WM_HOST_STDIN (WM_USER + 1)		// wParam: byte count (0 = end of input), lParam: malloc'd bytes
#define WM_HOST_CHILD (WM_USER + 2)		// wParam: pid, lParam: exit code
#define kMaxChildren 16

static int gArgc;
static char **gArgv;

static HANDLE gChildren[kMaxChildren];
static DWORD gChildIds[kMaxChildren];
static int gChildCount = 0;

static xsMachine *gMachine = NULL;
static xsSlot gStdinCallback;
static xsSlot gChildCallback;
static int gStdinWatched = 0;
static int gChildWatched = 0;
static HWND gWindow = NULL;
static DWORD gConsoleInputMode = 0, gConsoleOutputMode = 0;
static int gConsoleModes = 0;

/*
	process lifetime
*/

static void restoreConsole(void)
{
	if (gConsoleModes) {
		SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), gConsoleInputMode);
		SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), gConsoleOutputMode);
		gConsoleModes = 0;
	}
}

static void killChildren(void)
{
	int i;
	for (i = 0; i < gChildCount; i++) {
		TerminateProcess(gChildren[i], 1);
		CloseHandle(gChildren[i]);
	}
	gChildCount = 0;
}

static BOOL WINAPI onConsoleControl(DWORD type)
{
	// atexit handlers restore the console and stop the children
	exit(128 + 2);
	return TRUE;
}

static void untrackChild(DWORD pid)
{
	int i;
	for (i = 0; i < gChildCount; i++) {
		if (gChildIds[i] == pid) {
			CloseHandle(gChildren[i]);
			gChildren[i] = gChildren[gChildCount - 1];
			gChildIds[i] = gChildIds[gChildCount - 1];
			gChildCount--;
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
	arguments, environment, console
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
	char path[MAX_PATH];
	if (_getcwd(path, sizeof(path)))
		xsResult = xsString(path);
}

void xs_host_tmpdir(xsMachine *the)
{
	char path[MAX_PATH];
	DWORD length = GetTempPathA(sizeof(path), path);
	if ((length > 1) && (path[length - 1] == '\\'))
		path[length - 1] = 0;
	xsResult = xsString(length ? path : ".");
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
	CONSOLE_SCREEN_BUFFER_INFO info;
	if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info))
		xsResult = xsInteger(info.srWindow.Right - info.srWindow.Left + 1);
	else
		xsResult = xsInteger(80);
}

void xs_host_isTTY(xsMachine *the)
{
	xsResult = xsBoolean(_isatty(_fileno(stdin)));
}

void xs_host_platform(xsMachine *the)
{
	xsResult = xsString("win");
}

/*
	the host window: worker threads post here, the main thread calls into JavaScript
*/

static LRESULT CALLBACK hostWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
		case WM_HOST_STDIN: {
			char *bytes = (char *)lParam;
			xsBeginHost(gMachine);
			{
				xsVars(1);
				if (wParam > 0)
					xsVar(0) = xsArrayBuffer(bytes, (xsIntegerValue)wParam);
				else
					xsVar(0) = xsNull;
				xsCallFunction1(gStdinCallback, xsUndefined, xsVar(0));
			}
			xsEndHost(gMachine);
			free(bytes);
			return 0;
		}
		case WM_HOST_CHILD: {
			untrackChild((DWORD)wParam);
			xsBeginHost(gMachine);
			{
				xsCallFunction2(gChildCallback, xsUndefined, xsInteger((xsIntegerValue)wParam), xsInteger((xsIntegerValue)lParam));
			}
			xsEndHost(gMachine);
			return 0;
		}
	}
	return DefWindowProc(window, message, wParam, lParam);
}

static void createHostWindow(void)
{
	WNDCLASSEX wcex;
	if (gWindow)
		return;
	ZeroMemory(&wcex, sizeof(WNDCLASSEX));
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpfnWndProc = hostWindowProc;
	wcex.lpszClassName = kHostWindowClass;
	RegisterClassEx(&wcex);
	gWindow = CreateWindowEx(0, kHostWindowClass, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);
}

/*
	stdin: a thread blocks in ReadFile and posts each chunk; the console is in raw mode with
	virtual terminal input so arrow keys arrive as escape sequences, like a POSIX terminal
*/

static unsigned __stdcall stdinThread(void *ignore)
{
	HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
	while (1) {
		char buffer[4096];
		DWORD count = 0;
		char *bytes;
		if (!ReadFile(input, buffer, sizeof(buffer), &count, NULL) || (count == 0)) {
			PostMessage(gWindow, WM_HOST_STDIN, 0, (LPARAM)NULL);
			return 0;
		}
		bytes = malloc(count);
		memcpy(bytes, buffer, count);
		PostMessage(gWindow, WM_HOST_STDIN, (WPARAM)count, (LPARAM)bytes);
	}
}

void xs_host_setStdin(xsMachine *the)
{
	gMachine = the;
	if (gStdinWatched) {
		xsForget(gStdinCallback);
		gStdinCallback = xsArg(0);
		xsRemember(gStdinCallback);
		return;
	}
	gStdinCallback = xsArg(0);
	xsRemember(gStdinCallback);
	createHostWindow();
	_beginthreadex(NULL, 0, stdinThread, NULL, 0, NULL);
	gStdinWatched = 1;
}

/*
	child processes
*/

typedef struct {
	HANDLE process;
	DWORD pid;
} xsdbChildWait;

static unsigned __stdcall childWaitThread(void *it)
{
	xsdbChildWait *wait = it;
	DWORD code = 1;
	WaitForSingleObject(wait->process, INFINITE);
	GetExitCodeProcess(wait->process, &code);
	PostMessage(gWindow, WM_HOST_CHILD, (WPARAM)wait->pid, (LPARAM)code);
	free(wait);
	return 0;
}

void xs_host_setChildExit(xsMachine *the)
{
	if (gChildWatched)
		xsUnknownError("child exit callback already set");
	gMachine = the;
	gChildCallback = xsArg(0);
	xsRemember(gChildCallback);
	createHostWindow();
	gChildWatched = 1;
}

// a command line from path and args, quoting arguments with spaces or quotes as CreateProcess expects
static char *buildCommandLine(xsMachine *the, xsSlot path, xsSlot array)
{
	int count = xsToInteger(xsGet(array, xsID("length")));
	size_t size = 0, length = 0;
	char *line;
	int i;
	for (i = -1; i < count; i++)
		size += strlen(xsToString((i < 0) ? path : xsGetIndex(array, i))) * 2 + 4;
	line = malloc(size + 1);
	if (!line)
		xsUnknownError("not enough memory");
	for (i = -1; i < count; i++) {
		char *argument = xsToString((i < 0) ? path : xsGetIndex(array, i));
		int quote = (argument[0] == 0) || strchr(argument, ' ') || strchr(argument, '"') || strchr(argument, '\t');
		char *p;
		if (i >= 0)
			line[length++] = ' ';
		if (quote)
			line[length++] = '"';
		for (p = argument; *p; p++) {
			if (*p == '"')
				line[length++] = '\\';
			line[length++] = *p;
		}
		if (quote)
			line[length++] = '"';
	}
	line[length] = 0;
	return line;
}

static HANDLE openNull(DWORD access)
{
	SECURITY_ATTRIBUTES attributes = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
	return CreateFileA("NUL", access, FILE_SHARE_READ | FILE_SHARE_WRITE, &attributes, OPEN_EXISTING, 0, NULL);
}

// spawn(path, [args], "ignore" | "inherit") -> pid. stdin is NUL; stderr is inherited.
void xs_host_spawn(xsMachine *the)
{
	STARTUPINFOA startup;
	PROCESS_INFORMATION process;
	char *commandLine = buildCommandLine(the, xsArg(0), xsArg(1));
	int ignoreStdout = 1;
	HANDLE nullIn, nullOut = INVALID_HANDLE_VALUE;
	xsdbChildWait *wait;

	if (xsToInteger(xsArgc) > 2)
		ignoreStdout = (0 == strcmp(xsToString(xsArg(2)), "ignore"));

	ZeroMemory(&startup, sizeof(startup));
	startup.cb = sizeof(startup);
	startup.dwFlags = STARTF_USESTDHANDLES;
	nullIn = openNull(GENERIC_READ);
	startup.hStdInput = nullIn;
	if (ignoreStdout) {
		nullOut = openNull(GENERIC_WRITE);
		startup.hStdOutput = nullOut;
	}
	else
		startup.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	startup.hStdError = GetStdHandle(STD_ERROR_HANDLE);

	if (!CreateProcessA(NULL, commandLine, NULL, NULL, TRUE, 0, NULL, NULL, &startup, &process)) {
		DWORD error = GetLastError();
		free(commandLine);
		CloseHandle(nullIn);
		if (nullOut != INVALID_HANDLE_VALUE) CloseHandle(nullOut);
		xsUnknownError("spawn failed: error %lu", (unsigned long)error);
	}
	free(commandLine);
	CloseHandle(nullIn);
	if (nullOut != INVALID_HANDLE_VALUE) CloseHandle(nullOut);
	CloseHandle(process.hThread);

	if (gChildCount < kMaxChildren) {
		gChildren[gChildCount] = process.hProcess;
		gChildIds[gChildCount] = process.dwProcessId;
		gChildCount++;
	}
	if (gChildWatched) {
		HANDLE duplicate;
		DuplicateHandle(GetCurrentProcess(), process.hProcess, GetCurrentProcess(), &duplicate, 0, FALSE, DUPLICATE_SAME_ACCESS);
		wait = malloc(sizeof(xsdbChildWait));
		wait->process = duplicate;
		wait->pid = process.dwProcessId;
		_beginthreadex(NULL, 0, childWaitThread, wait, 0, NULL);
	}
	xsResult = xsInteger((xsIntegerValue)process.dwProcessId);
}

void xs_host_kill(xsMachine *the)
{
	DWORD pid = (DWORD)xsToInteger(xsArg(0));
	int i;
	for (i = 0; i < gChildCount; i++) {
		if (gChildIds[i] == pid) {
			TerminateProcess(gChildren[i], 1);
			return;
		}
	}
}

typedef struct {
	HANDLE pipe;
	char *data;
	size_t length;
} xsdbPipeReader;

static unsigned __stdcall pipeReaderThread(void *it)
{
	xsdbPipeReader *reader = it;
	size_t size = 0;
	while (1) {
		char chunk[4096];
		DWORD count = 0;
		if (!ReadFile(reader->pipe, chunk, sizeof(chunk), &count, NULL) || (count == 0))
			break;
		if (reader->length + count + 1 > size) {
			size = (size ? size * 2 : 1024);
			while (size < reader->length + count + 1)
				size *= 2;
			reader->data = realloc(reader->data, size);
		}
		memcpy(reader->data + reader->length, chunk, count);
		reader->length += count;
		reader->data[reader->length] = 0;
	}
	return 0;
}

// execute(path, [args], timeoutMilliseconds) -> { status, stdout, stderr }. Blocks until the child exits.
// status is the exit code, or -1 when the timeout killed the child.
void xs_host_execute(xsMachine *the)
{
	SECURITY_ATTRIBUTES attributes = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
	STARTUPINFOA startup;
	PROCESS_INFORMATION process;
	HANDLE outRead, outWrite, errRead, errWrite, nullIn, threads[2];
	xsdbPipeReader out = { NULL, NULL, 0 }, err = { NULL, NULL, 0 };
	char *commandLine;
	DWORD timeout = INFINITE, code = 1;
	int status;

	if (xsToInteger(xsArgc) > 2) {
		int milliseconds = xsToInteger(xsArg(2));
		if (milliseconds >= 0)
			timeout = (DWORD)milliseconds;
	}
	commandLine = buildCommandLine(the, xsArg(0), xsArg(1));

	CreatePipe(&outRead, &outWrite, &attributes, 0);
	CreatePipe(&errRead, &errWrite, &attributes, 0);
	SetHandleInformation(outRead, HANDLE_FLAG_INHERIT, 0);
	SetHandleInformation(errRead, HANDLE_FLAG_INHERIT, 0);
	nullIn = openNull(GENERIC_READ);

	ZeroMemory(&startup, sizeof(startup));
	startup.cb = sizeof(startup);
	startup.dwFlags = STARTF_USESTDHANDLES;
	startup.hStdInput = nullIn;
	startup.hStdOutput = outWrite;
	startup.hStdError = errWrite;

	if (!CreateProcessA(NULL, commandLine, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &startup, &process)) {
		DWORD error = GetLastError();
		free(commandLine);
		CloseHandle(outRead); CloseHandle(outWrite); CloseHandle(errRead); CloseHandle(errWrite); CloseHandle(nullIn);
		xsUnknownError("execute failed: error %lu", (unsigned long)error);
	}
	free(commandLine);
	CloseHandle(outWrite);
	CloseHandle(errWrite);
	CloseHandle(nullIn);
	CloseHandle(process.hThread);

	out.pipe = outRead;
	err.pipe = errRead;
	threads[0] = (HANDLE)_beginthreadex(NULL, 0, pipeReaderThread, &out, 0, NULL);
	threads[1] = (HANDLE)_beginthreadex(NULL, 0, pipeReaderThread, &err, 0, NULL);

	if (WaitForSingleObject(process.hProcess, timeout) == WAIT_TIMEOUT) {
		TerminateProcess(process.hProcess, 1);
		WaitForSingleObject(process.hProcess, INFINITE);
		status = -1;
	}
	else {
		GetExitCodeProcess(process.hProcess, &code);
		status = (int)code;
	}
	WaitForMultipleObjects(2, threads, TRUE, INFINITE);
	CloseHandle(threads[0]);
	CloseHandle(threads[1]);
	CloseHandle(outRead);
	CloseHandle(errRead);
	CloseHandle(process.hProcess);

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
	MSG msg;

	gArgc = argc;
	gArgv = argv;

	if (_isatty(_fileno(stdin))) {
		HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
		HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
		if (GetConsoleMode(input, &gConsoleInputMode) && GetConsoleMode(output, &gConsoleOutputMode)) {
			DWORD inputMode = gConsoleInputMode;
			inputMode &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT);
			inputMode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
			SetConsoleMode(input, inputMode);
			SetConsoleMode(output, gConsoleOutputMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
			gConsoleModes = 1;
		}
	}
	atexit(killChildren);
	atexit(restoreConsole);
	SetConsoleCtrlHandler(onConsoleControl, TRUE);

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

	if (!error) {
		while (GetMessage(&msg, NULL, 0, 0) > 0) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	xsDeleteMachine(machine);
	return error;
}
