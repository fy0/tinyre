/**
 * tinyre v0.9.0
 * fy, 2012-2015
 *
 */

#ifndef TINYRE_UTILS_H
#define TINYRE_UTILS_H

#include "lib/platform.h"
#include "lib/utf8_lite.h"
#include "tinyre.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <math.h>

#define _new(__obj_type, __size) (__obj_type*)malloc((sizeof(__obj_type)*(__size)))

static void putcode(uint32_t code) {
    if (code < 0xff) {
        putchar((char)code);
    } else {
        char* ret = ucs4_to_utf8(code);
        // bug: printf_u8(ret);
        printf_u8("%s", ret);
        free(ret);
    }
}

#endif

