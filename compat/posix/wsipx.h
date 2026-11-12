/* Stand-in for <wsipx.h>: Novell IPX socket address (IPX itself is not available). */
#ifndef RA_COMPAT_WSIPX_H
#define RA_COMPAT_WSIPX_H
/* Platform structures always use the natural layout, whatever the includer packs to. */
#pragma pack(push)		/* RA_NATURAL_PACKING_BEGIN */
#pragma pack()
#include "winsock.h"
typedef struct sockaddr_ipx {
	short		sa_family;
	char		sa_netnum[4];
	char		sa_nodenum[6];
	unsigned short sa_socket;
} SOCKADDR_IPX, *PSOCKADDR_IPX, *LPSOCKADDR_IPX;
#define NSPROTO_IPX		1000
#define NSPROTO_SPX		1256
#define NSPROTO_SPXII	1257
#pragma pack(pop)		/* RA_NATURAL_PACKING_END */
#endif
