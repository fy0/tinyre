/*
 * start : 2012-4-8 09:57
 * update: 2012-4-23
 *
 * tinyre
 * 微型python风格正则表达式库
 * 第七原型
 *
 * 首个实用版本：
 * 2012-4-23 16:58
 */

/* 这份代码可以在 gcc 和 clang 下，
 * 通过以下命令编译通过：
 * gcc main.c -std=c99
 * clang main.c
 * 在MSVC中可以使用C++方式编译通过，
 * 但不推荐。
 * [4.22后新版本暂时无法在VC中编译]
 */

#include <stdio.h>
#include <stdlib.h>
#ifndef  _MSC_VER
#include <stdbool.h>
#endif
#include <string.h>
#include <assert.h>
#include <ctype.h>

#define s_foreach(__s,__p) for (__p = __s;*__p;__p++)

#define pc(c) printf("_%c",c)
#define _p(x) printf("%s\n",x)

/* 回溯栈节点 */
typedef struct btstack_node {
    char* re;
    char* s;
} btstack_node;

/* 内部匹配使用的组 */
typedef struct _tre_group {
    char* name;
    char* start;
    char* end;
} _tre_group;

/* 返回结果使用的组 */
typedef struct tre_group {
    char*name;
    char*text;
} tre_group;

/* 编译后的表达式 */
typedef struct tre_pattern {
    int groupnum;
    char* re;
    _tre_group *groups;
} tre_pattern;

/* 匹配后返回的结果 */
typedef struct tre_Match {
    int groupnum;
    tre_group* groups;
} tre_Match;

#ifndef  __clang__
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
#endif

bool dotall = false;

static int
int2str(int val,char*buf)
{
    const unsigned int radix = 10;

    char* p;
    unsigned int a;        //every digit
    int len;
    char* b;            //start of the digit char
    char temp;
    unsigned int u;

    p = buf;
    if (val < 0)
    {
        *p++ = '-';
        val = 0 - val;
    }
    u = (unsigned int)val;

    b = p;
    do
    {
        a = u % radix;
        u /= radix;
        *p++ = a + '0';
    } while (u > 0);
    len = (int)(p - buf);
    *p-- = 0;
    //swap
    do
    {
        temp = *p;
        *p = *b;
        *b = temp;
        --p;
        ++b;

    } while (b < p);
    return len;
}

