/**
 * tinyre v0.9.0
 * fy, 2012-2015
 *
 */

#ifndef TINYRE_H
#define TINYRE_H

/* compiled groups */
typedef struct MatchGroup {
	char* name;
	int* codes;
} MatchGroup;

typedef struct tre_Pattern {
	int num;
	MatchGroup* groups;
    char flag;
} tre_Pattern;

/* 匹配后返回的结果 */

typedef struct tre_group {
	char* name;
	char* head;
	char* tail;
	char* tmp;
} tre_group;

typedef struct tre_Match {
    int groupnum;
    tre_group* groups;
} tre_Match;

/* 表达式编译和匹配 */
tre_Pattern* tre_compile(char* s, char flag);
//tre_Match* tre_match(tre_Pattern* tp, char* str);

/* 释放内存占用 */
//void tre_free_pattern(tre_Pattern *ptn);
//void tre_free_match(tre_Match *m);

#endif
