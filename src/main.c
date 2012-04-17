/*
 * start : 2012-4-8 09:57
 * tiny re
 * 微型python风格正则表达式库
 * 第六原型a (DFA)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

/* 关于 from 和 prev：
 * from 一般只用于并联情况，
 * 若这个值不存在，则视为 from 与 prev 等价
 */

typedef struct node {
    char re[2];
    /* 跳转而来的前一节点 */
    struct node *from;
    /* 逻辑关系上的前一节点 */
    struct node *prev;
    /* 成功后的后续节点 */
    struct node *next;
    /* 失败后转向的节点 */
    struct node *failed;
} tre_node;

/* 内部匹配使用的组 */
typedef struct _tre_group {
    char* name;
    tre_node* start;
    tre_node* end;
    struct _tre_group *prev;
    struct _tre_group *next;
} _tre_group;

/* 返回结果使用的组 */
typedef struct tre_group {
    char*name;
    char*text;
} tre_group;

/* 编译后的表达式 */
typedef struct tre_pattern {
    int groupnum;
    _tre_group *groups;
    tre_node *start;
} tre_pattern;

/* 匹配后返回的结果 */
typedef struct tre_Match {
    int groupnum;
    tre_group* groups;
} tre_Match;


static char*
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

#define s_foreach(__s,__p) for (__p = __s;*__p;__p++)

#define _p(x) printf("%s\n",x)

#define newnode() (tre_node*)calloc(1,sizeof(tre_node))

bool dotall = false;

/* 第六原型 (DFA) 
 * 这里实现了对 [] () ? + * 的支持 */
tre_pattern* compile(char*re)
{
    tre_pattern *ret = (tre_pattern*)calloc(1,sizeof(tre_pattern));
    tre_node*start = (tre_node*)calloc(1,sizeof(tre_node));

    ret->start = start;

    assert(start);
    
    if (strlen(re)==0) {
        return ret;
    }

    /* 并行模式，用于[]中的单字 */
    bool parellelmode = false;
    char *p;
    /* 首次指定为头节点，自此以后每次由当前字符处理结束后生成 */
    tre_node *node = start;
    /* 这是 group 链表的尾节点 */
    _tre_group *grouptail = NULL;

    s_foreach(re,p) {
        switch (*p) {
            case '\\':
                node->re[0] = '\\';
                node->re[1] = *(++p);
                if (!parellelmode) {
                    node->next = newnode();
                    node->next->prev = node;
                    node = node->next;
                } else {
                    node->failed = newnode();
                    node->failed->prev = node->prev;
                    node->failed->from = node;
                    node = node->failed;
                }
                break;
            case '?' :
                if (!parellelmode) node->prev->failed = node;
                else goto __def;
                break;
            case '+' :
                if (!parellelmode) {
                    *node = *node->prev;
                    node = node->failed = newnode();
                } else goto __def;
                break;
            case '*' : 
                if (!parellelmode) {
                    node->prev->next = node->prev;
                    node->prev->failed = node;
                } else goto __def;
                break;
            case '|' :
                if (!grouptail) {
                    ;
                } else {
                    /* 斩断旧有联系 */
                    /*node->prev->next = NULL;
                    tre_node* _node = start;
                    while (_node->failed) _node = _node->failed;
                    _node->failed = _node;
                    node->from = _node;
                    node->prev = NULL;
                    for (;_node;_node=_node->next)
                        if (!_node->failed) _node->failed = node;*/
                }
                break;
            case '(' :
                ret->groupnum++;
                if (!grouptail)
                    grouptail = ret->groups = (_tre_group*)calloc(1,sizeof(_tre_group));
                else {
                    grouptail->next = (_tre_group*)calloc(1,sizeof(_tre_group));
                    grouptail->next->prev = grouptail;
                    grouptail = grouptail->next;
                }
                if (*(p+1)=='?') {
                    if (*(p+2)=='P'&&*(p+3)=='<') {
                        char*p1 = p+4;
                        while (*p1!='>') p1++;
                        if (p1==p+4) {}; // TODO:这里报错
                        grouptail->name = strndup(p+4,p1-(p+4));
                        p = p1;
                    }
                }
                grouptail->start = node;
                break;
            case ')':
                if (grouptail) {
                    _tre_group *pg = grouptail;
                    /* TODO:括号不对齐时这里将会崩溃 */
                    while (pg->end) pg = pg->prev;
                    pg->end = node->prev;
                    break;
                } else goto __def;
            case '[' :
                if (parellelmode) goto __def;
                parellelmode = true;
                break;
            case ']' :
                if (parellelmode) {
                    parellelmode = false;
                    /* 断绝并联关系 */
                    node->from->failed = NULL;
                    /* 调整节点指向 */
                    tre_node* _node = node->from;
                    node->from = NULL;
                    if (!_node) break;
                    do {
                        _node->next = node;
                        _node = _node->from;
                    } while (_node);
                    break;
                }
            default : __def:
                node->re[0] = *p;
                if (!parellelmode) {
                    node->next = newnode();
                    node->next->prev = node;
                    node = node->next;
                } else {
                    /* node->next 将在]结束后统一添加 */
                    node->failed = newnode();
                    node->failed->prev = node->prev;
                    node->failed->from = node;
                    node = node->failed;
                }
                break;
        }
    }
    /* 没有必要的尾节点 */
    //node->prev->next = NULL;
    //free(node);
    return ret;
}

