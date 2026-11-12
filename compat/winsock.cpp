/*
**	Command & Conquer Red Alert(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
**	winsock.cpp -- Winsock 1.1 emulation on BSD sockets.
**
**	The game's network layer (CODE/WSPROTO.CPP, WSPUDP.CPP) asks for WSAAsyncSelect()
**	window messages when its sockets become readable/writable. Registered sockets are polled
**	from the main event pump (Winsock_Poll) and the corresponding messages are posted to
**	the game window, just as Windows would have done.
*/

#include "win32_internal.h"

#if !defined(_WIN32)

#include "winsock.h"
#include <vector>
#include <fcntl.h>
#include <poll.h>

struct AsyncSocket {
	SOCKET	Socket;
	HWND		Window;
	UINT		Message;
	int		Events;
	bool		WriteSignalled;	// FD_WRITE is edge triggered under Windows.
};

static std::vector<AsyncSocket> AsyncSockets;
static int WinsockError = 0;

extern "C" int WSAStartup(WORD version, LPWSADATA data)
{
	if (data) {
		memset(data, 0, sizeof(*data));
		data->wVersion = version;
		data->wHighVersion = MAKEWORD(1, 1);
		strcpy(data->szDescription, "BSD sockets (Red Alert port)");
		strcpy(data->szSystemStatus, "Running");
		data->iMaxSockets = 256;
		data->iMaxUdpDg = 65467;
	}
	return(0);
}

extern "C" int WSACleanup(void)
{
	AsyncSockets.clear();
	return(0);
}

extern "C" int WSAGetLastError(void)
{
	return(WinsockError ? WinsockError : errno);
}

extern "C" void WSASetLastError(int error)
{
	WinsockError = error;
}

extern "C" int closesocket(SOCKET s)
{
	for (size_t i = 0; i < AsyncSockets.size(); i++) {
		if (AsyncSockets[i].Socket == s) {
			AsyncSockets.erase(AsyncSockets.begin() + i);
			break;
		}
	}
	return(close(s));
}

extern "C" int ioctlsocket(SOCKET s, int command, unsigned int * argp)
{
	if (command == (int)FIONBIO) {
		int flags = fcntl(s, F_GETFL, 0);
		if (argp && *argp) {
			flags |= O_NONBLOCK;
		} else {
			flags &= ~O_NONBLOCK;
		}
		return(fcntl(s, F_SETFL, flags) == 0 ? 0 : SOCKET_ERROR);
	}
	if (command == (int)FIONREAD) {
		int value = 0;
		int result = ioctl(s, FIONREAD, &value);
		if (argp) *argp = (unsigned int)value;
		return(result);
	}
	return(SOCKET_ERROR);
}

extern "C" int WSAAsyncSelect(SOCKET s, HWND window, UINT message, int events)
{
	for (size_t i = 0; i < AsyncSockets.size(); i++) {
		if (AsyncSockets[i].Socket == s) {
			AsyncSockets.erase(AsyncSockets.begin() + i);
			break;
		}
	}
	if (events == 0 || message == 0) return(0);

	/*
	**	WSAAsyncSelect implicitly makes the socket non-blocking.
	*/
	int flags = fcntl(s, F_GETFL, 0);
	fcntl(s, F_SETFL, flags | O_NONBLOCK);

	AsyncSocket as;
	as.Socket = s;
	as.Window = window;
	as.Message = message;
	as.Events = events;
	as.WriteSignalled = false;
	AsyncSockets.push_back(as);
	return(0);
}

