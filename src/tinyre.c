/*
 * start : 2012-4-8 09:57
 * update: 2012-12-08
 *
 * tinyre
 * 正则表达式引擎
 * 第八原型 - 12.10.9
 *
 */

#include "tinyre.h"

#include <stdio.h>
#include <stdlib.h>
#ifndef  _MSC_VER
#include <stdbool.h>
#endif
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#ifdef py_lib_27
#include <python2.7/Python.h>
#endif

#define s_foreach(__s,__p) for (__p = __s;*__p;__p++)
#define _s_foreach(__s,__p) for (;*__p;__p++)
#define _new(__obj_type, __size) (__obj_type*)malloc((sizeof(__obj_type)*(__size)))

#define pc(c) printf("_%c",c)
#define _p(x) printf("%s\n",x)

#define byte unsigned char

#define ins_cmp      0
#define ins_cmp_spe  1
#define ins_mcmp     2
#define ins_ncmp     3
#define ins_ncmp_spe 4
#define ins_mncmp    5
#define ins_nop      6
#define ins_push     7
#define ins_escape   8
#define ins_jmp      9
#define ins_jmpl     10
#define ins_jmp_gt   11
#define ins_grp_head 20
#define ins_grp_tail 21
#define ins_begin    30
#define ins_end      31
#define ins_eof      40

typedef struct _tre_node tre_node;

/* 回溯栈节点 */
typedef struct tre_btstack {
    char* s;
    void* re;
    void* head;
    char flag;
} tre_btstack;

/* 内部匹配使用的组 */
typedef struct tre_group_link {
    char* name;
    char* head;
    char* tail;
    void* tmp;
    struct tre_group_link* _next;
} tre_group_link;

typedef struct tre_linknode {
    void* head;
    void* tail;
    char flag;
    struct tre_linknode* _next;
} tre_linknode;

typedef struct tre_precheck_info {
    char* s;
    int btcount;
    tre_node* btinfo;
    int groupnum;
    tre_group *groups;
    char flag;
} tre_precheck_info;


inline void tre_err(const char* s);
inline static bool do_ins_cmp(char *re, char*c, char flag);
inline char* getlastpos(char* p, char* start);

#ifndef  __clang__

/*static char*
strndup (const char *s, size_t n)
{
    char *result;
    size_t len = strlen (s);

    if (n < len)
        len = n;

    result = (char *) malloc (len + 1);
    if (!result)
        return 0;

    result[len] = '\0';
    return (char *) memcpy (result, s, len);
}
*/

#if __STDC_VERSION__ == 199901L
char* strdup(char* s)
{
    char* ret;
    int len = strlen(s);
    ret = (char*)malloc(len+1);
    return (char*)memcpy(ret, s, len+1);
}
#endif

#endif

char* write_ins(void**pcode, byte ins, ...)
{
    void* p = *pcode;
    va_list va;
    va_start(va, ins);

    switch (ins) {
        case ins_cmp: 
        case ins_cmp_spe:
        {
            char c;
            *(byte*)p++ = ins;
            c = va_arg(va, int);
            *(char*)p++ = c;
            break;
        }
        case ins_mcmp:
        case ins_mncmp:
        {
            char*s = va_arg(va, char*);
            int count = 0;
            byte* pIns = (byte*)p++;
            int* pi = (int*)p;
            p = (byte*)(pi + 1);

            while (*++s != ']') {
                if (*s == '\\') {
                    *(byte*)p++ = ins_cmp_spe;
                    *(char*)p++ = *(++s);
                } else {
                    *(byte*)p++ = ins_cmp;
                    *(char*)p++ = *s;
                }
                count++;
            }
            va_end(va);
            if (count != 0) {
                *pIns = ins;
                *pi = count;
                *pcode = p;
            }
            return s;
        }
        default:
            *(byte*)p++ = ins;
            break;
    }

    *pcode = p;
    va_end(va);
    return NULL;
}

void tre_err(const char* s) {
    printf("%s\n", s);
    exit(-1);
}

