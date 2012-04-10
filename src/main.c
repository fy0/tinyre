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
#include <assert.h>

typedef struct tre_group {
    char* name;
    int start,len;
    char* s;
    int groupnum;
} tre_group;

typedef struct tre_Match {
    int groupnum;
    tre_group* group;
} tre_Match;

//#define match_init(__m) memset(__m,0,sizeof(tre_Match))

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

/* 第三原型: 在第二原型基础上拥有了一个分组（即括号）的实现
 * 这里会处理的东西：
 * + 和 ? 和 * 和 ^ 和 $
 */
tre_Match* tre_match(const char* re,const char* s)
{
    int groupnum = 1;
    tre_group* groups;
    char _r[2] = {0,0};
    char _s;
    const char *p;
    const char *start=NULL;
    groups = malloc(sizeof(tre_group)*5);

    if (re[0]=='^') re++;
    s_foreach(re,p) {
        switch (*p) {
            case '('  :
                {
                    setstart();
                    const char *p1=p+1,*p2;
                    int count = 0;

                    s_foreach (p1,p2) {
                        if (*p2=='(') count++;
                        else if (*p2==')') {
                            // 此时 p2 指向右括号
                            if (!count) break;
                            count--;
                        }
                    }
                    tre_Match *ret=tre_match(strndup(p1,p2-p1),s);
                    for (int i=0;i<ret->groupnum;i++) {
                        if (groupnum>=5)
                            assert(realloc(groups,sizeof(tre_group)*groupnum));
                        groups[groupnum] = ret->group[i];
                        groupnum ++;
                    }
                    if (!ret) return 0;
                    p = p2;
                    s+=ret->group[0].len-1;
                    break;
                }
            case '?'  : 
                {
                    setstart();
                    if (p>re+1&&*(p-2)=='\\') {
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
                    if (!match(_r,_s)) return 0;
                    break;
                }
            case '$'  : 
                if (!*(s+1)) return 0;break;
            default   : 
                {
                    setstart();
                    if(*(p+1)=='?'||*(p+1)=='*') continue;
                    _r[0]=*p;
                    _s=*s;
                    if (!match(_r,_s))
                        return 0;
                }
        }
        s++;
    }
    if (!start) return NULL;
    tre_Match* result = malloc(sizeof(tre_Match));

    result->groupnum = groupnum;
    result->group = groups;
    result->group[0].name = NULL;
    result->group[0].start = 0;
    result->group[0].len = s-start;
    result->group[0].s = strndup(start,s-start);
    return result;
}

int main(int argc,char* argv[])
{
    tre_Match* ret;
    if (ret=tre_match("a(b?c+)(d)d","abcccddd")) 
        printf("匹配成功！ %s\n",ret->group[0].s);
    else printf("匹配失败！\n");
    return 0;
}

