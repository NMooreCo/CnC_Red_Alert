/*
**	Stand-in for <sys/timeb.h>: Android's C library has no ftime(). The same, from
**	gettimeofday(). (compat/android is on the include path only for Android.)
*/
#ifndef RA_COMPAT_SYS_TIMEB_H
#define RA_COMPAT_SYS_TIMEB_H
#include <sys/time.h>
#include <time.h>

struct timeb {
	time_t				time;
	unsigned short		millitm;
	short					timezone;
	short					dstflag;
};

static inline int ftime(struct timeb * tb)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	tb->time = tv.tv_sec;
	tb->millitm = (unsigned short)(tv.tv_usec / 1000);
	tb->timezone = 0;
	tb->dstflag = 0;
	return(0);
}
#endif
