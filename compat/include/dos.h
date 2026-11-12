/* Stand-in for the DOS <dos.h> header. Only the pieces the Win32 build used are provided. */
#ifndef RA_COMPAT_DOS_H
#define RA_COMPAT_DOS_H
/* Platform structures always use the natural layout, whatever the includer packs to. */
#pragma pack(push)		/* RA_NATURAL_PACKING_BEGIN */
#pragma pack()
#include <stdint.h>
#include "i86.h"

#define _A_NORMAL	0x00
#define _A_RDONLY	0x01
#define _A_HIDDEN	0x02
#define _A_SYSTEM	0x04
#define _A_VOLID	0x08
#define _A_SUBDIR	0x10
#define _A_ARCH		0x20

struct find_t {
	char		reserved[21];
	char		attrib;
	unsigned short wr_time;
	unsigned short wr_date;
	unsigned int size;
	char		name[260];
	void *	handle;		/* port: directory iterator state */
};

struct diskfree_t {
	unsigned total_clusters;
	unsigned avail_clusters;
	unsigned sectors_per_cluster;
	unsigned bytes_per_sector;
};

#ifdef __cplusplus
extern "C" {
#endif
unsigned _dos_findfirst(const char *path, unsigned attributes, struct find_t *buffer);
unsigned _dos_findnext(struct find_t *buffer);
unsigned _dos_findclose(struct find_t *buffer);
void _dos_getdrive(unsigned *drive);
void _dos_setdrive(unsigned drive, unsigned *total);
unsigned _dos_getdiskfree(unsigned drive, struct diskfree_t *diskspace);
#ifdef __cplusplus
}
#endif
#pragma pack(pop)		/* RA_NATURAL_PACKING_END */
#endif