/* 括号匹配，返回左括号位置，从括号后一个字符开始匹配 */
char* getlastpos(char* p, char* start) {
    if (p-- == start) {
        //ERR 词法错误: 无法匹配至开头
        tre_err("ERR 无法匹配至开头");
        return NULL;
    }
    switch (*p) {
        case ')':
            {
                int count = 1;
                char l = '(', r = ')';
                for (p--; p >=  start; p--) {
                    if (*p == l && *(p-1) != '\\') {
                        if (--count == 0) return p;
                    } else if (*p == r) count++;
                }
                tre_err("ERR 括号不能正确匹配");
                return NULL;
            }
        case ']':
            {
                for (p--; p >=  start; p--) {
                    if (*p == '[' && *(p-1) != '\\') return p;
                }
                tre_err("ERR 括号不能正确匹配");
                return NULL;
            }
        default:
            return p;
    }
}

/* 扫描分组，扫描基本词法 */
tre_precheck_info* tre_precheck(char* s)
{
    char *p;
    char *buf;
    int btcount = 0, grpcount = 0;
    tre_precheck_info* ret;
    tre_linknode *btnode = _new(tre_linknode,1), *btlist = btnode;
    tre_group_link *grpnode = _new(tre_group_link,1), *grplist = grpnode;
    char* lastpos;

    /* 对 | 提供支持 */
    /* TODO: 优化 - 这里能够整合到一起 */
    bool is_pattern_flag_changed = false;
    tre_group_link** grplast = _new(tre_group_link*, 16);
    tre_linknode** partlast = _new(tre_linknode*, 16);

    memset(grplast, 0, sizeof(tre_group_link*)*16);
    memset(partlast, 0, sizeof(tre_linknode*)*16);

    tre_group_link** grplast_start = grplast;
    tre_linknode** partlast_start = partlast;

#define init_grp() \
    grpnode->name = NULL; \
    grpnode->tmp = (void*)1;

#define next_grp() \
    grpnode = grpnode->_next = _new(tre_group_link,1); \
    grpnode->_next = NULL;

#define work_grp() \
    grplast++; \
    partlast++; \
    *grplast = grpnode;

    btnode->_next = NULL;
    grpnode->_next = NULL;
    buf = (char*)malloc(strlen(s));

    for (p = s + strlen(s); p >= s; p--) {
        switch (*p) {
            case ')':
                if (*(p-1)=='\\') break;
                grpcount++;
                lastpos = getlastpos(p+1, s);
                init_grp();
                grpnode->head = lastpos;
                grpnode->tail = p;
                work_grp();
                next_grp();
                break;
            case ']':
                if (*(p-1)=='\\') break;
                p = getlastpos(p+1, s);
                break;
            case '*':
            case '+':
            case '?':
                {
                    if (*(p-1)=='\\') break;
                    btcount++;
                    lastpos = getlastpos(p, s);
                    btnode->head = lastpos;
                    btnode->tail = p-1;
                    btnode->flag = *p;
                    if (lastpos!=p-1) {
                        grpcount++;
                        init_grp();
                        grpnode->head = lastpos;
                        grpnode->tail = p-1;
                        work_grp();
                        next_grp();
                        p--;
                    }
                    btnode = btnode->_next = _new(tre_linknode,1);
                    btnode->_next = NULL;
                    if (*(p-1) == ']') p = lastpos;
                    break;
                }
            case '}':
                if (*(p-1)=='\\') break;
                p--;
                while (*p != '{' && *(p-1) != '\\') {
                    //TODO: 完善这个功能;
                    if (isdigit(*p) || *p == ',');
                    else {
                        // TODO:抛出异常
                    }
                    p--;
                }
            case '|':
                if (*(p-1)=='\\') break;
                btcount++;
                if (*partlast) {
                    /* flag 有值表示这是一个正常回溯组 */
                    (*partlast)->head = p;
                    (*partlast)->flag = '|';
                }
                *partlast = btnode;
                btnode->tail = p;
                btnode->flag = 0;
                if (*grplast) {
                    btnode->head = (*grplast)->head;
                    /* 这表示当前组中存在 | 因此要在组开始后写入一个 push */
                    (*grplast)->tmp = (void*)2;
                } else {
                    /* 这里只会在未进入任何组时候被访问 */
                    btnode->head = (void*)tre_pattern_or;
                    is_pattern_flag_changed = true;
                }
                btnode = btnode->_next = _new(tre_linknode,1);
                btnode->_next = NULL;
                break;
            case '(':
                if (*(p-1)=='\\') break;
                if (--grplast < grplast_start) ;
                    //TODO: ERR;
                partlast--;
                break;
        }
    }
    free(buf);

    ret = _new(tre_precheck_info, 1);
    if (is_pattern_flag_changed)
        ret->flag = tre_pattern_or;
    else
        ret->flag = tre_pattern_none; 
    ret->btcount = btcount;
    ret->groupnum = grpcount;
    if (grpcount) {
        //tre_group_link* tmp = grplist;
        tre_group* grpinfo = _new(tre_group, grpcount), *pinfo = grpinfo;

        /* 按 head 排序，由于是单向链表，解链成本很高 */
        /* TODO: 记得回来优化 */

        void** grps = _new(void*, grpcount), **grps_first = grps;
        void** grp_heads = _new(void*, grpcount), **grp_heads_first = grp_heads;

        while (grplist->_next) {
            *(grps++) = grplist;
            *(grp_heads++) = grplist->head;
            grplist = grplist->_next;
        }

        int i,j;
        void* head_min;
        int head_min_pos;

        grps = grps_first;
        grp_heads = grp_heads_first;

        for (i=0; i<grpcount-1; i++) {
            head_min_pos = i;
            head_min = *(grp_heads + i);
            for (j=i+1; j<grpcount; j++) {
                if (*(grp_heads+j) < head_min) {
                    head_min = *(grp_heads+j);
                    head_min_pos = j;
                }
            }
            memcpy(pinfo++, *(grps + head_min_pos) , sizeof(tre_group));
            free(*(grps + head_min_pos));
            *(grps + head_min_pos) = *(grps + i);
            *(grp_heads + head_min_pos) = *(grp_heads + i);
        }
        memcpy(pinfo++, *(grps + grpcount - 1) , sizeof(tre_group));
        free(*(grps + grpcount - 1));
        free(grps);
        free(grp_heads);
/*      旧的复制代码
        grplist = tmp;

        while (grplist->_next) {
            tmp = grplist->_next;
            memcpy(pinfo--, grplist, sizeof(tre_group));
            free(grplist);
            grplist = tmp;
        }*/
        ret->groups = grpinfo;
    }

    if (btcount) {
        tre_linknode* tmp;
        tre_node* btinfo = _new(tre_node, btcount), *pinfo = btinfo + btcount - 1;

        while (btlist->_next) {
            tmp = btlist->_next;
            memcpy(pinfo--, btlist, sizeof(tre_node));
            free(btlist);
            btlist = tmp;
        }
        ret->btinfo = btinfo;
    }

    free(grplist);
    free(btlist);

    free(grplast_start);
    free(partlast_start);

    ret->s = s;
    return ret;
}

