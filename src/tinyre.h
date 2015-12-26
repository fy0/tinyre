/**
 * tinyre v0.9.0
 * fy, 2012-2015
 *
 */

#ifndef TINYRE_H
#define TINYRE_H

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
    char* name;
    int type;
    int extra;
    int* codes;
} MatchGroup;

typedef struct tre_Pattern {
    int num; // group num
    int num_all; // group num include non-grouping parentheses (?:) (?=) ..
    MatchGroup* groups;
    int flag;
} tre_Pattern;

/* 匹配后返回的结果 */

typedef struct tre_GroupResult {
    char* name;
    int* head;
    int* tail;
} tre_GroupResult;


typedef struct tre_Match {
    int groupnum;
    tre_GroupResult* groups;
} tre_Match;

/* 表达式编译和匹配 */
tre_Pattern* tre_compile(char* s, int flag);
tre_Match* tre_match(tre_Pattern* tp, const char* str, int backtrack_limit);

/* 释放内存占用 */
//void tre_free_pattern(tre_Pattern *ptn);
//void tre_free_match(tre_Match *m);

#endif
