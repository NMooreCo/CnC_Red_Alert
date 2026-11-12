/* Stand-in for <wsnwlink.h> (nothing from it is needed by the port). */
#include "winsock.h"
#ifndef RA_COMPAT_WSNWLINK_H
#define RA_COMPAT_WSNWLINK_H
/* Platform structures always use the natural layout, whatever the includer packs to. */
#pragma pack(push)		/* RA_NATURAL_PACKING_BEGIN */
#pragma pack()
/* IPX socket options (IPX is not available on modern systems; these calls simply fail). */
#define IPX_PTYPE			0x4000
#define IPX_FILTERPTYPE	0x4001
#define IPX_STOPFILTERPTYPE 0x4003
#define IPX_ADDRESS		0x4007
#define IPX_MAX_ADAPTER_NUM 0x400D
typedef struct _IPX_ADDRESS_DATA {
	INT		adapternum;
	UCHAR		netnum[4];
	UCHAR		nodenum[6];
	BOOL		wan;
	BOOL		status;
	INT		maxpkt;
	ULONG		linkspeed;
} IPX_ADDRESS_DATA, *PIPX_ADDRESS_DATA;
#pragma pack(pop)		/* RA_NATURAL_PACKING_END */
#endif
