/* Stand-in for <io.h>: POSIX style low level file I/O. */
#ifndef RA_COMPAT_IO_H
#define RA_COMPAT_IO_H
/* Platform structures always use the natural layout, whatever the includer packs to. */
#pragma pack(push)		/* RA_NATURAL_PACKING_BEGIN */
#pragma pack()
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#ifndef O_BINARY
#define O_BINARY	0
#endif
#ifndef O_TEXT
#define O_TEXT		0
#endif
#ifdef __cplusplus
extern "C" {
#endif
int filelength(int handle);
#ifdef __cplusplus
}
#endif
#pragma pack(pop)		/* RA_NATURAL_PACKING_END */
#endif
