
#include "utf8_lite.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef _MSC_VER
#include <Windows.h>
#endif


/*
** Decode one UTF-8 sequence, returning NULL if byte sequence is invalid.
*/
const char *utf8_decode(const char *o, int *val) {
    static const unsigned int limits[] = { 0xFF, 0x7F, 0x7FF, 0xFFFF };
    const unsigned char *s = (const unsigned char *)o;
    unsigned int c = s[0];
    unsigned int res = 0;  /* final result */
    if (c < 0x80)  /* ascii? */
        res = c;
    else {
        int count = 0;  /* to count number of continuation bytes */
        while (c & 0x40) {  /* still have continuation bytes? */
            int cc = s[++count];  /* read next byte */
            if ((cc & 0xC0) != 0x80)  /* not a continuation byte? */
                return NULL;  /* invalid byte sequence */
            res = (res << 6) | (cc & 0x3F);  /* add lower 6 bits from cont. byte */
            c <<= 1;  /* to test next bit */
        }
        res |= ((c & 0x7F) << (count * 5));  /* add first byte */
        if (count > 3 || res > MAXUNICODE || res <= limits[count])
            return NULL;  /* invalid byte sequence */
        s += count;  /* skip continuation bytes read */
    }
    if (val) *val = res;
    return (const char *)s + 1;  /* +1 to include first byte */
}


int utf8_len(const char *s) {
    int code;
    int len = 0, rlen = strlen(s);
    const char* s_end = s + rlen + 1;

    for (const char *p = utf8_decode(s, &code); p != s_end; p = utf8_decode(p, &code)) {
        len += 1;
    }
    
    return len;
}


char* ucs4_to_utf8(int code) {
    const char  abPrefix[] = {0, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC};
    const int adwCodeUp[] = {
        0x80,           // U+00000000 ～ U+0000007F
        0x800,          // U+00000080 ～ U+000007FF
        0x10000,        // U+00000800 ～ U+0000FFFF
        0x200000,       // U+00010000 ～ U+001FFFFF
        0x4000000,      // U+00200000 ～ U+03FFFFFF
        0x80000000      // U+04000000 ～ U+7FFFFFFF
    };

    int i, ilen;

    // 根据UCS4编码范围确定对应的UTF-8编码字节数
    ilen = sizeof(adwCodeUp) / sizeof(uint32_t);
    for(i = 0; i < ilen; i++) {
        if( code < adwCodeUp[i] ) break;
    }

    if (i == ilen) return NULL;    // 无效的UCS4编码

    ilen = i + 1;   // UTF-8编码字节数
    char* pbUTF8 = malloc(sizeof(char) * (ilen+1));

    if (pbUTF8 != NULL) {   // 转换为UTF-8编码
        for( ; i > 0; i-- ) {
            pbUTF8[i] = (char)((code & 0x3F) | 0x80);
            code >>= 6;
        }

        pbUTF8[0] = (char)(code | abPrefix[ilen - 1]);
    }

    /*for (i = 0; i < ilen; i++) {
        printf("%2x ", pbUTF8[i]);
    }*/
    pbUTF8[ilen] = 0;
    
    return pbUTF8;
}

uint32_t* utf8_to_ucs4_str(const char *s, int *plen) {
    int code;
    const char *p = s;
    int len = utf8_len(s);
    uint32_t *buf = malloc((len+1) * sizeof(uint32_t));

    for (int i = 0; i < len;++i) {
        p = utf8_decode(p, &code);
        buf[i] = (uint32_t)code;
    }

    buf[len] = '\0';
    if (plen) *plen = len;
    return buf;
}
