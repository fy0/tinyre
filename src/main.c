/*
 * start : 2012-4-8 09:57
 * update: 2012-4-22
 * tinyre
 * 微型python风格正则表达式库
 * 第七原型
 */

/* 这份代码可以在 gcc 和 clang 下，
 * 通过以下命令编译通过：
 * gcc main.c -std=c99
 * clang main.c
 * 在MSVC中可以使用C++方式编译通过，
 * 但不推荐。
 * [4.22 新版本暂时无法在VC中编译]
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

/* 编译后的表达式 */
typedef struct tre_pattern {
    int groupnum;
    char* re;
} tre_pattern;

typedef struct btstack_node {
    char* re;
    char* s;
} btstack_node;

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
        /* TODO:为分组申请内存 */
        if (cout_parentheses) {
            //ptn->groups = (_tre_group*)malloc(cout_parentheses*sizeof(_tre_group));
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
                    else if (*(pr-1)!=')'||*(pr-2)=='\\') {
                        if (*(pr-2)=='\\') start = pr - 2;
                        else start = pr - 1;
                    } else {
                        char *p1 = pr-1;
                        int _cout = 1;
                        while (*p1!='('||_cout!=0) {
                            p1--;
                            if (*p1==')') _cout++;
                            else if (*p1=='(') _cout--;
                        }
                        start = p1;
                    }
                    // 申请缓存空间
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
            case '['  :
                {
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
                        } else _buf[index++].tlen = 1;
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
                        *pr++ = *p1++;
                        if (_buf[i].tlen==2) *pr++ = *p1++;
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

                    /* 编译后[abc]将形如:p a :j5: :p b :j2: c */
                    /* 跳转总距离为(元素数-1)个len(":p") 加
                     * len(元素本身) 加 len(跳转指令长度) */
                    break;
                }
            case '('  :
                /* TODO:组 */
                goto _def;
            case ')'  :
                /* TODO:组 */
                goto _def;
            default   :  _def :
                *(pr++) = *p;
        }
    }
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

//#define _pop() 
#define _exitmatch() return NULL;
char* tre_match(tre_pattern*re,char*s)
{
    int len = strlen(s);
    if (!len) return NULL;

    #ifndef  _MSC_VER
    btstack_node stack[len];
    #else
    btstack_node* stack = (btstack_node*)malloc(len*sizeof(btstack_node));
    #endif
    int top = 0;

    char *p,*start=s;
    char c[2];

    bool _dumpstack = false;

    s_foreach(re->re,p) {
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
                            #ifndef  _MSC_VER
                            char _buf[p1-p];
                            #else
                            char* _buf = (char*)malloc((p1-p)*sizeof(char));
                            #endif
                            _buf[p1-p-1] = '\0';
                            memcpy(_buf,p+1,p1-p-1);
                            int i = atoi(_buf);
                            if (i<0)
                                /* 这个偏移量的处理有点病态 */
                                p += (i-3);
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
            default:
                c[0] = *p;
_def:           if (!match(c,*s)) {
                    if (!top) _exitmatch();
                    p = stack[top].re+1;
                    s = stack[top--].s;
                    int _cout = 1;
                    if (*p=='(') {
                        while (*p!=')'||_cout!=0) {
                            p++;
                            if (*p=='(') _cout++;
                            else if (*p==')') _cout--;
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

void tre_freepattern(tre_pattern*re)
{
    free(re->re);
    free(re);
}

/* tinyre
 * 这是一个从头设计的正则引擎。
 *
 * 当前仅支持：. ? * + () [] \d \s \S
 *
 * 注意：[]后面暂时不能跟随 ? * +
 *
 */
int main(int argc,char* argv[])
{
    tre_pattern* ret = tre_compile("[\\da]23");
    if (ret) {
        //_p(ret->re);
        /*char*p;
        s_foreach(ret,p) {
            pc(*p);
        }
        putchar('\n');*/
        char *r = tre_match(ret,"a23");
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

