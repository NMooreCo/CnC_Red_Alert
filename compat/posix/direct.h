/* Stand-in for <direct.h>. */
#ifndef RA_COMPAT_DIRECT_H
#define RA_COMPAT_DIRECT_H
/* Platform structures always use the natural layout, whatever the includer packs to. */
#pragma pack(push)		/* RA_NATURAL_PACKING_BEGIN */
#pragma pack()
#include <sys/types.h>
#include <sys/stat.h>
#if !defined(_WIN32)
#include <unistd.h>
#endif
#ifdef __cplusplus
extern "C" {
#endif
int ra_mkdir(const char *path);
#ifdef __cplusplus
}
#endif
#pragma pack(pop)		/* RA_NATURAL_PACKING_END */
#endif
