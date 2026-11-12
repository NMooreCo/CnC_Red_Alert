/* Stand-in for the Watcom/Borland <mem.h> header. */
#ifndef RA_COMPAT_MEM_H
#define RA_COMPAT_MEM_H
/* Platform structures always use the natural layout, whatever the includer packs to. */
#pragma pack(push)		/* RA_NATURAL_PACKING_BEGIN */
#pragma pack()
#include <string.h>
#pragma pack(pop)		/* RA_NATURAL_PACKING_END */
#endif