tre_pattern* tre_compile(char*re)
{
    char* ret;
    char*p,*pr;
    tre_pattern* ptn;

    int gtop = -1;

    if (!re) return NULL;
    else {
        ptn = (tre_pattern*)calloc(1,sizeof(tre_pattern));
        assert(ptn);

        /* 首先预测编译后长度，
         * 并据此为buf申请内存 */
        /* 全长，冒号，括号，问号，星号，加号，方括号*/
        int len=0,cout_colon=0,cout_parentheses=0,cout_qsmark=0,
            cout_star=0,cout_plus=0,cout_square=0;
        s_foreach(re,p) {
            len++;
            switch (*p) {
                case ':': cout_colon++;break;
                case '(': cout_parentheses++;break;
                case '?': cout_qsmark++;break;
                case '*': cout_star++;break;
                case '+': cout_plus++;break;
                case '[': cout_square++;break;
            }
        }
        /* 这边的内存预算几乎是一塌糊涂 */
        pr = ret = (char*)calloc(1,len+cout_colon+cout_qsmark+cout_star*7+cout_plus*8+cout_square*14+1);
        if (cout_parentheses) {
            ptn->groups = (_tre_group*)calloc(1,cout_parentheses*sizeof(_tre_group));
        }
    }
    s_foreach(re,p) {
        switch (*p) {
            case ':'  :
                *(pr++) = '\\';
                *(pr++) = ':';
                break;
            case '\\' :
                *(pr++) = '\\';
                *(pr++) = *(++p);
                break;
            case '?'  : case '+' : case '*' :
                {
                    // 当前点到开头的偏移位置
                    int offset = pr - ret;
                    if (!offset) return NULL; // 这是表达式开头就给个 ?

                    // 首先确定前方字符的开始位置
                    char* start=NULL;
                    if (offset==1) start = pr-1;
                    else if ((*(pr-1)!=')'&&*(pr-1)!=']')||*(pr-2)=='\\') {
                        if (*(pr-2)=='\\') start = pr - 2;
                        else start = pr - 1;
                    } else {
                        char _signl,_signr = *(pr-1);
                        if (_signr==')') _signl = '(';
                        else _signl = '[';

                        char *p1 = pr-1;
                        int _cout = 1;
                        while (*p1!=_signl||_cout!=0) {
                            p1--;
                            if (*p1==_signr) _cout++;
                            else if (*p1==_signl) _cout--;
                        }
                        start = p1;
                    }
                    // 申请缓存空间，len为括号内文本长度
                    int len = pr-start;
                    #ifndef  _MSC_VER
                    char buf[len];
                    #else
                    char* buf = (char*)malloc(len*sizeof(char));
                    #endif
                    // 备份要被覆盖的字串
                    memcpy(&buf,start,len);
                    pr = start;
                    // 根据运算符输入指令
                    char _op = *p;
                    if (*p=='+') {
                        memcpy(pr,":-:p",4);
                        pr+=4;
                    } else {
                        memcpy(pr,":p",2);
                        pr+=2;
                    }
                    // 恢复数据
                    memcpy(pr,&buf,len);
                    pr += len;
                    // 根据运算符输入指令
                    if (*p!='?') {
                        memcpy(pr,":j",2);
                        pr+=2;
                        char* _buf = (char*)malloc(11);
                        // -2 是因为 :j 的长度是 2
                        int _len = int2str(0-len-2,_buf);
                        memcpy(pr,_buf,_len);
                        free(_buf);
                        pr += _len;
                        *pr++ = ':';
                    }
                    // 对前面的分组进行修正处理
                    if (gtop>=0) {
                        int _offset = (_op=='+') ? 4:2;
                        for (int i=0;i<=gtop;i++) {
                            char *_pos = ptn->groups[i].start;
                            if (_pos>=start&&_pos<=pr) {
                                ptn->groups[i].start += _offset;
                            }
                            _pos = ptn->groups[i].end;
                            if (_pos>=start&&_pos<=pr) {
                                ptn->groups[i].end += _offset;
                            }
                        }
                    }
                    break;
                }
            case '['  :
                {
                    *pr++ = '[';
                    /* 确定[]内元素个数 */
                    char* p1 = p+1;
                    int num = 0;
                    while (*p1!=']'||*(p1-1)=='\\') {
                        if (*p1!='\\') num++;
                        p1++;
                    }

                    #ifndef  _MSC_VER
                    struct {
                        int tlen,jlen;
                    } _buf[num];
                    #else
                    /* 这里该怎么初始化呢？ */
                    struct {
                        int tlen,jlen;
                    } *_buf;
                    #endif

                    p1 = p+1;
                    int index=0;
                    while (*p1!=']'||*(p1-1)=='\\') {
                        if (*p1=='\\') {
                            _buf[index++].tlen = 2;
                            p1++;
                        } else {
                            /* 特殊字符转义 */
                            switch (*p1) {
                                case'*':case'.':case'+':case'?':case'(':case')':
                                case'[':case']':case'{':case'}':case'^':case'$':
                                case':':
                                    _buf[index++].tlen = 2;
                                    break;
                                default:
                                    _buf[index++].tlen = 1;
                            }
                        }
                        p1++;
                    }

                    /* 基础偏移量为末尾元素的长度 */
                    int offset = _buf[num-1].tlen;
                    /* 从后往前逐个计算偏移量 */
                    for (int i=num-2;i>=0;i--) {
                        int _jlen = _buf[i].jlen = offset;
                        /* 计算位数 */
                        int len_of_jlen=0;
                        _jlen+=1;
                        while (_jlen>0) {_jlen /= 10;len_of_jlen++;}
                        /* 压栈指令偏移(:p) */
                        offset += 2;
                        /* 用于匹配的字符的偏移 */
                        offset += _buf[i].tlen;
                        /* 跳转指令偏移(:jn:) */
                        offset += (3 + len_of_jlen);
                    }

                    /* 写入指令 */
                    /* 这里忽略了末尾元素，因末尾元素不需压栈和跳转 */
                    p1 = p+1;
                    for (int i=0;i<num-1;i++) {
                        /* 压栈指令部分 */
                        memcpy(pr,":p",2);
                        pr+=2;
                        /* 匹配字符部分 */
                        if (_buf[i].tlen==2) {
                            switch (*p1) {
                                case'*':case'.':case'+':case'?':case'(':case')':
                                case'[':case']':case'{':case'}':case'^':case'$':
                                case':':
                                    *pr++ = '\\';
                                    break;
                                default:
                                    *pr++ = *p1++;
                            }
                        }
                        *pr++ = *p1++;
                        /* 跳转指令部分 */
                        memcpy(pr,":j",2);
                        pr+=2;
                        char* ibuf = (char*)malloc(11);
                        int _len = int2str(_buf[i].jlen,ibuf);
                        memcpy(pr,ibuf,_len);
                        free(ibuf);
                        pr += _len;
                        *pr++ = ':';
                    }

                    /* 写入末尾元素 */
                    *pr++ = *p1++;
                    if (_buf[num-1].tlen==2) *pr++ = *p1++;

                    p = p1;
                    *pr++ = *p;

                    break;
                }
            case '('  :
                gtop++;
                ptn->groups[gtop].start = pr;

                if (*(p+1)=='?') {
                    if (*(p+2)=='P'&&*(p+3)=='<') {
                        char*p1 = p+4;
                        while (*p1!='>') p1++;
                        if (p1==p+4) {}; // TODO:这里报错
                        ptn->groups[gtop].name = strndup(p+4,p1-(p+4));
                        p = p1;
                    }
                    *(pr++) = '(';
                    break;
                }
                goto _def;
            case ')'  :
                if (gtop>=0) {
                    _tre_group*pg = ptn->groups + gtop;
                    while (pg->end&&pg>=ptn->groups) pg--;
                    pg->end = pr;
                }
                goto _def;
            default   :  _def :
                *(pr++) = *p;
        }
    }
    ptn->groupnum = gtop+1;
    ptn->re = ret;
    return ptn;
}

