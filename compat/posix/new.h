/* Stand-in for the pre-standard <new.h> header. */
#ifndef RA_COMPAT_NEW_H
#define RA_COMPAT_NEW_H
/* Platform structures always use the natural layout, whatever the includer packs to. */
#pragma pack(push)		/* RA_NATURAL_PACKING_BEGIN */
#pragma pack()
#include <new>
#pragma pack(pop)		/* RA_NATURAL_PACKING_END */
#endif
