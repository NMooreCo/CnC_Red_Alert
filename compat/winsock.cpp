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
**	winsock.cpp -- the parts of Winsock 1.1 that the port emulates.
**
**	The game's network layer (CODE/WSPROTO.CPP, WSPUDP.CPP) asks for WSAAsyncSelect()
**	window messages when its sockets become readable/writable. Registered sockets are polled
**	from the main event pump (Winsock_Poll) and the corresponding messages are posted to
**	the game window, just as Windows would have done. Host name look-ups that report back
**	with a window message are emulated too.
**
**	On Windows everything else is the real Winsock; elsewhere it is emulated here on BSD
**	sockets.
*/

#include "win32_internal.h"
#include "winsock.h"
#include <vector>
#include <SDL.h>

#if defined(_WIN32)
#undef closesocket		/* The real one; winsock.h points the game's calls at ours. */
#else
#include <fcntl.h>
#include <poll.h>
#include <ifaddrs.h>
#endif

struct AsyncSocket {
	SOCKET	Socket;
	HWND		Window;
	UINT		Message;
	int		Events;
	Uint32	LastWrite;		// When FD_WRITE was last posted.
};

static std::vector<AsyncSocket> AsyncSockets;

enum {
	SOCKET_READABLE = 0x01,
	SOCKET_WRITABLE = 0x02,
	SOCKET_FAILED = 0x04
};


/***************************************************************************************
**	Platform specific helpers.
*/
#if defined(_WIN32)

static void Set_Non_Blocking(SOCKET s)
{
	u_long on = 1;
	ioctlsocket(s, FIONBIO, &on);
}

static int Close_Socket_Handle(SOCKET s)
{
	return(closesocket(s));
}

static bool In_Set(fd_set const & set, SOCKET s)
{
	for (u_int i = 0; i < set.fd_count; i++) {
		if (set.fd_array[i] == s) return(true);
	}
	return(false);
}

/*
**	Find out, without waiting, which sockets are readable, writable or failed.
*/
static bool Check_Sockets(std::vector<AsyncSocket> const & sockets, std::vector<int> & states)
{
	fd_set readable, writable, failed;
	readable.fd_count = 0;
	writable.fd_count = 0;
	failed.fd_count = 0;
	for (size_t i = 0; i < sockets.size() && i < FD_SETSIZE; i++) {
		if (sockets[i].Events & (FD_READ | FD_ACCEPT | FD_CLOSE)) readable.fd_array[readable.fd_count++] = sockets[i].Socket;
		if (sockets[i].Events & (FD_WRITE | FD_CONNECT)) writable.fd_array[writable.fd_count++] = sockets[i].Socket;
		failed.fd_array[failed.fd_count++] = sockets[i].Socket;
	}
	struct timeval no_wait = {0, 0};
	if (select(0, &readable, &writable, &failed, &no_wait) <= 0) return(false);
	for (size_t i = 0; i < sockets.size(); i++) {
		states[i] = 0;
		if (In_Set(readable, sockets[i].Socket)) states[i] |= SOCKET_READABLE;
		if (In_Set(writable, sockets[i].Socket)) states[i] |= SOCKET_WRITABLE;
		if (In_Set(failed, sockets[i].Socket)) states[i] |= SOCKET_FAILED;
	}
	return(true);
}

/*
**	The addresses the host name resolves to: those of every network interface, on Windows.
*/
extern "C" int Winsock_Get_Local_Addresses(unsigned int * addresses, int max)
{
	int count = 0;
	char name[256];
	if (max > 0) addresses[count++] = htonl(INADDR_LOOPBACK);
	if (gethostname(name, sizeof(name)) != 0) return(count);
	struct hostent * host = gethostbyname(name);
	if (host == NULL || host->h_addrtype != AF_INET || host->h_length != 4) return(count);
	for (char ** entry = host->h_addr_list; *entry != NULL && count < max; entry++) {
		memcpy(&addresses[count++], *entry, 4);
	}
	return(count);
}

#else

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

static void Set_Non_Blocking(SOCKET s)
{
	int flags = fcntl(s, F_GETFL, 0);
	fcntl(s, F_SETFL, flags | O_NONBLOCK);
}

static int Close_Socket_Handle(SOCKET s)
{
	return(close(s));
}

static bool Check_Sockets(std::vector<AsyncSocket> const & sockets, std::vector<int> & states)
{
	std::vector<struct pollfd> fds(sockets.size());
	for (size_t i = 0; i < sockets.size(); i++) {
		fds[i].fd = sockets[i].Socket;
		fds[i].events = 0;
		if (sockets[i].Events & (FD_READ | FD_ACCEPT | FD_CLOSE)) fds[i].events |= POLLIN;
		if (sockets[i].Events & (FD_WRITE | FD_CONNECT)) fds[i].events |= POLLOUT;
		fds[i].revents = 0;
	}
	if (poll(&fds[0], fds.size(), 0) <= 0) return(false);
	for (size_t i = 0; i < sockets.size(); i++) {
		states[i] = 0;
		if (fds[i].revents & POLLIN) states[i] |= SOCKET_READABLE;
		if (fds[i].revents & POLLOUT) states[i] |= SOCKET_WRITABLE;
		if (fds[i].revents & (POLLHUP | POLLERR)) states[i] |= SOCKET_FAILED;
	}
	return(true);
}