static bool
match(tre_node*re,char c)
{
    char c1 = re->re[0];
    char c2 = re->re[1];
    switch (c1) {
        case '.' : 
            if (dotall || c!='\n') return true;
            break;
        case '\\' :
            switch (c2) {
                case 'd' :
                    if (isdigit(c)) return true;
                    break;
                case 's' :
                    if (isspace(c)) return true;
                    break;
                case 'S' :
                    if (!(isspace(c))) return true;
                    break;
                default  :
                    if (c2==c) return true;
            }
            break;
        default   :
            if (c1==c) {
                return true;
            }
    }
    return false;
}

tre_Match* tre_match(tre_pattern*re,char*s)
{
    char* start=s;
    tre_Match *ret = (tre_Match*)calloc(1,sizeof(tre_Match));
    tre_node *p = re->start;

    /* 为加速组机制设置的变量 */
    const char** g_text = NULL;
    tre_node **g_start = NULL;
    tre_node **g_end = NULL;

    int groupnum = ret->groupnum = re->groupnum;

    /* 初始化组机制 */
    if (groupnum) {
        ret->groups = (tre_group*)calloc(10,sizeof(tre_group));
        g_start = (tre_node**)calloc(groupnum,sizeof(tre_node*));
        g_end   = (tre_node**)calloc(groupnum,sizeof(tre_node*));
        g_text  = (const char**)calloc(groupnum,sizeof(char*));
        int i = 0;
        for (_tre_group* pg = re->groups;pg;pg = pg->next) {
            g_start[i] = pg->start;
            g_end[i]   = pg->end;
            if (pg->name) {
                ret->groups[i+1].name = strndup(pg->name,strlen(pg->name));
            }
            i++;
        }
    }

    while (true) {
        if (groupnum) {
            for (int i=0;i<groupnum;i++) {
                if (g_start[i]) {
                    if (g_start[i]==p) {
                        /* 找到组的开头了 */
                        g_text[i] = s;
                        g_start[i] = NULL;
                        /* 若组内只有一个字符 */
                        if (g_end[i]==p) {
                            i--;
                            continue;
                        }
                        break;
                    }
                } else {
                    /* 匹配组的末尾 */
                    if (g_end[i]&&g_end[i]==p) {
                        /* 找到组的末尾 */
                        ret->groups[i+1].text = strndup(g_text[i],s-g_text[i]+1);
                        g_end[i] = NULL;
                        break;
                    }
                }
            }
        }

        bool _ret = match(p,*s);
        if (_ret && p->next) p = p->next;
        else if (p->failed) {
            p = p->failed;
            continue;
        }
        else break;
        if (!*(++s)) break;
    }
    if (!ret->groupnum) ret->groups = (tre_group*)calloc(1,sizeof(tre_group));
    //else assert(realloc(ret->groups,ret->groupnum+1));
    ret->groups[0].text = strndup(start,s-start);
    return ret;
}

/* tinyre 正则 第六原型(DFA) a版
 * 支持 [] () . * ? \d \s \S
 */
int main(int argc,char* argv[])
{
    tre_pattern* re = compile("a.*");
    tre_Match* ret = tre_match(re,"a[2]");
    if (ret) {
        _p(ret->groups[0].text);
    }
    return 0;
}

