﻿/*
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

void tre_err(int err_code) {
    if (err_code == ERR_LEXER_UNBALANCED_PARENTHESIS) {
        printf_u8("input error: unbalanced parenthesis.\n");
    } else if (err_code == ERR_LEXER_UNEXPECTED_END_OF_PATTERN) {
        printf_u8("input error: unexpected end of pattern.\n");
    } else if (err_code == ERR_LEXER_UNKNOW_SPECIFIER) {
        printf_u8("input error: unknown specifier.\n");
    } else if (err_code == ERR_LEXER_BAD_GROUP_NAME) {
        printf_u8("input error: bad group name\n");
    } else if (err_code == ERR_LEXER_UNICODE_ESCAPE) {
        printf_u8("input error: unicode escape failed, requires 4 chars(\\u0000).\n");
    } else if (err_code == ERR_LEXER_UNICODE6_ESCAPE) {
        printf_u8("input error: unicode escape failed, requires 8 chars(\\u00000000).\n");
    } else if (err_code == ERR_LEXER_HEX_ESCAPE) {
        printf_u8("input error: hex escape failed, requires 2 chars(\\x00).\n");
    } else if (err_code == ERR_PARSER_REQUIRES_FIXED_WIDTH_PATTERN) {
        printf_u8("input error: look-behind requires fixed-width pattern\n");
    } else if (err_code == ERR_PARSER_BAD_CHARACTER_RANGE) {
        printf_u8("input error: bad character range\n");
    } else if (err_code == ERR_PARSER_NOTHING_TO_REPEAT) {
        printf_u8("input error: nothing to repeat\n");
    } else if (err_code == ERR_PARSER_IMPOSSIBLE_TOKEN) {
        printf_u8("input error: impossible token\n");
    } else {
        printf_u8("parsering falied!!!\n");
    }
}

tre_Pattern* tre_compile(char* s, int flag) {
    int ret;
    tre_Token *last_token;
    TokenInfo* tki;
    tre_Pattern* groups;

    ret = tre_lexer(s, &tki);

    if (ret >= 0) {
#ifdef TRE_DEBUG
        debug_token_print(tki->tokens, ret);
#endif
    }

    if (ret < 0) {
        tre_err(ret);
        return NULL;
    }

    groups = tre_parser(tki, &last_token);

    if (last_token == NULL || last_token < tki->tokens + ret) {
        token_info_free(tki);
        ret = tre_last_parser_error();
        tre_err(ret);
    } else {
        groups->flag = flag | tki->extra_flag;
        token_info_free(tki);
        return groups;
    }

    return NULL;
}

tre_Match* tre_match(tre_Pattern* tp, const char* str, int backtrack_limit)
{
    VMState* vms = vm_init(tp, str, backtrack_limit);
    tre_GroupResult* groups = vm_exec(vms);
    tre_Match* match = _new(tre_Match, 1);
    match->groupnum = vms->group_num;
    match->groups = groups;
    vm_free(vms);
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
    //pattern = tre_compile("c.", FLAG_DOTALL); // ca c\n
    //pattern = tre_compile("(?#123C."); // error: unbalanced parenthesis
    //pattern = tre_compile("(?i)C.", FLAG_NONE); // ca
    //pattern = tre_compile("(?s)c.", FLAG_NONE); // c\n
    //pattern = tre_compile("(?is)C.", FLAG_NONE); // c\n
    //pattern = tre_compile("a中+文*测?试\\醃b[1\\d2+\\][\\]\\a3]厑c\\de{1,5}\\", 0); // a中中中测试醃b+厑c1eeee\\ 
    //pattern = tre_compile("(?P<asdf>1)((?:a(c))(?:d))", FLAG_NONE); // 1acd
    //pattern = tre_compile("(?!a)+b", FLAG_NONE); // b
    //pattern = tre_compile("(?!(eb|ec))e", FLAG_NONE); // efe
    //pattern = tre_compile("(?=(c|(e)(b)))e", FLAG_NONE); // eb
    //pattern = tre_compile("e(?<=((?=(e*))e))e", FLAG_NONE); //ee
    //pattern = tre_compile("c(?<!b)e", FLAG_NONE); //ce
    //pattern = tre_compile("c(?<!bc{10})e", FLAG_NONE); //ce
    //pattern = tre_compile("[123,4]{3}", FLAG_NONE); //4,1
    //pattern = tre_compile("[----a-z123,4d-e]+", FLAG_NONE); //4,1-f
    //pattern = tre_compile("[\\S-z]+", FLAG_NONE); //a
    //pattern = tre_compile("[\\r-A]+", FLAG_IGNORECASE); //[ //not match
    //pattern = tre_compile("(.a+)[\\64]\\1", FLAG_NONE); // aaa@aaaa
    //pattern = tre_compile("\\u5B25A", FLAG_NONE); // 嬥A
    //pattern = tre_compile("\\U00005b25A", FLAG_NONE); // 嬥A
    //pattern = tre_compile("\\x61b", FLAG_NONE); // ab
    //pattern = tre_compile("\\uAaAA", FLAG_NONE); // success
    //pattern = tre_compile("\\uAaA", FLAG_NONE); // failed
    //pattern = tre_compile("\\x0a", FLAG_NONE); // success
    //pattern = tre_compile("\\xa", FLAG_NONE); // falied
    //pattern = tre_compile("\\U0000000B", FLAG_NONE); // success
    //pattern = tre_compile("a{}", FLAG_NONE); // a{}
    //pattern = tre_compile("a{0,", FLAG_NONE); // a{0,
    pattern = tre_compile("a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?aaaaaaaaaaaaaaa", FLAG_NONE); // slow, requires optimization
    if (pattern) {
        match = tre_match(pattern, "aaaaaaaaaaaaaaa", 50000000);

        if (match->groups) {
            putchar('\n');
            for (i = 0; i < match->groupnum; i++) {
                printf_u8("Group %2d: ", i);
                if (match->groups[i].name) printf_u8("(%s) ", match->groups[i].name);
                else printf_u8("(null) ");
                printf_u8("%d %d\n", match->groups[i].head, match->groups[i].tail);
                if (match->groups[i].head) {
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