void debug_print_checkresult(char *raw, tre_precheck_info* info)
{
    tre_node *node;
    tre_group *grp;
    printf("原始字符串   : %s\n", raw);
    printf("回溯信息数目 : %d\n", info->btcount);

    printf("节点信息:\n");
    if (info->btcount) {
        for (node = info->btinfo; node < info->btinfo + info->btcount; node++) {
            if (node->head == (void*)tre_pattern_or)
                printf("头    -> %x 尾   -> %x flag:%d\n",  node->head, node->tail, node->flag);
            else
                printf("头 %c%c -> %x 尾 %c%c -> %x flag:%d\n", *(char*)node->head, *(char*)(node->head+1) , node->head, *(char*)(node->tail-1), *(char*)node->tail, node->tail, node->flag);
        }
        putchar('\n');
    }
 
    if (info->groupnum) {
        printf("分组信息:\n");
        for (grp = info->groups; grp < info->groups + info->groupnum; grp++) {
            printf("头 %c -> %x 尾 %c -> %x \n", *(char*)grp->head, grp->head, *(char*)grp->tail, grp->tail);
        }
        putchar('\n');
    }
}

tre_Pattern* tre_compile(char* s, char flag)
{

    char* pc;
    char* new;

    void* code = malloc(1024);
    void* p = code;

    tre_precheck_info* info =  tre_precheck(s);
#ifdef DEBUG
    debug_print_checkresult(s, info);
#endif

    new = info->s;
    flag |= info->flag;

    if (info->flag == tre_pattern_or) {
        tre_node *_node;
        for (_node = info->btinfo; _node <= info->btinfo + (info->btcount - 1); _node++) {
            if (_node->head == (void*)tre_pattern_or) {
                write_ins(&p, ins_push);
                _node->head = p;
                break;
            }
        }
    }

    s_foreach(new, pc) {
        switch (*pc) {
            /* 回溯组后缀 */
            case '?':
                write_ins(&p, ins_nop);
                break;
            case '+':
            case '*':
                write_ins(&p, ins_jmpl);
                break;
            case '\\':
                write_ins(&p, ins_cmp_spe, *++pc);
                break;
            case '^':
                write_ins(&p, ins_begin);
                break;
            case '$':
                write_ins(&p, ins_end);
                break;
            case '[':
                if (*(byte*)(pc+1)=='^') pc = write_ins(&p, ins_mncmp, pc+1);
                else pc = write_ins(&p, ins_mcmp, pc);
                break;
            default:
                {
                    tre_group *_g;
                    tre_node *_node;

                    if (info->btcount) {
                        /* 回溯组前缀 */
                        for (_node = info->btinfo; _node <= info->btinfo + (info->btcount - 1); _node++) {

                            /* 存在 flag 表示这是一个正常回溯组，不存在flag表示这是一个用于 | 的卖萌回溯组，
                             * 卖萌回溯组将由下面 *pc == '(' 的卖萌回馈单元进行处理 */
                            if (_node->head == pc && _node->flag) {
                                if (_node->flag == '+')
                                    write_ins(&p, ins_escape);
                                write_ins(&p, ins_push);
                                _node->head = p;


                                if (_node->tail == pc)
                                    _node->tail = p;
                                break;
                            }

                            if (_node->tail == pc) {
                                _node->tail = p;
                                if (*pc=='|') {
                                    /* 占位用途 */
                                    write_ins(&p, ins_jmp_gt);
                                    write_ins(&p, ins_nop);
                                    write_ins(&p, ins_nop);
                                    continue;
                                }
                                break;
                            }

                        }
                    }

                    if (*pc == '(') {
                        for (_g = info->groups; _g <= info->groups + info->groupnum - 1; _g++) {
                            if (_g->head == pc) {
                                _g->head = p;
                                write_ins(&p, ins_grp_head);
                                /* 下面这段代码为了实现 | 而写 */
                                /* tmp==2 表示当前组中存在 | 因此要在组开始后写入一个 push */
                                if (_g->tmp == (void*)2) {
                                    write_ins(&p, ins_push);
                                    /*TODO:有优化可能，重点可能是回溯组的访问顺序 */
                                    for (_node = info->btinfo; _node <= info->btinfo + (info->btcount - 1); _node++) {
                                        if (_node->head == pc && !_node->flag)
                                            _node->head = p;
                                    }
                                }
                                /* 下面这段代码为了实现组的辅助功能而写 */
                                if (*(pc+1) == '?') {
                                    /* 分组名支持 */
                                    if (*(pc+2)=='P' && *(pc+3)=='<') {
                                        char*p1 = pc+4;
                                        while (*p1!='>') p1++;
                                        if (p1==pc+4) {}; // TODO: 错误, 没有组名
                                        _g->name = strndup(pc+4,p1-(pc+4));
                                        pc = p1;
                                    }
                                }
                                break;
                            }
                        }
                    } else if (*pc == ')') {
                        for (_g = info->groups ; _g <= info->groups + info->groupnum - 1; _g++) {
                            if (_g->tail == pc) {
                                _g->tail = p;
                                write_ins(&p, ins_grp_tail);
                                break;
                            }
                        }
                    } else if (*pc == '.')
                        write_ins(&p, ins_cmp_spe, *pc);
                    else if (*pc == '|')
                        ;
                    else
                        write_ins(&p, ins_cmp, *pc);
                    break;
                }
        }
    }

    write_ins(&p, ins_eof);

    tre_Pattern *ret = _new(tre_Pattern, 1);
    ret->flag = flag;
    ret->code = code;
    ret->btinfo = info->btinfo;
    ret->btcount = info->btcount;
    ret->groups = info->groups;
    ret->groupnum = info->groupnum;
    free(info);
    return ret;
}

