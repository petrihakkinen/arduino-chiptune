#ifndef DEFS_H
#define DEFS_H

#ifdef WIN32
#include <string.h> // for memset

typedef char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;

#define min(a,b) ((a)<(b) ? (a) : (b))
#define max(a,b) ((a)>(b) ? (a) : (b))
#endif

#endif
