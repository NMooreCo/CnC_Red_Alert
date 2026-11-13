/* Stand-in for <malloc.h> (not available on every platform). */
#ifndef RA_COMPAT_MALLOC_H
#define RA_COMPAT_MALLOC_H
/* Platform structures always use the natural layout, whatever the includer packs to. */
#pragma pack(push)		/* RA_NATURAL_PACKING_BEGIN */
#pragma pack()
#if defined(__ANDROID__)
#include_next <malloc.h>		/* Android's own: its <stdlib.h> declares malloc() through it. */
#endif
#include <stdlib.h>
#pragma pack(pop)		/* RA_NATURAL_PACKING_END */
#endif