void debug_printcode(void* code)
{
    char*p;
    for (p=code;*p!=ins_eof;p++) {
        switch (*p) {
            case ins_cmp:
                printf("%x cmp   %c   ; 比较指令\n", p, *(p+1));
                p++;
                break;
            case ins_cmp_spe:
                printf("%x cmps  %c   ; 特殊比较指令\n", p, *(p+1));
                p++;
                break;
            case ins_eof:
                printf("%x eof        ; 结束标志\n", p);
                break;
            case ins_escape:
                printf("%x esc       ; 弃栈指令\n", p);
                break;
            case ins_jmp:
                printf("%x jmp        ; 跳转指令\n", p);
                break;
            case ins_jmpl:
                printf("%x jmpl      ; 左向跳转指令\n", p);
                break;
            case ins_push:
                printf("%x push      ; 压栈指令\n", p);
                break;
            case ins_nop:
                printf("%x nop       ; 空指令\n", p);
                break;
            case ins_grp_head:
                printf("%x grp_head  ; 分组头\n", p);
                break;
            case ins_grp_tail:
                printf("%x grp_tail  ; 分组尾\n", p);
                break;
            case ins_mcmp:
                printf("%x mcmp      ; 多重匹配指令 : ", p);
                goto mcmp;
            case ins_mncmp:
                printf("%x mncmp     ; 否定多重匹配指令 : ", p);
mcmp:           {
                    int i;
                    int* pi = (int*)(p + 1);
                    int count = *pi;
                    printf("%d ", count);
                    p = (char*)(pi + 1);
                    for (i=0;i<count;i++) {
                        if (*p++==ins_cmp) putchar(*p++);
                        else printf("\\%c", *p++);
                    }
                    putchar('\n');
                    p--;
                }
                break;
            case ins_begin:
                printf("%x begin     ; 匹配头\n", p);
                break;
            case ins_end:
                printf("%x end       ; 匹配尾\n", p);
                break;
            case ins_jmp_gt:
                printf("%x jmp_gt    ; 跳至分组尾\n", p);
                break;
            default:
                printf("%x 无法识别 %d\n", p+1, *p);
        }
    }
    putchar('\n');
}