extern "C" int Winsock_Get_Local_Addresses(unsigned int * addresses, int max)
{
	struct ifaddrs * list = NULL;
	int count = 0;
	if (getifaddrs(&list) != 0) return(0);
	for (struct ifaddrs * entry = list; entry != NULL && count < max; entry = entry->ifa_next) {
		if (entry->ifa_addr != NULL && entry->ifa_addr->sa_family == AF_INET) {
			addresses[count++] = ((struct sockaddr_in *)entry->ifa_addr)->sin_addr.s_addr;
		}
	}
	freeifaddrs(list);
	return(count);
}

#endif


/***************************************************************************************
**	WSAAsyncSelect.
*/
static void Forget_Socket(SOCKET s)
{
	for (size_t i = 0; i < AsyncSockets.size(); i++) {
		if (AsyncSockets[i].Socket == s) {
			AsyncSockets.erase(AsyncSockets.begin() + i);
			break;
		}
	}
}

extern "C" int Winsock_Close_Socket(SOCKET s)
{
	Forget_Socket(s);
	return(Close_Socket_Handle(s));
}

extern "C" int Winsock_Async_Select(SOCKET s, HWND window, UINT message, int events)
{
	Forget_Socket(s);
	if (events == 0 || message == 0) return(0);

	/*
	**	WSAAsyncSelect implicitly makes the socket non-blocking.
	*/
	Set_Non_Blocking(s);

	AsyncSocket as;
	as.Socket = s;
	as.Window = window;
	as.Message = message;
	as.Events = events;
	as.LastWrite = 0;
	AsyncSockets.push_back(as);
	return(0);
}

void Winsock_Poll(void)
{
	if (AsyncSockets.empty()) return;

	/*
	**	Copy: posting may not re-enter, but be safe against the vector changing.
	*/
	std::vector<AsyncSocket> sockets = AsyncSockets;
	std::vector<int> states(sockets.size(), 0);
	if (!Check_Sockets(sockets, states)) return;

	Uint32 now = SDL_GetTicks();
	for (size_t i = 0; i < sockets.size(); i++) {
		AsyncSocket & as = sockets[i];
		int state = states[i];

		/*
		**	Like Windows, don't post another FD_READ (or FD_ACCEPT) while one is still waiting
		**	to be handled. (The game reads its messages in a "while (PeekMessage())" loop, and
		**	each PeekMessage polls the sockets again.)
		*/
		if ((state & SOCKET_READABLE) && (as.Events & FD_READ)) {
			if (!Win32_Message_Pending(as.Message, (WPARAM)as.Socket, WSAMAKESELECTREPLY(FD_READ, 0))) {
				PostMessage(as.Window, as.Message, (WPARAM)as.Socket, WSAMAKESELECTREPLY(FD_READ, 0));
			}
		}
		if ((state & SOCKET_READABLE) && (as.Events & FD_ACCEPT)) {
			if (!Win32_Message_Pending(as.Message, (WPARAM)as.Socket, WSAMAKESELECTREPLY(FD_ACCEPT, 0))) {
				PostMessage(as.Window, as.Message, (WPARAM)as.Socket, WSAMAKESELECTREPLY(FD_ACCEPT, 0));
			}
		}

		/*
		**	Windows only sends FD_WRITE when a socket becomes writable (again, after a send
		**	failed with WSAEWOULDBLOCK). The game's handler flushes its send queue, and the game
		**	asks for that itself whenever it queues a packet; a writable socket only gets a
		**	reminder now and then, so that the message loop can't spin on it.
		*/
		if ((state & SOCKET_WRITABLE) && (as.Events & FD_WRITE) && (now - as.LastWrite) >= 100) {
			if (!Win32_Message_Pending(as.Message, (WPARAM)as.Socket, WSAMAKESELECTREPLY(FD_WRITE, 0))) {
				PostMessage(as.Window, as.Message, (WPARAM)as.Socket, WSAMAKESELECTREPLY(FD_WRITE, 0));
			}
			for (size_t j = 0; j < AsyncSockets.size(); j++) {
				if (AsyncSockets[j].Socket == as.Socket) AsyncSockets[j].LastWrite = now;
			}
		}
		if ((state & SOCKET_FAILED) && (as.Events & FD_CLOSE)) {
			PostMessage(as.Window, as.Message, (WPARAM)as.Socket, WSAMAKESELECTREPLY(FD_CLOSE, 0));
		}
	}
}


/***************************************************************************************
**	Asynchronous host name look-ups are performed synchronously and the completion message
**	is posted immediately.
*/
static HANDLE Async_Host_Reply(HWND window, UINT message, struct hostent * host, char * buffer, int buflen)
{
	static intptr_t next_handle = 1;
	HANDLE handle = (HANDLE)(next_handle++);
	int error = 0;
	if (host == NULL || buflen < (int)sizeof(struct hostent)) {
		error = WSAHOST_NOT_FOUND;
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

extern "C" HANDLE Winsock_Async_Get_Host_By_Name(HWND window, UINT message, const char * name, char * buffer, int buflen)
{
	return(Async_Host_Reply(window, message, gethostbyname(name), buffer, buflen));
}

extern "C" HANDLE Winsock_Async_Get_Host_By_Addr(HWND window, UINT message, const char * addr, int len, int type, char * buffer, int buflen)
{
	return(Async_Host_Reply(window, message, gethostbyaddr(addr, len, type), buffer, buflen));
}

extern "C" int Winsock_Cancel_Async_Request(HANDLE )
{
	return(0);
}
