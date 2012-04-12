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

#define s_foreach(__s,__p) for (__p = __s;*__p;__p++)

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

bool dotall =false;

typedef struct tre_element {
    /* 该组所需要匹配的正则语句 */
    char* re;
    /* 是否允许拥有不确定性 ? * 后缀 
     * 当为 true 时进行回溯 (backtracking) */
    bool flag;
    /* 无穷匹配？ * 和 + 你们自重 */
    bool infinite;
    /* 字节点数目 */
    int childnum;
    /* 子节点们 */
    struct tre_element* childs;
} tre_element;

typedef struct tre_group {
    int start,len;
} tre_group;

typedef struct tre_Match {
    int groupnum;
    tre_group* group;
} tre_Match;

typedef struct tre_regex {
    //int groupnum;
    //tre_group* group;
    int emnum;
    tre_element* ems;
} tre_regex;

#define pc(x) printf("_%c",x)
#define _p(s) printf("%s\n",s)

/* 基本元素匹配
 * 所谓基本元素，就是其表达式只对应一个字符的元素，例如
 * [abcde] a . 这样的东西*/
static bool
match_elemlent(tre_element*em,const char*s)
{
    char c = *s;
    char*p = em->re;
    int re_len = strlen(em->re);

    if (re_len==1||(re_len==2&&em->re[0]=='\\')) {
        switch (*p) {
            case '.' : 
                if (dotall || *s!='\n') return true;break;
            case '\\': 
                {
                    switch (p[1]) {
                        case 's': if (isspace(*s)) return true;break;
                        case 'S': if (!(isspace(*s))) return true;break;
                        case 'd': if (isdigit(*s)) return true;break;
                        default : if (p[1]==*s) return true;
                    }
                    break;
                }
            default  : 
                if (p[0]==*s) return true;
        }
    } else {
        assert(p[0]=='[');
        char*p1;
        s_foreach(p+1,p1)
            if (*p1==*s) return true;
    }
    return false;
}

/* 第五原型：从 DFA 转向 NFA
 * 加入语句编译，并会将整个表达式分解成多个元素分别匹配
 * 这里会处理的东西： () 和 . 和 + 和 *
 * 
 * 注意：
 * 此版本无分组的支持，对括号只支持到一重
 */
tre_regex* compile(char*re)
{
    tre_regex *ret = malloc(sizeof(tre_regex));
    /* 直接申请内存长度为最大可能长度 */
    ret->ems = malloc(sizeof(tre_element)*strlen(re));
    tre_element* curem = ret->ems;

    char *p;
    s_foreach(re,p) {
        switch (*p) {
            case '?' : case '*' : case '+' :
                break;
            case '\\'  :
                /* 若\的下个字符不存在，报错 */
                assert(*(p+1));
                curem->childnum = 0;
                curem->re = strndup(p,2);
                /* 检查问号后缀和星号后缀 */
                if (*(p+2)=='+') {
                    curem->flag = false;
                    curem->infinite = false;
                    curem++;
                    curem->flag = true;
                    curem->infinite = true;
                    curem->childnum = 0;
                    curem->re = (curem-1)->re;
                } else  {
                    curem->flag = (*(p+2)=='?'||*(p+2)=='*') ? true : false;
                    curem->infinite = ((*(p+2)=='*') ? true : false);
                }
                /* 因为这个元素涉及两个字符，故向下传动 */
                p++;
                curem++;
                break;
            case '('   :
                {
                    char*p1;
                    int imatch=0;
                    int count=1;
                    char* last=NULL;
                    s_foreach (p+1,p1) {
                        if (*p1=='(') {
                            imatch++;
                            last=p1;
                            count++;
                        } else if (*p1==')') {
                            if (!imatch) break;
                            /* 若括号内内容不为空 */
                            if (p1!=last+1) {
                                char* __p = strndup(last,p1-last);
                                tre_regex *ret = compile(__p);
                                curem->childnum = ret->emnum;
                                curem->childs = ret->ems;
                            }
                            imatch--;
                        }
                    }
                    /* 若括号不匹配弹错 */
                    assert(!imatch);
                    /* 例行分组 */
                    if (p+1==p1) {
                        /* 若括号内容为空则跳过 */
                        p++;
                        break;
                    }
                    curem->re = strndup(p+1,p1-p-1);
                    if (*(p1+1)=='+') {
                        curem->flag = false;
                        curem->infinite = false;
                        curem++;
                        *curem = *(curem-1);
                        curem->flag = true;
                        curem->infinite = true;
                    } else {
                        curem->flag = (*(p1+1)=='?'||*(p1+1)=='*') ? true : false;
                        curem->infinite = (*(p1+1)=='*') ? true : false;
                    }
                    p = p1;
                    curem++;
                    break;
                }
            /*case '['   :
                {
                    break;
                }*/
            default    :
                curem->childnum = 0;
                curem->re = strndup(p,1);
                if (*(p+1)=='+') {
                    curem->flag = false;
                    curem->infinite = false;
                    curem++;
                    curem->childnum = 0;
                    curem->flag = true;
                    curem->infinite = true;
                    curem->re = (curem-1)->re;
                } else {
                    curem->flag = (*(p+1)=='?'||*(p+1)=='*') ? true : false;
                    curem->infinite = (*(p+1)=='*') ? true : false;
                }
                curem++;
        }
    }
    ret->emnum = curem - ret->ems;
    return ret;
}