void debug_checknode2(tre_Pattern* info) {
    tre_node* node;
    printf("节点信息:\n");
    if (info->btcount) {
        for (node = info->btinfo; node < info->btinfo + info->btcount; node++) {
            printf("头 %d -> %x 尾 %d -> %x \n", *(char*)node->head, node->head, *(char*)node->tail, node->tail);
        }
    }
    putchar('\n');
    
}

void debug_checkgroup(tre_Pattern* info) {
    tre_group* node;
    printf("分组信息:\n");
    if (info->groupnum) {
        for (node = info->groups; node < info->groups + info->groupnum; node++) {
            printf("头 %d -> %x 尾 %d -> %x \n", *(char*)node->head, node->head, *(char*)node->tail, node->tail);
        }
        putchar('\n');
    }
}

tre_Match* tre_match(tre_Pattern* tp, char* str)
{
    byte* p = tp->code;
    char* pc = str;
    tre_btstack* bts = _new(tre_btstack, tp->btcount * 5 + 5);
    tre_btstack* bts_end = bts + tp->btcount * 5 - 1;
    tre_btstack* pb = bts - 1;
    int pbcount = 0;
    tre_node* pn = tp->btinfo;
    tre_group* groups = _new(tre_group, tp->groupnum + 1);
    tre_group* groups_head = groups + 1;
    bool bEscape = false;
    memcpy(groups_head, tp->groups, sizeof(tre_group) * tp->groupnum);
#ifdef DEBUG
    debug_checknode2(tp);
    debug_checkgroup(tp);
    debug_printcode(p);
    printf("\n开始匹配\n");
#endif

    while (true) {
        switch (*p) {
            case ins_cmp:
#ifdef DEBUG
                printf("%x cmp   %c %c   ; 比较指令\n", p, *(p+1), *pc);
#endif
                if (*(char*)++p != *pc++) {
                    goto failed;
                }
                break;
            case ins_cmp_spe:
#ifdef DEBUG
                printf("%x cmps  %c %c   ; 特殊比较指令\n", p, *(p+1), *pc);
#endif
                if (!do_ins_cmp((char*)++p, pc++, tp->flag)) {
                    goto failed;
                }
                break;
            case ins_jmpl:
                {
#ifdef DEBUG
                    printf("%x jmpl        ; 前向跳转指令\n", p);
#endif
                    /* -1 是与下面的 p++ 抵消*/
                    p = pb->head - 1;
                    break;
                }
            case ins_push:
#ifdef DEBUG
                printf("%x push        ; 压栈指令\n", p);
#endif
                {
                    tre_node* _p;

                    if (pb >= bts_end) {
                        int offset = (int)(pb - bts);
                        int _size = ((int)(bts_end - bts) + 1) * 2;
                        while (true) {
                            bts = realloc(bts, sizeof(tre_btstack)*_size);
                            if (bts) {
                                bts_end = bts + _size - 1;
                                pb = bts + offset;
                                break;
                            }
                        }
                    }
                    (++pb)->s = pc;
                    pb->head = p;
                    for (_p = pn; _p < pn + tp->btcount; _p++) {
                        if (_p->head == p+1) {
                            if (*(byte*)(_p->tail) == 0)
                                pb->re = _p->tail+3;
                            else pb->re = _p->tail + 2;
                        break;
                    }
                    }
                    if (bEscape) {
                        bEscape = false;
                        pb->flag = 0;
                    } else {
                        pb->flag = 1;
                        pbcount ++;
                    }
#ifdef DEBUG
                    //printf("; 栈顶  : %x\n", pb);
                    //printf("; s     : %x\n", pb->s);
                    //printf("; re    : %x\n", pb->re);
#endif
                    break;
                }
            case ins_escape:
                bEscape = true;
                break;
            case ins_grp_head:
#ifdef DEBUG
                printf("%x grp_head    ; 分组头\n", p);
#endif
                {
                    tre_group *_g;
                    for (_g = groups_head ; _g <= groups_head + tp->groupnum - 1; _g++) {
                        if ((byte*)_g->head == p) {
                            _g->tmp = pc;
                            break;
                        }
                    }
                    break;
                }
            case ins_grp_tail:
#ifdef DEBUG
                printf("%x grp_tail    ; 分组尾\n", p);
#endif
                {
                    tre_group *_g;
                    for (_g = groups_head ; _g <= groups_head + tp->groupnum - 1; _g++) {
                        if ((byte*)_g->tail == p) {
                            _g->head = _g->tmp;
                            _g->tail = pc-1;
                            _g->tmp = NULL;
                            break;
                        }
                    }
                    break;
                }
            case ins_mcmp:
            case ins_mncmp:
#ifdef DEBUG
                printf("%x mcmp        ; 多重匹配指令 : ", p);
#endif
                {
                    int i;
                    bool m = false;
                    bool bMatch = (*p == ins_mcmp) ? true : false;
                    int* pi = (int*)(p + 1);
                    int count = *pi;
                    p = (byte*)(pi + 1);

                    for (i=0;i<count;i++) {
                        bool b = false;
#ifdef DEBUG
                        putchar(*(char*)(p+1));
#endif
                        if (*p++==ins_cmp) b = *(char*)p++ == *pc;
                        else b = do_ins_cmp((char*)p++, pc, tp->flag);

                        if (b) {
                            m = true;
                            break;
                        }
                    }
#ifdef DEBUG
                    putchar('\n');
#endif
                    if (m != bMatch) goto failed;

                    pc++;
                    p = (byte*)(pi + 1);
                    p += 2 * count - 1;
                    break;
                }
            case ins_jmp_gt:
#ifdef DEBUG
                printf("%x jmp_gt      ; 跳至分组尾\n", p);
#endif
                {
                    /* 若无 group_now 那么明显是在最外层，这时直接执行 eof */
                    /* TODO:找出当前所在分组，这里先使用一个无脑的算法，以后再优化 */
                    tre_group *curg;
                    if (tp->groupnum == 1) {
                        curg = groups_head;
                    } else {
                        tre_group *_g;
                        curg = NULL;
                        void* offset, *offset_min = (void*)(p - (byte*)groups_head->head);

                        for (_g = groups_head + 1; _g <= groups_head + tp->groupnum - 1; _g++) {
                            offset = (void*)(p - (byte*)_g->head);
                            if ((offset < offset_min) && (p <= (byte*)_g->tail)) {
                                offset_min = offset;
                                curg = _g;
                            }
                        }
                    }
                    if (curg) p = (byte*)curg->tail;
                    else goto end;
                    continue;
                }
            case ins_begin:
                if (pc != str) goto failed;
                break;
            case ins_end:
                if (*pc) goto failed;
                break;
            case ins_eof:
                goto end;
        }
        p++;
        continue;
failed:
#ifdef DEBUG
        printf("匹配失败 %c\n", *(pc-1));
#endif
        if (pbcount--) {
#ifdef DEBUG
            printf("返回回溯点\n");
#endif
            while (!pb->flag) pb--;
            p = pb->re;
            pc = (pb--)->s;
        } else {
            free(bts);
            free(groups);
            return NULL;
        }
    }

end:;
    groups[0].name = NULL;
    groups[0].tmp = NULL;
    groups[0].head = str;
    groups[0].tail = pc-1;

    tre_Match* m = _new(tre_Match, 1);
    m->groupnum = tp->groupnum;
    m->groups = groups;
    free(bts);
    return m;
}

