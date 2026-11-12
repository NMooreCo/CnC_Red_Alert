/* Stand-in for the DOS console header <conio.h>. */
#ifndef RA_COMPAT_CONIO_H
#define RA_COMPAT_CONIO_H
/* Platform structures always use the natural layout, whatever the includer packs to. */
#pragma pack(push)		/* RA_NATURAL_PACKING_BEGIN */
#pragma pack()
#ifdef __cplusplus
extern "C" {
#endif
int getch(void);
int kbhit(void);
#ifdef __cplusplus
}
#endif
#pragma pack(pop)		/* RA_NATURAL_PACKING_END */
#endif