/* 匹配假设：无分组 
 * 回溯点有两个：一个是当有可回溯记录，且新元素匹配失败 
 *               一个是当已经到了末尾，但是后面还有宽度大于0的元素
 * 回溯的顺序是从后到前 */
char*
tre_match(tre_regex *re,const char*s)
{
    const char* start=s;
    const char* btpoint = NULL;
    const char* end = s + strlen(s);
    tre_element *em;

    for (em = re->ems ; em<re->ems+re->emnum ; em++) {
        if (s==end) {
            /* 回溯点2 */
            tre_element *_em;
            for (_em = em+1;_em < re->ems+re->emnum;_em++) {
                if (!_em->flag) {
                    /* 找到宽度不为0的元素，尝试从这里回溯 */
                    tre_regex *_re = malloc(sizeof(tre_regex));
                    _re->emnum = re->ems+re->emnum - _em;
                    _re->ems = _em;
                    const char*_s;
                    const char* _ret;
                    for (_s = s-1 ; _s >= btpoint ; _s--)
                        if (_ret=tre_match(_re,_s)) {
                            s = _s + strlen(_ret);
                            free((void*)_ret);
                            goto end;
                        }
                    return NULL;
                }
            }
            break;
        }
        if (match_elemlent(em,s)) {
            if (!btpoint) btpoint = s;
            s++;
            if (em->infinite) em--;
        } else {
            if (em->flag) goto check;
            /* 回溯点 1 */
            tre_element *_em;
            for (_em = em;_em < re->ems+re->emnum;_em++) {
                if (!_em->flag) {
                    /* 找到宽度不为0的元素，尝试从这里回溯 */
                    tre_regex *_re = malloc(sizeof(tre_regex));
                    _re->emnum = re->ems+re->emnum - _em;
                    _re->ems = _em;
                    const char*_s;
                    const char* _ret;
                    for (_s = s-1 ; _s >= btpoint ; _s--) {
                        if (_ret=tre_match(_re,_s)) {
                            s = _s + strlen(_ret);
                            free((void*)_ret);
                            goto end;
                        }
                    }
                    return NULL;
                }
            }
            return NULL;
        }
check:;
        // TODO: 这里可以优化下，这句注释掉是为了回溯 b?bc 这样的句子
        //if ((!em->flag)&&(!btpoint)) btpoint = NULL;
    }
end:    return strndup(start,s-start);
}

//#define fs_foreach(__s,__p) for (__p = __s->s;__p<__s->s+__s->len;__p++)

// "a(b?c+)(d)d"  "abcccddd"

/* -- Tinyre 第五原型 --
 * 可识别的表达式：
 * . \s \S \d 和 任意字符
 * 可识别的辅助符号：
 * + 和 ? 和 * 和 ()      
 * -> 注意括号中暂时只能有单个字符，同时由于没有分组这东西，
 * 括号暂时只有象征意义。
 * */
int main(int argc,char* argv[])
{
    tre_regex* re = compile("1?12");
    assert(re);
    for (int i=0;i<re->emnum;i++) {
        printf("元素 %d,匹配内容 %s,允许回溯 %s,无穷：%s\n",i,re->ems[i].re,re->ems[i].flag?"是":"否",re->ems[i].infinite?"是":"否");
        for (int j=0;j<re->ems[i].childnum;j++)
            printf("-->  %d,匹配内容 %s,允许回溯 %s\n",i,re->ems[i].childs[j].re,re->ems[i].childs[j].flag?"是":"否");
    }
    char* ret;
    if (ret = tre_match(re,"123")) {
        printf("匹配成功！ %s\n",ret);
    } else {
        printf("匹配失败！\n");
    }
    return 0;
}

