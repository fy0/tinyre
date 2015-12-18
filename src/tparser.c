
#include "tutils.h"
#include "tlexer.h"
#include "tparser.h"
#include "tvm.h"
#include "tdebug.h"

#define paser_accept(__stat) if (!((__stat))) return NULL;
#define check_token(tk) if (tk == NULL || tk->token == 0) return NULL;

ParserMatchGroup* m_start = NULL;
ParserMatchGroup* m_cur = NULL;
tre_TokenGroupName* tk_group_names;

tre_Token* parser_char_set(tre_Token* tk);
tre_Token* parser_char(tre_Token* tk);
tre_Token* parser_block(tre_Token* tk);
tre_Token* parser_group(tre_Token* tk);

tre_Token* parser_single_char(tre_Token* tk) {
    if (tk->token == TK_CHAR || tk->token == TK_SPE_CHAR) {
        // CODE GENERATE
        // CMP/CMP_SPE CODE
        m_cur->codes->ins = tk->token == TK_CHAR ? ins_cmp : ins_cmp_spe;
        m_cur->codes->data = _new(int, 1);
        m_cur->codes->len = sizeof(int) * 1;
        *(int*)m_cur->codes->data = tk->code;
        m_cur->codes->next = _new(INS_List, 1);
        m_cur->codes = m_cur->codes->next;
        m_cur->codes->next = NULL;
        // END
        return tk + 1;
    }
    return NULL;
}

tre_Token* parser_char_set(tre_Token* tk) {
    int num;
    int* data;
    bool is_ncmp;
    tre_Token* start;

    TRE_DEBUG_PRINT("CHAR_SET\n");

    paser_accept(tk->token == '[');
    is_ncmp = (tk->code == 1) ? true : false;
    tk++;
    check_token(tk);
    start = tk;
    paser_accept(tk->token == TK_CHAR || tk->token == TK_SPE_CHAR);
    tk++;
    check_token(tk);
    while ((tk->token == TK_CHAR || tk->token == TK_SPE_CHAR)) {
        tk++;
        check_token(tk);
    }
    paser_accept(tk->token == ']');

    // CODE GENERATE
    // CMP_MULTI NUM [TYPE1 CODE1], [TYPE2, CODE2], ...
    num = tk - start;
    m_cur->codes->ins = is_ncmp ? ins_ncmp_multi : ins_cmp_multi;
    m_cur->codes->data = _new(int, 2 * num + 1);
    m_cur->codes->len = sizeof(int) * (2 * num + 1);
    // END

    data = (int*)m_cur->codes->data;
    *(data++) = num;

    for (; start != tk; start++) {
        *data = start->token;
        *(data + 1) = start->code;
        data += 2;
    }

    m_cur->codes->next = _new(INS_List, 1);
    m_cur->codes = m_cur->codes->next;
    m_cur->codes->next = NULL;
    return tk + 1;
}

tre_Token* parser_char(tre_Token* tk) {
    TRE_DEBUG_PRINT("CHAR\n");

    tre_Token* ret = parser_single_char(tk);
    if (ret) return ret;
    return parser_char_set(tk);
}

tre_Token* parser_other_tokens(tre_Token* tk) {
    if (tk->token == '^' || tk->token == '$') {
        // CODE GENERATE
        // MATCH_START/MATCH_END
        m_cur->codes->ins = tk->token == '^' ? ins_match_start : ins_match_end;
        m_cur->codes->len = 0;
        m_cur->codes->next = _new(INS_List, 1);
        m_cur->codes = m_cur->codes->next;
        m_cur->codes->next = NULL;
        // END
        return tk + 1;
    }
    return NULL;
}

tre_Token* parser_or(tre_Token* tk) {
    // try to catch a |
    OR_List *or, *or2;
    paser_accept(tk->token == '|');
    or = _new(OR_List, 1);
    or->codes = m_cur->codes;
    or->next = NULL;

    if (m_cur->or_list) {
        or2 = m_cur->or_list;
        while (or2->next) {
            or2 = or2->next;
        }
        or2->next = or;
    } else {
        m_cur->or_list = or;
    }
    m_cur->or_num++;

    // CODE GENERATE
    // GROUP_END -1
    m_cur->codes->ins = ins_group_end;
    m_cur->codes->data = _new(int, 1);
    m_cur->codes->len = sizeof(int);
    *(int*)m_cur->codes->data = -1;
    m_cur->codes->next = _new(INS_List, 1);
    m_cur->codes = m_cur->codes->next;
    m_cur->codes->next = NULL;
    // END
    return tk + 1;
}

tre_Token* parser_block(tre_Token* tk) {
    tre_Token *ret, *ret2;
    INS_List* last_ins;

    TRE_DEBUG_PRINT("BLOCK\n");

    check_token(tk);
    last_ins = m_cur->codes;

    ret = parser_char(tk);
    if (!ret) ret = parser_group(tk);
    if (!ret) {
        ret2 = parser_or(tk);
        if (ret2) return ret2;
    }

    if (ret) {
        bool need_checkpoint = false, greed = true;
        int llimit, rlimit;

        tk = ret;

        if (tk->token == '?') {
            ret++;
            llimit = 0;
            rlimit = 1;
            need_checkpoint = true;
        } else if (tk->token == '+') {
            ret++;
            llimit = 1;
            rlimit = -1;
            need_checkpoint = true;
        } else if (tk->token == '*') {
            ret++;
            llimit = 0;
            rlimit = -1;
            need_checkpoint = true;
        } else if (tk->token == '{') {
            if ((tk + 1)->token == '}') {
                ret += 2;
                llimit = tk->code;
                rlimit = (tk + 1)->code;
                need_checkpoint = true;
            } else {
                return NULL;
            }
        }

        if (need_checkpoint) {
            if (ret->token == '?') {
                ret ++;
                greed = false;
            }
        }

        // CODE GENERATE
        // CHECK_POINT LLIMIT RLIMIT
        if (need_checkpoint) {
            // 将上一条指令（必然是一条匹配指令复制一遍）
            memcpy(m_cur->codes, last_ins, sizeof(INS_List));
            // 为这条指令创建新的后继节点
            m_cur->codes->next = _new(INS_List, 1);
            m_cur->codes = m_cur->codes->next;
            m_cur->codes->next = NULL;

            last_ins->ins = greed ? ins_check_point : ins_check_point_no_greed;
            last_ins->data = _new(int, 2);
            last_ins->len = sizeof(int) * 2;
            *(int*)last_ins->data = llimit;
            *((int*)last_ins->data + 1) = rlimit;
        }
        // END
    } else {
        ret = parser_other_tokens(tk);
    }
    
    return ret;
}