static bool
match(char re[2],char c)
{
    if (c=='\0') return false;

    char c1 = re[0];
    char c2 = re[1];
    switch (c1) {
        case '.' :
            if (dotall || c!='\n') return true;
            break;
        case '\\' :
            switch (c2) {
                case 'd' : if (isdigit(c)) return true;break;
                case 's' : if (isspace(c)) return true;break;
                case 'S' : if (!(isspace(c))) return true;break;
                default  : if (c2==c) return true;
            }
            break;
        default   :
            if (c1==c) {
                return true;
            }
    }
    return false;
}

#define _exitmatch() {free(g_text);free(g_len);free(ret->groups);free(ret);return NULL;}
tre_Match* tre_match(tre_pattern*re,char*s)
{
    int len = strlen(s);
    if (!len) return NULL;

    tre_Match* ret = (tre_Match*)calloc(1,sizeof(tre_Match));

    len = len > strlen(re->re) ? len : strlen(re->re);

    #ifndef  _MSC_VER
    btstack_node stack[len];
    #else
    btstack_node* stack = (btstack_node*)malloc(len*sizeof(btstack_node));
    #endif
    int top = -1;

    char *p,*start=s;
    char c[2];

    /* 提供对组机制的支持 */
    const char** g_text = NULL;
    int *g_len = NULL;

    if (re->groupnum) {
        g_text  = (const char**)calloc(re->groupnum,sizeof(char*));
        g_len   = (int*)calloc(re->groupnum,sizeof(int));
        ret->groupnum = re->groupnum;
    }

    bool _dumpstack = false;

    ret->groups = (tre_group*)calloc(re->groupnum+1,sizeof(tre_group));

    s_foreach(re->re,p) {
        switch (*p) {
            case '('  :
                /* 接下来的东西在组内 */
                for (int i=0;i<re->groupnum;i++) {
                    if (re->groups[i].start==p) {
                        if (*s) g_text[i] = s;
                        break;
                    }
                }
                break;
            case ')'  :
                /* 组的末尾 */
                for (int i=0;i<re->groupnum;i++) {
                    if (re->groups[i].end==p) {
                        g_len[i] = s-g_text[i];
                        break;
                    }
                }
                break;
            case '['  : case ']'  :
                break;
            case ':'  :
                switch (*++p) {
                    case 'p' :
                        if (!_dumpstack) {
                            stack[++top].re = p;
                            stack[top].s = s;
                        } else _dumpstack = false;
                        break;
                    case '-' :
                        _dumpstack = true;
                        break;
                    case 'j' :
                        {
                            char* p1 = p+1;
                            while (*p1!=':') p1++;
                            #ifndef  _MSC_VER
                            char _buf[p1-p];
                            #else
                            char* _buf = (char*)malloc((p1-p)*sizeof(char));
                            #endif
                            _buf[p1-p-1] = '\0';
                            memcpy(_buf,p+1,p1-p-1);
                            int i = atoi(_buf);
                            if (i<0)
                                p += (i-2);
                            else {
                                p = p1+i;
                            }
                        }
                }
                break;
            case '\\' :
                c[0] = '\\';
                c[1] = *++p;
                goto _def;
            case '$'  :
                if (*s) _exitmatch();
                break;
            default:
                c[0] = *p;
_def:           if (!match(c,*s)) {
                    if (top<0) _exitmatch();
                    p = stack[top].re+1;
                    s = stack[top--].s;

                    int _cout = 1;
                    if (*p=='('||*p=='[') {
                        char _signl=*p,_signr;
                        if (_signl=='(') _signr = ')';
                        else _signr = ']';

                        while (*p!=_signr||_cout!=0) {
                            p++;
                            if (*p==_signl) _cout++;
                            else if (*p==_signr) _cout--;
                        }
                    }
                    if (*p=='\\') p++;
                    if (*(p+1)==':') {
                        if (*(p+2)=='j') {
                            p+=2;
                            while (*p!=':') p++;
                        }
                    }
                } else s++;
                break;
        }
    }

    if (re->groupnum) {
        for (int i=0;i<re->groupnum;i++) {
            ret->groups[i+1].text = strndup(g_text[i],g_len[i]);
            if (re->groups[i].name)
                ret->groups[i+1].name = strndup(re->groups[i].name,strlen(re->groups[i].name));
        }
    }

    free(g_text);
    free(g_len);
    ret->groups[0].text = strndup(start,s-start);
    return ret;
}

