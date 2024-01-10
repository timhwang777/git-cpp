#ifndef ZLIB_IMPLEMENT_H
#define ZLIB_IMPLEMENT_H

#include <cstdio>

int decompress(FILE* input, FILE* output);
int compress (FILE* input, FILE* output);

#endif // ZLIB_IMPLEMENT_H