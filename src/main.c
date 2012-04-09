/*
 * start : 2012-4-8 09:57
 * tiny re
 * 微型python风格正则表达式库
 */

#define _GNU_SOURCE 

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define continue_assert(c) switch(c){case'?':case'*':case'+':p++;continue;}
#define setstart() if (!start) start = s

/* 第一原型:没有[]没有{}没有() 
 * 有字符，有 . 有 ? 有 + 有 *
 * */
char* tre_match(const char*re,const char*s)
{
    const char *start = NULL;
    char*ret=NULL;
    bool b=false;

    for (const char*p=re;*p;) {
        switch(*p) {
            case '.': setstart();p++;break;
            case '*': setstart();if(*(p-1)!=*s){p++;continue;} break;
            case '+': setstart();
                      if (*(p-1)!=*s) {
                          if (!b) return NULL;
                          p++;
                          continue;
                      } b=true; break;
            case '?': setstart();if (*(p++-1)!=*s)continue;p++;break;
            case '$': if (!*s) {p++;s--;break;} return NULL;
            case '^': p++;continue;
            default : setstart();continue_assert(*(p+1));if(*p!=*s){return NULL;} p++;break;
        }
        s++;
    }
    ret = strndup(start,s-start);
    return ret;
}

int main(int argc,char* argv[])
{
    char* ret;
    if ( ret=tre_match("^db*c$","dbbbbbc"))
        printf("匹配成功！%s\n",ret);
    else printf("匹配失败！\n");
    return 0;
}