void tre_freepattern(tre_pattern*re)
{
    if (re->groupnum) {
        _tre_group* g;
        for (int i=0;i<re->groupnum;i++) {
            g = re->groups + i;
            // 提醒：free(NULL); 不算错误
            free(g->name);
        }
    }
    free(re->groups);
    free(re->re);
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

/* tinyre
 * 这是一个从头设计的正则引擎。
 *
 * 当前仅支持 : . ? * + $ () [] \d \s \S
 *
 * 支持分组
 *
 */
int main(int argc,char* argv[])
{
    char regex[] = "[ab.c](((?P<aa>.)*3)?3)$";
    char text[]  = ".11333";

    /* 用法：
     * 编译表达式
     * 1. tre_pattern* re = tre_compile(regex);
     * 匹配文本
     * 2. tre_Match *ret = tre_match(re,text);
     * 取出文本
     * 3. printf("%s\n",ret->group(0).text);
     */

    //tre_pattern* ret = tre_compile("(a*)*");
    tre_pattern* ret = tre_compile(regex);
    if (ret) {
        // 输出表达式编译相关信息
        printf("表达式编译为：%s\n",ret->re);
        printf("共有组 %d 个。\n",ret->groupnum);
        if (ret->groupnum) {
            _tre_group* g;
            for (int i=0;i<ret->groupnum;i++) {
                g = ret->groups + i;
                printf("组 %d ，开始位置 %7x 结束位置 %7x ('%c','%c') ",i+1,
                        g->start,g->end,*g->start,*g->end);
                if (g->name) printf("组名：%s\n",g->name);
                else printf("无组名\n");
            }
        }
        /*char*p;
        s_foreach(ret->re,p) {
            pc(*p);
        }
        putchar('\n');*/
        putchar('\n');

        // 进行匹配并输出相关信息
        tre_Match *r = tre_match(ret,text);
        if (r) {
            printf("匹配文本：%s\n",r->groups[0].text);
            if (r->groupnum) {
                for (int i=1;i<r->groupnum+1;i++) {
                    printf("组 %d ，匹配文本 %s ，",i,r->groups[i].text);
                    if (r->groups[i].name) printf("组名：%s\n",r->groups[i].name);
                    else printf("无组名\n");
                }
            }
            tre_freematch(r);
        } else {
            _p("匹配失败！");
        }
        tre_freepattern(ret);
    }
    return 0;
}

