
#ifndef UTF8_LITE_H
#define UTF8_LITE_H

#define MAXUNICODE    0x10FFFF

const char *utf8_decode (const char *o, int *val);
int utf8_len(const char *s);

char* ucs4_to_utf8(int code);

#endif
