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
    } else if (err_code == ERR_LEXER_BAD_GROUP_NAME_IN_BACKREF) {
        printf_u8("input error: bad group name in backref\n");
    } else if (err_code == ERR_LEXER_INVALID_GROUP_NAME_OR_INDEX) {
        printf_u8("input error: invalid group name or index\n");
    } else if (err_code == ERR_LEXER_REDEFINITION_OF_GROUP_NAME) {
        printf_u8("input error: redefinition of group name\n");
    } else if (err_code == ERR_PARSER_REQUIRES_FIXED_WIDTH_PATTERN) {
        printf_u8("input error: look-behind requires fixed-width pattern\n");
    } else if (err_code == ERR_PARSER_BAD_CHARACTER_RANGE) {
        printf_u8("input error: bad character range\n");
    } else if (err_code == ERR_PARSER_NOTHING_TO_REPEAT) {
        printf_u8("input error: nothing to repeat\n");
    } else if (err_code == ERR_PARSER_IMPOSSIBLE_TOKEN) {
        printf_u8("input error: impossible token\n");
    } else if (err_code == ERR_PARSER_UNKNOWN_GROUP_NAME) {
        printf_u8("input error: unknow group name\n");
    } else if (err_code == ERR_PARSER_INVALID_GROUP_INDEX) {
        printf_u8("input error: invalid group index in conditional backref\n");
    } else {
        printf_u8("parsering falied!!!\n");
    }
}

tre_Pattern* tre_compile(char* s, int flag, int* err_code) {
    int ret;
    tre_Pattern* groups;
    tre_Lexer* lexer;

    int len;
    uint32_t* buf = utf8_to_ucs4_str(s, &len);

    lexer = tre_lexer_new(buf, len);

//#define TRE_DEBUG_LEXER
#ifdef TRE_DEBUG_LEXER
    debug_token_print(lexer);
    return 0;
#endif

    groups = tre_parser(lexer, &ret);

    if (groups == NULL) {
        *err_code = ret;
    } else {
        groups->flag = flag | lexer->extra_flag;
    }

    //token_info_free(tki);
    return groups;
}

tre_Match* tre_match(tre_Pattern* tp, const char* str, int backtrack_limit)
{
    VMState* vms = vm_init(tp, str, backtrack_limit);
    tre_GroupResult* groups = vm_exec(vms);
    tre_Match* match = _new(tre_Match, 1);
    match->groupnum = vms->group_num;
    match->groups = groups;
    match->str = vms->input_str;
    vm_free(vms);
    return match;
}

void tre_pattern_free(tre_Pattern *ptn) {
    int i;

    for (i = 0; i < ptn->num_all; i++) {
        free(ptn->groups[i].codes);
        free(ptn->groups[i].name);
    }

    free(ptn->groups);
    free(ptn);
}

void tre_match_free(tre_Match *m) {
    int i;
    if (m->groups) {
        for (i = 0; i < m->groupnum; i++) {
            free(m->groups[i].name);
        }
    }
    free(m->str);
    free(m->groups);
    free(m);
}
