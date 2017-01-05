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
#include <wchar.h>

#define tre_new(__obj_type, __size) (__obj_type*)malloc((sizeof(__obj_type)*(__size)))

typedef struct tre_Stack {
    void* data;
    int top;
    int len;
} tre_Stack;


#define stack_init(_s, _type, _len) { (_s).data = _len ? tre_new(_type, _len) : NULL; (_s).top = -1; (_s).len = _len; }
#define stack_get_top(_s, _type) ((_type*)((_s).data) + (_s).top)
#define stack_empty(_s) (_s.top == -1)
#define stack_push(_s, _type) ((_type*)(_s.data) + ++(_s).top)
#define stack_pop(_s, _type) ((_type*)((_s).data) + (_s).top--)
#define stack_check(_s, _type, _step) if (_s.top == _s.len || _s.len == 0) { _s.len += _step; _s.data = _s.data ? realloc(_s.data, sizeof(_type) * _s.len) : tre_new(_type, _s.len);}
#define stack_free(_s) free((_s).data);
#define stack_copy(_s, _dest, _type) { (_dest).data = tre_new(_type, (_s).len);memcpy((_dest).data, (_s).data, sizeof(_type) * ((_s).top+1)); }

#endif

