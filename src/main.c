/*
 * start : 2012-4-8 09:57
 * tiny re
 * 微型python风格正则表达式库
 * 第七原型
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#define s_foreach(__s,__p) for (__p = __s;*__p;__p++)

#define pc(c) printf("_%c",c)
#define _p(x) printf("%s\n",x)

typedef struct btstack_node {
    char* re;
    char* s;
} btstack_node;

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

char* tre_compile(char*re)
{
    char* ret;
    char*p,*pr;

    if (!re) return NULL;
    else {
        /* 首先预测编译后长度，
         * 并据此为buf申请内存 */
        /* 长度 */
        int len=0;
        /* 冒号，括号，问号，星号，加号，方括号*/
        int cout_colon=0,cout_parentheses=0,cout_qsmark=0,
            cout_star=0,cout_plus=0;//cout_square=0;
        s_foreach(re,p) {
            len++;
            switch (*p) {
                case ':': cout_colon++;break;
                case '(': cout_parentheses++;break;
                case '?': cout_qsmark++;break;
                case '*': cout_star++;break;
                case '+': cout_plus++;break;
                //case '[': cout_square++;break;
            }
        }
        pr = ret = (char*)calloc(1,len+cout_colon+cout_qsmark+cout_star*7+cout_plus*8+1);
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
                    else if (*(pr-1)!=')'||*(pr-2)=='\\') {
                        if (*(pr-2)=='\\') start = pr - 2;
                        else start = pr - 1;
                    } else {
                        char *p1 = pr-1;
                        while (*p1!='(') p1--;
                        start = p1;
                    }
                    // 申请缓存空间
                    int len = pr-start;
                    char buf[len];
                    // 备份要被覆盖的字串
                    memcpy(&buf,start,len);
                    pr = start;
                    // 根据运算符输入指令
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
                        char* _buf = (char*)malloc(12);
                        int _len = int2str(0-(len+1),_buf);
                        memcpy(pr,_buf,_len);
                        free(_buf);
                        pr += _len;
                        *pr++ = ':';
                    }
                    break;
                }
            default   :
                *(pr++) = *p;
        }
    }
    return ret;
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

//#define _pop() 
#define _exitmatch() return NULL;
char* tre_match(char*re,char*s)
{
    int len = strlen(s);
    if (!len) return NULL;

    btstack_node stack[len];
    int top = 0;

    char *p,*start=s;
    char c[2];

    bool _dumpstack = false;

    s_foreach(re,p) {
        switch (*p) {
            case '('  :
                break;
            case ')'  :
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
                            char _buf[p1-p];
                            _buf[p1-p-1] = '\0';
                            memcpy(&_buf,p+1,p1-p-1);
                            int i = atoi(_buf);
                            p += (i-3);
                        }
                }
                break;
            case '\\' :
                c[0] = '\\';
                c[1] = *++p;
                goto _def;
            default:
                c[0] = *p;
_def:           if (!match(c,*s)) {
                    if (!top) _exitmatch();
                    p = stack[top].re+1;
                    s = stack[top--].s;
                    int _cout = 0;
                    if (*p=='(') {
                        while (*p!=')'||_cout!=0) {
                            p++;
                            //if (*p=='(') _cout++;
                            //else if (*p==')') _cout--;
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
    return strndup(start,s-start);
}

void tre_freepattern(char*re)
{
    free(re);
}

/* 第七原型
 * 这是一个从头设计的正则引擎。
 *
 * 当前仅支持：. ? * + \d \s \S
 * 同时请注意括号不要嵌套
 *
 * 匆匆做完，不知道问题有多少。
 * 此项目定案，这个看起来原型不错。
 */
int main(int argc,char* argv[])
{
    char* ret = tre_compile("(\\d)*");
    if (ret) {
        /*char*p;
        s_foreach(ret,p) {
            pc(*p);
        }
        putchar('\n');*/
        char *r = tre_match(ret,"12");
        if (r) {
            _p(r);
            free(r);
        } else {
            _p("匹配失败！");
        }
        tre_freepattern(ret);
    }
    return 0;
}

