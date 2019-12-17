#ifndef _BENCODE_H_
#define _BENCODE_H_

#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int decodedict(char *, int, char *, char **, int *);

char *encodestring(char *, int *, char *, int);

char *encodedictstart(char *, int *);
char *encodedictend(char *, int *);

#endif
