/* Stand-in for the Watcom <i86.h> header (DOS register structures; unused at run time). */
#ifndef RA_COMPAT_I86_H
#define RA_COMPAT_I86_H
/* Platform structures always use the natural layout, whatever the includer packs to. */
#pragma pack(push)		/* RA_NATURAL_PACKING_BEGIN */
#pragma pack()
struct DWORDREGS { unsigned int eax, ebx, ecx, edx, esi, edi, cflag; };
struct WORDREGS { unsigned short ax, _1, bx, _2, cx, _3, dx, _4, si, _5, di, _6; unsigned int cflag; };
struct BYTEREGS { unsigned char al, ah, _1, _2, bl, bh, _3, _4, cl, ch, _5, _6, dl, dh, _7, _8; };
union REGS { struct DWORDREGS x; struct WORDREGS w; struct BYTEREGS h; };
#define _REGS REGS
struct SREGS { unsigned short es, cs, ss, ds, fs, gs; };
#define _SREGS SREGS
#pragma pack(pop)		/* RA_NATURAL_PACKING_END */
#endif
