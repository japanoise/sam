#ifndef _U_H
#define _U_H
#include <assert.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <locale.h>
#include <stdbool.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "utf.h"

typedef int64_t  vlong;
typedef uint64_t ulong;
typedef uint64_t uvlong;
typedef uint64_t uint;
typedef uint32_t u32int;
typedef uint16_t u16int;
typedef uint16_t ushort;
typedef uint8_t  uchar;
typedef int8_t   schar;

#define UNICODE_REPLACEMENT_CHAR 0xfffd

#endif