void Winsock_Poll(void)
{
	if (AsyncSockets.empty()) return;

	std::vector<struct pollfd> fds(AsyncSockets.size());
	for (size_t i = 0; i < AsyncSockets.size(); i++) {
		fds[i].fd = AsyncSockets[i].Socket;
		fds[i].events = 0;
		if (AsyncSockets[i].Events & (FD_READ | FD_ACCEPT | FD_CLOSE)) fds[i].events |= POLLIN;
		if (AsyncSockets[i].Events & (FD_WRITE | FD_CONNECT)) fds[i].events |= POLLOUT;
		fds[i].revents = 0;
	}
	if (poll(&fds[0], fds.size(), 0) <= 0) return;

	/*
	**	Copy: posting may not re-enter, but be safe against the vector changing.
	*/
	std::vector<AsyncSocket> sockets = AsyncSockets;
	for (size_t i = 0; i < sockets.size(); i++) {
		AsyncSocket & as = sockets[i];
		short re = fds[i].revents;
		if ((re & POLLIN) && (as.Events & FD_READ)) {
			PostMessage(as.Window, as.Message, (WPARAM)as.Socket, WSAMAKESELECTREPLY(FD_READ, 0));
		}
		if ((re & POLLIN) && (as.Events & FD_ACCEPT)) {
			PostMessage(as.Window, as.Message, (WPARAM)as.Socket, WSAMAKESELECTREPLY(FD_ACCEPT, 0));
		}
		if (re & POLLOUT) {
			if (as.Events & FD_WRITE) {
				/*
				**	Windows only sends FD_WRITE once the socket becomes writable again after a
				**	send failed with WSAEWOULDBLOCK. The game's handler simply flushes its send
				**	queue, so a periodic notification while writable is harmless.
				*/
				PostMessage(as.Window, as.Message, (WPARAM)as.Socket, WSAMAKESELECTREPLY(FD_WRITE, 0));
			}
		}
		if ((re & (POLLHUP | POLLERR)) && (as.Events & FD_CLOSE)) {
			PostMessage(as.Window, as.Message, (WPARAM)as.Socket, WSAMAKESELECTREPLY(FD_CLOSE, 0));
		}
	}
}

/*
**	Asynchronous host name lookups are performed synchronously and the completion message
**	is posted immediately.
*/
static HANDLE Async_Host_Reply(HWND window, UINT message, struct hostent * host, char * buffer, int buflen)
{
	static intptr_t next_handle = 1;
	HANDLE handle = (HANDLE)(next_handle++);
	int error = 0;
	if (host == NULL || buflen < (int)sizeof(struct hostent)) {
		error = 11001;		// WSAHOST_NOT_FOUND
	} else {
		/*
		**	Copy the hostent (and first address) into the caller's buffer.
		*/
		struct hostent * out = (struct hostent *)buffer;
		char * p = buffer + sizeof(struct hostent);
		char * end = buffer + buflen;
		memset(out, 0, sizeof(*out));
		out->h_addrtype = host->h_addrtype;
		out->h_length = host->h_length;
		if (p + 2 * sizeof(char *) + host->h_length <= end && host->h_addr_list && host->h_addr_list[0]) {
			char ** list = (char **)p;
			p += 2 * sizeof(char *);
			memcpy(p, host->h_addr_list[0], host->h_length);
			list[0] = p;
			list[1] = NULL;
			out->h_addr_list = list;
			p += host->h_length;
		}
		if (host->h_name && p + strlen(host->h_name) + 1 <= end) {
			strcpy(p, host->h_name);
			out->h_name = p;
		}
	}
	PostMessage(window, message, (WPARAM)handle, WSAMAKEASYNCREPLY(0, error));
	return(handle);
}

extern "C" HANDLE WSAAsyncGetHostByName(HWND window, UINT message, const char * name, char * buffer, int buflen)
{
	return(Async_Host_Reply(window, message, gethostbyname(name), buffer, buflen));
}

extern "C" HANDLE WSAAsyncGetHostByAddr(HWND window, UINT message, const char * addr, int len, int type, char * buffer, int buflen)
{
	return(Async_Host_Reply(window, message, gethostbyaddr(addr, len, type), buffer, buflen));
}

extern "C" int WSACancelAsyncRequest(HANDLE )
{
	return(0);
}

#else

void Winsock_Poll(void)
{
}

#endif
