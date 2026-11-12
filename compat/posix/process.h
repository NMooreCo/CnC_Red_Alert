/* Stand-in for <process.h>. */
#ifndef RA_COMPAT_PROCESS_H
#define RA_COMPAT_PROCESS_H
/* Platform structures always use the natural layout, whatever the includer packs to. */
#pragma pack(push)		/* RA_NATURAL_PACKING_BEGIN */
#pragma pack()
#include <stdlib.h>
#pragma pack(pop)		/* RA_NATURAL_PACKING_END */
#endif
