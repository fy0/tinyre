/*
 * start : 2012-4-8 09:57
 * update: 2015-12-10 v0.9.0
 *
 * tinyre
 * fy, 2012-2015
 *
 */

#include "tutils.h"
#include "tlexer.h"
#include "tparser.h"
#include "tvm.h"
#include "tdebug.h"

void tre_err(const char* s) {
    printf_u8("%s\n", s);
    exit(-1);
}

tre_Pattern* tre_compile(char* s, int flag) {
    int ret;
    int extra_flag;
    tre_Token *tk, *tokens = NULL;

    ret = tre_lexer(s, &tokens, &extra_flag);
    
    if (ret >= 0) {
#ifdef _DEBUG
        debug_token_print(tokens, ret);
#endif
    } else if (ret == -1) {
        printf_u8("input error: characters inside {...} invalid.\n");
        exit(-1);
    }

    tk = tokens;

    tre_Pattern* groups = tre_parser(tokens, &tk);
    free(tokens);

    if (tk == NULL || tk < tokens + ret) {
        printf_u8("parsering falied!!!\n");
    } else {
        groups->flag = flag | extra_flag;
        return groups;
    }

    return NULL;
}

tre_Match* tre_match(tre_Pattern* tp, const char* str)
{
    VMState* vms = vm_init(tp, str);
    tre_group* groups = vm_exec(vms);
    tre_Match* match = _new(tre_Match, 1);
    match->groupnum = vms->group_num;
    match->groups = groups;
    return match;
}

#ifdef DEMO

int main(int argc,char* argv[])
{
    int i;
    tre_Pattern* pattern;
    tre_Match* match = NULL;
    platform_init();

    //tre_compile("a中+文*测?试\\醃b[1\\d2+\\][\\]\\a3]厑c\\de{1,5}\\", 0);
    //tre_compile("abc\\a\\d([123](c)d)jk123", 0);
    //MatchGroup* groups = tre_compile("ab?c+\\a*\\d([123]+(c)?d)jk123", 0);
    //tre_Pattern* groups = tre_compile("(a([acd\\s123]))", 0);
    //tre_Pattern* groups = tre_compile("^(a).?([acd\\s123])", 0);
    //pattern = tre_compile("^(bb)*?a{1{,}c+?", 0);
    //pattern = tre_compile("^a$b?", 0);
    //pattern = tre_compile("(a|)+", 0); //a
    //pattern = tre_compile("(b|)+?b", 0); //a
    //pattern = tre_compile("(a|b|c)", 0); //c
    //pattern = tre_compile("c.", FLAG_DOTALL); // ca c\\n
    pattern = tre_compile("C.", FLAG_IGNORECASE);
    if (pattern) {
        match = tre_match(pattern, "ca");

        putchar('\n');
        if (match->groups) {
            for (i = 0; i < match->groupnum; i++) {
                printf_u8("Group %2d: ", i);
                //printf_u8("%d %d\n", match->groups[i].head, match->groups[i].tail);
                if (match->groups[i].head && match->groups[i].tail) {
                    debug_printstr(match->groups[i].head, match->groups[i].tail);
                } else {
                    printf_u8("match failed.");
                }
                printf_u8("\n");
            }
        }
    }

    return 0;
}

#endif
