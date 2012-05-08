/*
 * start : 2012-4-8 09:57
 * update: 2012-5-8
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

/* 回溯栈节点，此栈一定被使用 */
typedef struct btstack_node {
    char* re;
    char* s;
} btstack_node;

/* 次等回溯栈节点，此栈不一定被使用 */
typedef struct sec_btstack_node {
    int ecx;
} sec_btstack_node;

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

        /* 首先预测编译后长度，并据此为buf申请内存 */
        /* 全长，冒号，括号，问号，星号，加号，方括号，竖线，
         * 连字符，左花括号 */
        int len=0,cout_colon=0,cout_parentheses=0,cout_qsmark=0,
            cout_star=0,cout_plus=0,cout_square=0,cout_vv=0,
            cout_hyphen=0,cout_opencurly=0;
        s_foreach(re,p) {
            len++;
            switch (*p) {
                case '\\': len++;p++;break;
                case ':' : cout_colon++;break;
                case '(' : cout_parentheses++;break;
                case '?' : cout_qsmark++;break;
                case '*' : cout_star++;break;
                case '+' : cout_plus++;break;
                case '[' : cout_square++;break;
                case '|' : cout_vv++;break;
                case '-' : cout_hyphen++;break;
                case '{' : cout_opencurly++;break;
            }
        }
        /* 这边的内存预算几乎是一塌糊涂 */
        pr = ret = (char*)calloc(1,len+cout_colon+cout_qsmark+cout_star*7+cout_plus*8+cout_square*14+cout_vv*12+cout_hyphen+cout_opencurly*5+1);
        if (cout_parentheses) {
            ptn->groups = (_tre_group*)calloc(1,cout_parentheses*sizeof(_tre_group));
        }
        /* 开启次等回溯栈的标志 */
        if (cout_opencurly) *pr++ = '{';
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
            case '-'  :
                *(pr++) = '\\';
                *(pr++) = '-';
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
                        if (*p1!='\\') {
                            if (*p1=='-') num--;
                            else num++;
                        }
                        p1++;
                    }

                    /* 否定匹配模式 */
                    if (*(p+1)=='^') {
                        // 写入否定匹配指令
                        // 写入固定文本位置指令
                        memcpy(pr,":n:f",4);
                        pr+=4;
                        p1 = p+2;
                        while (*p1!=']'||*(p1-1)=='\\') {
                            switch (*p1) {
                                case '-':
                                    *pr = *(pr-1);
                                    *(pr-1) = '-';
                                    *++pr = *p1++;
                                    break;
                                case'*':case'.':case'+':case'?':case'(':case')':
                                case'[':case']':case'{':case'}':case'^':case'$':
                                case':':case'|':
                                    *pr++ = '\\';
                                    *pr++ = *(++p1);
                                    break;
                                default:
                                    *pr++ = *p1;
                            }
                            p1++;
                        }
                        memcpy(pr,":f",2);
                        pr+=2;
                        *pr++ = ']';
                        p = p1;
                        break;
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
                                case'-':
                                    if (index==0) {} //TODO:ERROR
                                    _buf[index-1].tlen = 3;
                                    p1++;
                                    break;
                                case'*':case'.':case'+':case'?':case'(':case')':
                                case'[':case']':case'{':case'}':case'^':case'$':
                                case':':case'|':
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
                                case':':case'|':
                                    *pr++ = '\\';
                                    break;
                                default:
                                    *pr++ = *p1++;
                            }
                        } else if (_buf[i].tlen==3) {
                            // for [a-z]
                            *pr++ = '-';
                            *pr++ = *p1++;
                            p1++;
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
                    else if (_buf[num-1].tlen==3) {
                        *pr = *(pr-1);
                        *(pr-1) = '-';
                        *++pr = *p1++;
                    }

                    /* 定位p指针，并将末尾字符写为] */
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
            case '{'  :
                {
                    char*p1;
                    bool __b=false;

                    for (p1=p+1;*p1!='}';p1++) {
                        if (*p1==',') {
                            __b=true;
                            break;
                        }
                    }

                    p1 = p+1;

                    if (__b) {
                        // 两个参数参数 a{x,y}
                        char* p2=p1+1;
                        while (*p2!=',') p2++;
                        char __buf[p2-p1+1];
                        __buf[p2-p1] = '\0';
                        memcpy(__buf,p1,p2-p1);
                        int x = atoi(__buf);

                        p1 = p2+1;
                        while (*p2!='}') p2++;
                        char __buf2[p2-p1+1];
                        __buf2[p2-p1] = '\0';
                        memcpy(__buf2,p1,p2-p1);
                        int y = atoi(__buf2);

                        /* 开始工作前的杂活，下面大段代码来自?*+部分 */
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

                        /* 写入指令 */
                        if (x==0) {
                            memcpy(pr,":p",2);
                            pr+=2;
                        } else {
                            memcpy(pr,":-:p",4);
                            pr+=4;
                        }

                        // 恢复数据
                        memcpy(pr,&buf,len);
                        pr += len;
                        // 写入跳转指令，参数一
                        memcpy(pr,":c",2);
                        pr+=2;
                        char* _buf = (char*)malloc(11);
                        // -2 是因为 :j 的长度是 2
                        int _len = int2str(0-len-2,_buf);
                        memcpy(pr,_buf,_len);
                        pr += _len;
                        *pr++ = ',';
                        // 参数二
                        _len = int2str(y,_buf);
                        memcpy(pr,_buf,_len);
                        pr += _len;
                        *pr++ = ',';
                        // 参数三
                        _len = int2str(x,_buf);
                        memcpy(pr,_buf,_len);
                        pr += _len;
                        *pr++ = ':';
                        // 末尾的计数比较指令
                        // 先写入y后写入x是为了优化
                        memcpy(pr,":c",2);
                        pr+=2;
                        memcpy(pr,_buf,_len);
                        pr += _len;
                        *pr++ = ':';
                        free(_buf);

                        // 对前面的分组进行修正处理
                        if (gtop>=0) {
                            int _offset = (x!=0) ? 4:2;
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
                    } else {
                        // 单个参数 a{x}
                        char* p2=p1+1;
                        while (*p2!='}') p2++;
                        char __buf[p2-p1+1];
                        __buf[p2-p1] = '\0';
                        memcpy(__buf,p1,p2-p1);
                        int xy = atoi(__buf);

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

                        // 写入跳转指令，参数一
                        int len = pr-start;
                        memcpy(pr,":c",2);
                        pr+=2;
                        char* _buf = (char*)malloc(11);
                        // -2 是因为 :j 的长度是 2
                        int _len = int2str(0-len-2,_buf);
                        memcpy(pr,_buf,_len);
                        pr += _len;
                        *pr++ = ',';
                        // 参数二
                        _len = int2str(xy,_buf);
                        memcpy(pr,_buf,_len);
                        pr += _len;
                        *pr++ = ',';
                        // 参数三
                        _len = int2str(xy,_buf);
                        memcpy(pr,_buf,_len);
                        pr += _len;
                        *pr++ = ':';
                        // 末尾的计数比较指令
                        // 先写入y后写入x是为了优化
                        memcpy(pr,":c",2);
                        pr+=2;
                        memcpy(pr,_buf,_len);
                        pr += _len;
                        *pr++ = ':';
                        free(_buf);
                    }
                    // 把后续无关文本跳过
                    while (*p!='}') p++;
                    break;
                }
            case '|'  :
                {
                    if (pr - ret == 0) return NULL;
                    /* 确定起始位置 */
                    char* start=NULL;

                    char *p1 = pr-1;
                    int _cout = 0;
                    // 由 !(((*p1=='('&&*(p1-1)!='\\')||(*p1=='|'&&*(p1-1)!='\\'))&&_cout==0) 转化
                    while (p1!=ret&&((*(p1-1)=='\\'||(*p1!='('&&*p1!='|'))||_cout!=0)) {
                        if (*(p1-1)!='\\') {
                            if (*p1==')') _cout++;
                            else if (*p1=='(') _cout--;
                        }
                        p1--;
                    }
                    if (p1==ret&&*p1!='(') start = p1;
                    else start = p1+1;
                    /* 写入指令 */
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
                    memcpy(pr,":p(",3);
                    pr+=3;
                    // 恢复数据
                    memcpy(pr,&buf,len);
                    pr += len;
                    // 跳转指令
                    memcpy(pr,"):jr:|",6);
                    pr+=6;
                    // 对前面的分组进行修正处理
                    if (gtop>=0) {
                        int _offset = 3;
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
                case 'D' : if (!isdigit(c)) return true;break;
                case 'w' : if (isalnum(c) || c == '_') return true;break;
                case 'W' : if (!isalnum(c) || c != '_') return true;break;
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

#define _exitmatch() {free(g_text);free(g_len);free(ret->groups);free(ret);free(sec_stack);return NULL;}
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

    /* 次等回溯栈，当存在{}时被启用 */
    sec_btstack_node* sec_stack = NULL;

    /* 文本位置固定开关，开启后s将不会再自增 */
    bool lockpos = false;
    /* 否定匹配开关 */
    bool nm = false;

    /* 计数变量，用于a{3}语法... */
    int ecx=0;

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
                /* 复位否定匹配 */
                if (nm) nm = false;
                /* 组的末尾 */
                for (int i=0;i<re->groupnum;i++) {
                    if (re->groups[i].end==p) {
                        g_len[i] = s-g_text[i];
                        break;
                    }
                }
                break;
            case '['  :
                break;
            case ']'  :
                /* 复位否定匹配 */
                if (nm) {
                    nm = false;
                }
                break;
            case ':'  :
                switch (*++p) {
                    case 'p' :
                        if (!_dumpstack) {
                            stack[++top].re = p;
                            stack[top].s = s;
                            if (sec_stack) sec_stack[top].ecx = ecx;
                        } else _dumpstack = false;
                        break;
                    case '-' :
                        _dumpstack = true;
                        break;
                    case 'j' :
                        {
                            char* p1 = p+1;
                            /* 右向跳转指令 :jr:
                             * 作用:将指针跳转到后面的括号
                             * 没有括号时候跳转到整个表达式的结束 */
                            if (*p1=='r') {
                                char *p2 = p+3;
                                char *p_end = re->re + strlen(re->re);
                                int _cout = 0; 
                                while ((*p2!=')'||*(p2-1)=='\\'||_cout!=0)&&p2!=p_end) {
                                    if (*p2=='('&&*(p2-1)=='\\') _cout++;
                                    else if (*p2==')'&&*(p2-1)=='\\') _cout--;
                                    p2++;
                                }
                                p = p2-1;
                            /* 左向跳转指令 :jl: */
                            } else if (*p1=='l') {
                                char *p2 = p-2;
                                int _cout = 0; 
                                while ((*p2!='('||*(p2-1)=='\\'||_cout!=0)&&p2!=re->re) {
                                    if (*p2==')'&&*(p2-1)=='\\') _cout++;
                                    else if (*p2=='('&&*(p2-1)=='\\') _cout--;
                                    p2--;
                                }
                                p = p2-1;
                            } else {
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
                            break;
                        }
                    case 'c' :
                        {
                            char* p1 = p+1;
                            bool __b=true;
                            for (char*p2=p1;*p2!=':';p2++) {
                                if (*p2==',') {
                                    __b = false;
                                    break;
                                }
                            }
                            if (__b) {
                                // 跳转比较指令
                                p1 = p+1;
                                while (*p1!=':') p1++;
                                #ifndef  _MSC_VER
                                char _buf[p1-p];
                                #else
                                char* _buf = (char*)malloc((p1-p)*sizeof(char));
                                #endif
                                _buf[p1-p-1] = '\0';
                                memcpy(_buf,p+1,p1-p-1);
                                int n = atoi(_buf);
                                p = p1;

                                if (ecx<n) {
                                    goto _failed;
                                    ecx=0;
                                }
                                break;
                            } else {
                                // 计数跳转指令
                                char* p2=p1;
                                while (*p2!=':') p2++;
                                #ifndef  _MSC_VER
                                char _buf[p2-p];
                                #else
                                char* _buf = (char*)malloc((p1-p2)*sizeof(char));
                                #endif
                                p2 = p;
                                // 获取首个参数
                                while (*p1!=',') p1++;
                                _buf[p1-p2-1] = '\0';
                                memcpy(_buf,p2+1,p1-p2-1);
                                int i = atoi(_buf);
                                p2 = p1++;
                                // 获取第二个参数
                                while (*p1!=',') p1++;
                                _buf[p1-p2-1] = '\0';
                                memcpy(_buf,p2+1,p1-p2-1);
                                int y = atoi(_buf);
                                p2 = p1++;
                                // 获取第三个参数
                                while (*p1!=':') p1++;
                                _buf[p1-p2-1] = '\0';
                                memcpy(_buf,p2+1,p1-p2-1);
                                int x = atoi(_buf);

                                ecx++;
                                if (ecx<=y) {
                                    if (ecx<x) _dumpstack = true;
                                    if (i<0) {
                                        p += (i-2);
                                    } else {
                                        p = p1+i;
                                    }
                                }
                            }
                            break;
                        }
                    /* 固定文本位置指令 */
                    case 'f'  :
                        lockpos = !lockpos;
                        if (!lockpos) s++;
                        break;
                    case 's' :
                        break;
                    case 'l' :
                        break;
                    case 'n'  :
                        nm = true;
                        break;
                }
                break;
            case '\\' :
                c[0] = '\\';
                c[1] = *++p;
                goto _def;
            case '-'  :
                /* 匹配 [a-z] */
                if (nm) {
                    printf("匹配的文本是%c，否定匹配状态\n",*s);
                }
                if (nm==(*s>=*(p+1)&&*s<=*(p+2))) {
                    goto _failed;
                }
                s++;
                p+=2;
                break;
            case '$'  :
                if (*s) _exitmatch();
                break;
            case '|'  :
                break;
            case '^'  :
                /* 匹配文本开头 */
                if (s!=start) goto _failed;
                break;
            case '{'  :
                /* 开启次等回溯栈 */
                if (!sec_stack) 
                    sec_stack = (sec_btstack_node*)malloc(len*sizeof(sec_btstack_node));
                break;
            default:
                c[0] = *p;
_def:           if (nm==match(c,*s)) {
_failed:            if (top<0) _exitmatch();
                    if (lockpos) lockpos = false;
                    if (nm) nm = false;
                    if (sec_stack) ecx = sec_stack[top].ecx;
                    p = stack[top].re+1;
                    s = stack[top--].s;

                    int _cout = 1;
                    /* 跳过回溯后遭遇的组 */
                    if (*p=='('||*p=='[') {
                        char _signl=*p,_signr;
                        if (_signl=='(') _signr = ')';
                        else _signr = ']';

                        while (*p!=_signr||_cout!=0) {
                            p++;
                            if (*p==_signl) _cout++;
                            else if (*p==_signr) _cout--;
                        }
                    /* 跳过 - 及其后的俩字符，为[a-z]语法而设定 */
                    } else if (*p=='-') p+=2;

                    if (*p=='\\') p++;
                    if (*(p+1)==':') {
                        if (*(p+2)=='j'||*(p+2)=='c') {
                            p+=2;
                            while (*p!=':') p++;
                        }
                    }
                } else if (!lockpos) s++;
                break;
        }
    }

    if (re->groupnum) {
        for (int i=0;i<re->groupnum;i++) {
            if (g_text[i]) {
                ret->groups[i+1].text = strndup(g_text[i],g_len[i]);
                if (re->groups[i].name)
                    ret->groups[i+1].name = strndup(re->groups[i].name,strlen(re->groups[i].name));
            }
        }
    }

    free(g_text);
    free(g_len);
    free(sec_stack);
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
 * 当前支持 : . ? * + | $ () [] {} \D \d \S \s \W \w  及 分组
 *
 *
 */
int main(int argc,char* argv[])
{
    char regex[] = ".?^[^ad]?(a{2,4})[a-z0-9]((aa)|b(b)b|d)[ab.c](((?P<aa>.)*3)?3)a-bc$";
    char text[]  = "caaafbbb.11333a-bc";

    /* 用法：
     * 编译表达式
     * 1. tre_pattern* re = tre_compile(regex);
     * 匹配文本
     * 2. tre_Match *ret = tre_match(re,text);
     * 取出文本
     * 3. printf("%s\n",ret->group(0).text);
     */

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