inline static bool
do_ins_cmp(char *re, char*c, char flag)
{
    if (*c=='\0') return false;

    switch (*re) {
        case '.' :
            if (*c=='\0') return false;
            if (flag & tre_pattern_dotall) return true;
            else if (*c!='\n') return true;
            break;
        case 'd' : if (isdigit(*c)) return true;break;
        case 'D' : if (!isdigit(*c)) return true;break;
        case 'w' : if (isalnum(*c) || *c == '_') return true;break;
        case 'W' : if (!isalnum(*c) || *c != '_') return true;break;
        case 's' : if (isspace(*c)) return true;break;
        case 'S' : if (!(isspace(*c))) return true;break;
        default  : if (*re==*c) return true;
    }
    return false;
}

void tre_free_pattern(tre_Pattern *ptn) {
    free(ptn->code);
    if (ptn->btcount)
       free(ptn->btinfo);
    if (ptn->groupnum)
        free(ptn->groups);
    free(ptn);
}

void tre_free_match(tre_Match *m) {
    int i;
    for (i = 1; i <= m->groupnum; i++)
        if (m->groups[i].name) free(m->groups[i].name);
    free(m->groups);
    free(m);
}

void tre_printstr(char* head, char*tail) {
    char*p;
    for (p=head;p<=tail;p++) {
        putchar(*p);
    }
}

