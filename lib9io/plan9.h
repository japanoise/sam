#include <inttypes.h>

/*
 * compiler directive on Plan 9
 */
#ifndef USED
#define USED(x)                                                                \
	if (x)                                                                 \
		;                                                              \
	else
#endif

/*
 * easiest way to make sure these are defined
 */
#define uchar _fmtuchar
#define ushort _fmtushort
#define uint _fmtuint
#define ulong _fmtulong
#define vlong _fmtvlong
#define uvlong _fmtuvlong
#define uintptr _fmtuintptr

typedef unsigned char      uchar;
typedef unsigned short     ushort;
typedef unsigned int       uint;
typedef unsigned long      ulong;
typedef unsigned long long uvlong;
typedef long long          vlong;
typedef uintptr_t          uintptr;
