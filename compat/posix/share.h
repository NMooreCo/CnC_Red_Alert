/* Stand-in for <share.h>. */
#ifndef RA_COMPAT_SHARE_H
#define RA_COMPAT_SHARE_H
/* Platform structures always use the natural layout, whatever the includer packs to. */
#pragma pack(push)		/* RA_NATURAL_PACKING_BEGIN */
#pragma pack()
#define SH_COMPAT	0x00
#define SH_DENYRW	0x10
#define SH_DENYWR	0x20
#define SH_DENYRD	0x30
#define SH_DENYNO	0x40
#pragma pack(pop)		/* RA_NATURAL_PACKING_END */
#endif
