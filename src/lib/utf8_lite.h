
#ifndef UTF8_LITE_H
#define UTF8_LITE_H

#include <stdint.h>

#define MAXUNICODE    0x10FFFF 

const char *utf8_decode (const char *o, int *val);
int utf8_len(const char *s);

char* ucs4_to_utf8(int code);

uint32_t* utf8_to_ucs4_str(const char *s, int *plen);

#endif