tre_Token* parser_group(tre_Token* tk) {
    int gindex = 1;
    char* name = NULL;
    tre_Token* ret;
    ParserMatchGroup* last_group;

    TRE_DEBUG_PRINT("GROUP\n");

    paser_accept(tk->token == '(');
    if (tk_group_names && tk_group_names->tk == tk++) name = tk_group_names->name;
    check_token(tk);

    last_group = m_cur;
    // 前进至最后一节
    m_cur = m_start;
    while (m_cur->next) {
        m_cur = m_cur->next;
        gindex++;
    }
    // 创建新节点
    m_cur->next = _new(ParserMatchGroup, 1);
    m_cur->next->codes = m_cur->next->codes_start = _new(INS_List, 1);
    m_cur = m_cur->next;
    m_cur->next = NULL;
    m_cur->or_num = 0;
    m_cur->or_list = NULL;
    m_cur->name = name;
    tk_group_names = tk_group_names->next;

    ret = tk;
    while ((ret = parser_block(ret))) tk = ret;

    paser_accept(tk->token == ')');

    // CODE GENERATE
    // CMP_GROUP INDEX
    last_group->codes->ins = ins_cmp_group;
    last_group->codes->data = _new(int, 1);
    last_group->codes->len = sizeof(int) * 1;
    *(int*)last_group->codes->data = gindex;
    last_group->codes->next = _new(INS_List, 1);
    last_group->codes = last_group->codes->next;
    last_group->codes->next = NULL;
    // END

    m_cur = last_group;
    return tk+1;
}

tre_Token* parser_blocks(tre_Token* tk) {
    tre_Token* ret;
    //paser_accept(tk = parser_block(tk));
    ret = tk;
    while ((ret = parser_block(ret))) tk = ret;
    return tk;
}

tre_Pattern* tre_parser(tre_Token* tk, tre_TokenGroupName* _tk_group_names, tre_Token** last_token) {
    tre_Token* tokens;
    tre_Pattern* ret;

    tk_group_names = _tk_group_names;

    m_cur = m_start = _new(ParserMatchGroup, 1);
    m_start->codes = m_start->codes_start = _new(INS_List, 1);
    m_start->codes->next = NULL;
    m_cur->next = NULL;
    m_cur->or_num = 0;
    m_cur->or_list = NULL;
    m_cur->name = NULL;

    tokens = parser_blocks(tk);
    *last_token = tokens;

    if (tokens) {
#ifdef _DEBUG
        debug_ins_list_print(m_start);
#endif
        ret = compact_group(m_start);
        return ret;
    }
    return NULL;
}


tre_Pattern* compact_group(ParserMatchGroup* parser_groups) {
    int* data;
    int gnum = 0;
    MatchGroup* g;
    MatchGroup* groups;
    ParserMatchGroup *pg, *pg_tmp;
    INS_List *code, *code_tmp;
    OR_List *or_lst;
    tre_Pattern* ret = _new(tre_Pattern, 1);

    for (pg = parser_groups; pg; pg = pg->next) gnum++;
    groups = _new(MatchGroup, gnum);

    gnum = 0;
    for (pg = parser_groups; pg; ) {
        int code_lens = pg->or_num * sizeof(int) * 2;
        or_lst = pg->or_list;
        g = groups + gnum;

        for (code = pg->codes_start; code->next; code = code->next) {
            code_lens += (code->len + sizeof(int));
        }

        // sizeof(int)*2 is space for group_end
        g->codes = malloc(code_lens + sizeof(int) * 2);
        g->name = pg->name;

        if (pg->or_num) {
            code_lens = pg->or_num * 2 * sizeof(int); // recount

            data = g->codes + (pg->or_num-1) * 2;
            for (code = pg->codes_start; true; code = code->next) {
                while (or_lst && or_lst->codes == code) {
                    *data++ = ins_save_snap;
                    *data = code_lens;
                    data -= 3;
                    or_lst = or_lst->next;
                }
                if (!code->next) break;
                code_lens += (code->len + sizeof(int));
            }
        }

        data = g->codes + pg->or_num * 2;

        for (code = pg->codes_start; code->next; ) {
            *data++ = code->ins;

            if (code->len) {
                memcpy(data, code->data, code->len);
                data += (code->len / sizeof(int));
                free(code->data);
            }

            code_tmp = code;
            code = code->next;
            free(code_tmp);
        }
        // the final one
        free(code);

        *data = ins_group_end;
        *(data + 1) = gnum;

        gnum++;

        pg_tmp = pg;
        pg = pg->next;
        free(pg_tmp);
    }

    ret->groups = groups;
    ret->num = gnum;

    // TODO: why crash? i need valgrind.
    //free(parser_groups);

    return ret;
}