#ifdef py_lib_27
void trepy_free_pattern(PyObject* obj)
{
    tre_free_pattern((tre_Pattern*)PyCapsule_GetPointer(obj, "_tre_pattern"));
}

static PyObject* trepy_compile(PyObject *self, PyObject* args)
{
    char* re;
    char flag;

    if(!PyArg_ParseTuple(args, "sb", &re, &flag)) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    tre_Pattern* ret = tre_compile(re, flag);
    if (ret) {
        PyObject *o = PyCapsule_New(ret,"_tre_pattern", trepy_free_pattern);
        Py_INCREF(o);
        return o;
    } else {
        Py_INCREF(Py_None);
        return Py_None;
    }
}

inline PyObject* tre_Match_c2py(tre_Match* m)
{
    int i;
    PyObject* t = PyTuple_New(m->groupnum + 1);

    for (i = 0;i<m->groupnum+1;i++) {
        PyObject* t2 = PyTuple_New(2);
        if (m->groups[i].name)
            PyTuple_SetItem(t2, 0, PyString_FromString(m->groups[i].name));
        else {
            Py_INCREF(Py_None);
            PyTuple_SetItem(t2, 0, Py_None);
        }

        if (!m->groups[i].tmp) {
            char* text = strndup(m->groups[i].head, m->groups[i].tail - m->groups[i].head + 1);
            PyTuple_SetItem(t2,1,PyString_FromString(text));
            free(text);
        } else {
            Py_INCREF(Py_None);
            PyTuple_SetItem(t2, 1, Py_None);
        }
        PyTuple_SetItem(t, i, t2);
    }

    tre_free_match(m);
    return t;
}

