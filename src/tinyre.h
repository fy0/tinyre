/**
 * tinyre v0.9.0
 * fy, 2012-2015
 *
 */

#ifndef TINYRE_H
#define TINYRE_H

#include <stdint.h>

enum tre_Flag {
    FLAG_NONE = 0,
    //FLAG_TEMPLATE = 1,
    FLAG_IGNORECASE = 2,
    //FLAG_LOCALE = 4,
    FLAG_MULTILINE = 8,
    FLAG_DOTALL = 16,
    //FLAG_UNICODE = 32,
    //FLAG_VERBOSE = 64,
    //FLAGTRE_DEBUG = 128,

    //FLAG_T = 1,
    FLAG_I = 2,
    //FLAG_L = 4,
    FLAG_M = 8,
    FLAG_S = 16,
    //FLAG_U = 32,
    //FLAG_X = 64,
};

/* compiled groups */
typedef struct MatchGroup {
    uint32_t* name;
    int name_len;
    int type;
    int extra;
    uint32_t* codes;
} MatchGroup;

typedef struct tre_Pattern {
    int num; // group num
    int num_all; // group num include non-grouping parentheses (?:) (?=) ..
    MatchGroup* groups;
    int flag;
} tre_Pattern;

/* 匹配后返回的结果 */

typedef struct tre_GroupResult {
    uint32_t *name;
    int name_len;
    int head;
    int tail;
} tre_GroupResult;


typedef struct tre_Match {
    int groupnum;
    uint32_t* str;
    tre_GroupResult* groups;
} tre_Match;

/* 表达式编译和匹配 */
tre_Pattern* tre_compile(char* s, int flag, int* err_code);
tre_Match* tre_match(tre_Pattern* tp, const char* str, int backtrack_limit);

/* 释放内存占用 */
void tre_pattern_free(tre_Pattern *ptn);
void tre_match_free(tre_Match *m);

/* 其他 */
void tre_err(int err_code);

#endif

