#ifndef _ERRSTR_H
#define _ERRSTR_H

#include <u.h>

extern char *strecpy(char *to, char *e, char *from);
extern int   errstr(char *, uint);
extern void  rerrstr(char *, uint);
extern void  werrstr(char *, ...);

#endif