static PyObject* trepy_match(PyObject *self, PyObject* args)
{
    tre_Pattern* re;
    PyObject* obj;
    char * text;

    if(!PyArg_ParseTuple(args, "Os", &obj, &text)) return NULL;
    re = (tre_Pattern*)PyCapsule_GetPointer(obj,"_tre_pattern");

    tre_Match* m = tre_match(re, text);
    if (!m) {
         Py_INCREF(Py_None);
         return Py_None;
    }

    return tre_Match_c2py(m);
}

static PyObject* trepy_compile_and_match(PyObject *self,PyObject* args)
{    
    char *re,*text, flag=0;
    if(!PyArg_ParseTuple(args, "ssb", &re, &text, &flag)) return NULL;

    tre_Pattern* p = tre_compile(re, flag);
    if (!p) {
         Py_INCREF(Py_None);
         return Py_None;
    }
    tre_Match* m = tre_match(p,text);
    if (!m) {
         Py_INCREF(Py_None);
         return Py_None;
    }

    return tre_Match_c2py(m);
}

static PyMethodDef tre_methods[] ={
    {"_compile", trepy_compile, METH_VARARGS},
    {"_match", trepy_match, METH_VARARGS},
    {"_compile_and_match", trepy_compile_and_match, METH_VARARGS},
    {NULL, NULL,0,NULL}
};

PyMODINIT_FUNC init_tre (void)
{
    Py_InitModule("_tre", tre_methods);
}
#endif

#ifdef DEMO

int main(int argc,char* argv[])
{
    //char regex[] = ".?^d+?(?#asdasd)a?(?!ad)([^1-9][^ac])(?:a{2,4})[a-z0-9]((aa)|b(b)b|d)[ab.c](((?P<aa>.)*3)?3)a-bce*?";
    //char text[]  = "dacbaaafbbb.11333a-bceee";
    
    //tre_Pattern* ptn =  tre_compile("^[^\\s](.c?e*d*(ha)*)d*(?P<asd>ff|e(?P<c>g|s))|cds$", tre_pattern_dotall);
    //tre_Match *m = tre_match(ptn, "a\ndddddhahahadddes");

    tre_Pattern* ptn =  tre_compile("a*", tre_pattern_dotall);
    tre_Match *m = tre_match(ptn, "a");

    if (m) {
        int i;
        for (i = 0;i<m->groupnum+1;i++) {
            printf("分组 %d :", i);
            if (!m->groups[i].tmp)
                tre_printstr(m->groups[i].head, m->groups[i].tail);
            if (m->groups[i].name)
                printf(" 组名:%s", m->groups[i].name);
            putchar('\n');
        }
        printf("匹配成功 \n");
        tre_free_match(m);
    } else 
        printf("匹配失败\n");

    tre_free_pattern(ptn);

   /* 用法：
     * 编译表达式
     * 1. tre_pattern* re = tre_compile(regex);
     * 匹配文本
     * 2. tre_Match *ret = tre_match(re,text);
     * 取出文本
     * 3. printf("%s\n",ret->group(0).text);
     */

    return 0;
}

#endif

