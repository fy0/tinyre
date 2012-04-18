/*
 * start : 2012-4-8 09:57
 * tiny re
 * 微型python风格正则表达式库
 * 第六原型b (DFA)
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
 * from 用于：
 * 1. 并联时 (|) 和 [] 指示当前节点是
 *    哪一个失败后跳转而来的。
 * 2. 在 ) 节点中标示 ( 的位置，方便
 *    * + ? 等运算符进行查找。
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

#define pc(c) printf("_%c",c)
#define _p(x) printf("%s\n",x)

#define newnode() (tre_node*)calloc(1,sizeof(tre_node))

bool dotall = false;

/* 关于 . + * 和组 
 * 对于这三个修饰符，我实现了一个支持接口：
 * 在 ( 节点上附加了 failed 属性，我担心这个特性将来会和回溯冲突。
 * */

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
                if (!parellelmode) {
                    tre_node* prev = node->prev;
                    if (!prev) ; /* TODO:这里报错:非法的"?" */
                    if (prev->re[0]==')') {
                        prev->from->failed = node;
                    } else node->prev->failed = node;
                } else goto __def;
                break;
            case '+' :
                if (!parellelmode) {
                    tre_node* prev = node->prev;
                    if (!prev) ; /* TODO:这里报错:非法的"+" */
                    if (prev->re[0]==')') {
                        if (prev->from->next != prev) {
                            for (tre_node _node = prev->from->next;_node!=prev;_node = _node->next) {
                                /* 复制一段节点，这个恐怕递归才能做到 */
                            }
                            prev->next = prev->from;
                        }
                    } else {
                        *node = *node->prev;
                        node = node->failed = newnode();
                    }
                } else goto __def;
                break;
            case '*' : 
                if (!parellelmode) {
                    tre_node* prev = node->prev;
                    if (!prev) ; /* TODO:这里报错:非法的"+" */
                    if (prev->re[0]==')') {
                        prev->next = prev->from;
                        prev->from->failed = node;
                    } else {
                        node->prev->next = node->prev;
                        node->prev->failed = node;
                    }
                } else goto __def;
                break;
            /* 这段算了，不好宁愿不要。
             * 这个实现的设计有点问题，
             * next 和 failed 的交替并不能总是好好工作。
            case '|' :
                if (!grouptail) {
                    ;
                } else {
                    * 斩断旧有联系 *
                    node->prev->next = NULL;
                    tre_node* _node = start;
                    while (_node->failed) _node = _node->failed;
                    _node->failed = _node;
                    node->from = _node;
                    node->prev = NULL;
                    for (;_node;_node=_node->next)
                        if (!_node->failed) _node->failed = node;
                }
                break;*/
            case '(' :
                node->re[0] = '(';
                node->next = newnode();
                node->next->prev = node;
                node = node->next;

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
                grouptail->start = node->prev;
                break;
            case ')':
                if (grouptail) {
                    _tre_group *pg = grouptail;
                    /* TODO:括号不对齐时这里将会崩溃 */
                    while (pg->end) pg = pg->prev;
                    node->re[0] = ')';
                    node->from = pg->start;
                    node->next = newnode();
                    node->next->prev = node;
                    node = node->next;
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
        if (p->re[0] == '(') {
            /* 接下来的东西在组内 */
            for (int i=0;i<groupnum;i++) {
                if (g_start[i]&&g_start[i]==p) {
                    g_text[i] = s;
                    g_start[i] = NULL;
                    break;
                }
            }
            p = p->next;
        } else if (p->re[0] == ')') {
            /* 组的末尾 */
            for (int i=0;i<groupnum;i++) {
                if (g_end[i]==p) { // (!g_start[i])&&
                    ret->groups[i+1].text = strndup(g_text[i],s-g_text[i]);
                    g_end[i] = NULL;
                    break;
                }
            }
            p = p->next;
        } else if (p->re[0] == '\0') {

            if (!groupnum) {
                ret->groups = (tre_group*)calloc(1,sizeof(tre_group));
            }
            else {
                free(g_start);
                free(g_end);
                free(g_text);
            }
            ret->groups[0].text = strndup(start,s-start);
            return ret;
        } else {
            bool _ret = match(p,*s);
            if (_ret && p->next) p = p->next;
            else if (p->failed) {
                p = p->failed;
                continue;
            } else break;
            s++;
        }
    }

    if (groupnum) {
        free(g_start);
        free(g_end);
        free(g_text);
        for (int i=0;i<=groupnum;i++) {
            free(ret->groups[i].name);
            free(ret->groups[i].text);
        }
        free(ret->groups);
    }
    free(ret);
    return NULL;
}

void tre_freepattern(tre_pattern*re)
{
    _tre_group* g;
    if (re->groups) {
        for (g=re->groups ; g->next ; g=g->next) {
            if (g->prev) {
                free(g->prev->name);
                free(g->prev);
            }
        }
        if (g->prev) {
            free(g->prev->name);
            free(g->prev);
        }
        free(g->name);
        free(g);
    }

    tre_node* n;
    for (n=re->start ; n->next ; n=n->next) {
        if (n->prev) {
            free(n->prev);
        }
    }
    free(n->prev);
    free(n);

    free(re);
}

void tre_freematch(tre_Match*m)
{
    for (int i=0;i<=m->groupnum;i++) {
        free(m->groups[i].name);
        free(m->groups[i].text);
    }
    free(m->groups);
    free(m);
}

/* tinyre 正则 第六原型(DFA) b版
 * 支持 [] () . * ? \d \s \S
 */
int main(int argc,char* argv[])
{
    tre_pattern* re = compile("(a(?P<g1>d))");
    for (tre_node* node = re->start;node;node = node->next) {
        printf("节点 %x 表达式: %1c",node,node->re[0]);
        if (node->re[0]=='\0') putchar(' ');
        if (node->re[0]=='\\') printf("%c ",node->re[1]);
        else printf("  ");
        printf("成功转向: %7x " , node->next);
        printf("失败转向: %7x " , node->failed);
        printf("上个节点: %7x " , node->prev);
        printf("from : %7x " , node->from);
        putchar('\n');
    }
    tre_Match* ret = tre_match(re,"ad");
    if (ret) {
        _p(ret->groups[0].text);
        _p(ret->groups[1].text);
        _p(ret->groups[2].text);
    } else {
        _p("匹配失败");
    }
    /* 这段代码在[]这样的路线选择上还有问题，回头再改 */
    tre_freepattern(re);
    if (ret) tre_freematch(ret);
    return 0;
}

