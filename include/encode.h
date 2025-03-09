#ifndef _ENCODE_H
#define _ENCODE_H
#include "u.h"

int dec64(uchar *out, int lim, char *in, int n);
int enc64(char *out, int lim, uchar *in, int n);
int dec32(uchar *out, int lim, char *in, int n);
int enc32(char *out, int lim, uchar *in, int n);
int dec16(uchar *out, int lim, char *in, int n);
int enc16(char *out, int lim, uchar *in, int n);

#endif
