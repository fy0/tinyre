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
#if _MSC_VER >= 1800
#include <stdbool.h>
#endif
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#define _new(__obj_type, __size) (__obj_type*)malloc((sizeof(__obj_type)*(__size)))

static void putcode(int code) {
    if (code < 0xff) {
        putchar((char)code);
    } else {
        char* ret = ucs4_to_utf8(code);
        printf_u8(ret);
        free(ret);
    }
}

#ifdef _DEBUG
#define TRE_DEBUG_PRINT(str) printf_u8(str)
#else
#define TRE_DEBUG_PRINT(expression) ;
#endif

#endif

