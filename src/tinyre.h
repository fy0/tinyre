
#ifndef _TINYRE_H_
#define _TINYRE_H_

#define tre_pattern_none   0
#define tre_pattern_or     1
#define tre_pattern_dotall 2

/* 回溯组，内部使用 */
struct _tre_node {
    void* head;
    void* tail;
    char flag;
};

/* 分组 */
typedef struct tre_group {
    char* name;
    char* head;
    char* tail;
    void* tmp;
} tre_group;

/* 编译后的表达式 */
typedef struct tre_Pattern {
    void* code;
    int btcount;
    struct _tre_node* btinfo;
    int groupnum;
    tre_group* groups;
    char flag;
} tre_Pattern;

/* 匹配后返回的结果 */
typedef struct tre_Match {
    int groupnum;
    tre_group* groups;
} tre_Match;

/* 表达式编译和匹配 */
tre_Pattern* tre_compile(char* s, char flag);
tre_Match* tre_match(tre_Pattern* tp, char* str);

/* 释放内存占用 */
void tre_free_pattern(tre_Pattern *ptn);
void tre_free_match(tre_Match *m);

/* 辅助功能 */
void tre_printstr(char* head, char*tail);

#endif

