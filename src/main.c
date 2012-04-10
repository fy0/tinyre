/*
 * start : 2012-4-8 09:57
 * tiny re
 * 微型python风格正则表达式库
 */

#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

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

#define continue_assert(c) switch(c){case'?':case'*':case'+':p++;continue;}
/* 作用:如果此时还未标记字符串被匹配的开始处，那么标记之 */
#define setstart() if (!start) start = s

#define pc(x) printf("_%c",x)
#define _p(s) printf("%s\n",s)

//#define fs_foreach(__s,__p) for (__p = __s->s;__p<__s->s+__s->len;__p++)

bool dotall = false;

/* 这里会处理的字符: 
 * 常规字符 和 . 和 \\ 和 \. 和 \* 和 \( 和 \[ 和 \{ 
 * 以及 \s \S \d 
 * */ 
static bool
match(char re[2],char c)
{
    switch (re[0]) {
        case '.' : if (dotall || c!='\n')return true;break;
        case '\\': {
                       switch (re[1]) {
                           case 's': if (isspace(c)) return true;break;
                           case 'S': if (!(isspace(c))) return true;break;
                           case 'd': if (isdigit(c)) return true;break;
                           default : if (re[1]==c) return true;
                       }
                       break;
                   }
        default  : if (re[0]==c) return true;
    };
    return false;
}

#define s_foreach(__s,__p) for (__p = __s;*__p;__p++)

/* 第二原型:对原函数进行拆分，一个专门进行分组，一个进行单字匹配
 * 这里会处理的东西：
 * + 和 ? 和 * 和 ^ 和 $
 */
char *tre_match(const char* re,const char* s)
{
    char _r[2] = {0,0};
    char _s;
    const char *p;
    const char *start=NULL;

    if (re[0]=='^') re++;
    s_foreach(re,p) {
        switch (*p) {
            case '?'  : 
                {
                    setstart();
                    if (p>re+1||*(p-2)=='\\') {
                        _r[0]='\\';
                        _r[1]=*(p-1);
                    } else _r[0]=*(p-1);
                    _s=*s;
                    if (!match(_r,_s)) s--;
                    break;
                }            
            case '*'  : case '+'  : 
                {
                    setstart();
                    if (p>re+1||*(p-2)=='\\') {
                        _r[0]='\\';
                        _r[1]=*(p-1);
                    } else _r[0]=*(p-1);
                    _s=*s;
                    if (!match(_r,_s)) {
                        s--;
                        break;
                    }
                    p--;
                    break;
                }
            case '\\' : 
                {
                    setstart();
                    p++;
                    _r[0]='\\';
                    _r[1]=*p;
                    _s=*s;
                    if (!match(_r,_s)) return NULL;
                    break;
                }
            case '$'  : 
                if (!*(s+1)) return NULL;break;
            default   : 
                {
                    setstart();
                    if(*(p+1)=='?'||*(p+1)=='*') continue;
                    _r[0]=*p;
                    _s=*s;
                    if (!match(_r,_s))
                        return NULL;
                }
        }
        s++;
    }
    if (!start) return NULL;
    return strndup(start,s-start);
}

int main(int argc,char* argv[])
{
    char *ret;
    if (ret=tre_match("ab*c","abbbcd")) 
        printf("匹配成功！ %s\n",ret);
    else printf("匹配失败！\n");
    return 0;
}

