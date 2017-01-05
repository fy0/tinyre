
#include "tutils.h"
#include "tlexer.h"
#include "tparser.h"
#include "tvm.h"
#include "tdebug.h"

#define paser_accept(__stat) if (!((__stat))) return false;
#define check_token(tk) if (tk->value == TK_END) return false;

bool parse_single_char(tre_Parser *ps);
bool parse_char_set(tre_Parser *ps);
bool parse_char(tre_Parser *ps);
bool parse_group(tre_Parser *ps);
bool parse_block(tre_Parser *ps);
bool parse_blocks(tre_Parser *ps);


bool parse_single_char(tre_Parser *ps) {
    tre_Token *tk = &(ps->lex->token);

    if (tk->value == TK_CHAR || tk->value == TK_CHAR_SPE) {
        // CODE GENERATE
        // CMP/CMP_SPE CODE
        ps->m_cur->codes->ins = tk->value == TK_CHAR ? INS_CMP : INS_CMP_SPE;
        ps->m_cur->codes->data = tre_new(uint32_t, 1);
        ps->m_cur->codes->len = 1;
        *ps->m_cur->codes->data = tk->extra.code;
        ps->m_cur->codes->next = tre_new(INS_List, 1);
        ps->m_cur->codes = ps->m_cur->codes->next;
        ps->m_cur->codes->next = NULL;
        // END
        tre_lexer_next(ps->lex);
        return true;
    }
    return false;
}


bool parser_char_set(tre_Parser *ps) {
    tre_Token *tk = &(ps->lex->token);

    bool is_ncmp;
    uint32_t *data, *data_start;
    int tmp, num = 0, size_all = 4;

    TRE_DEBUG_PRINT("CHAR_SET\n");

    paser_accept(tk->value == '[');
    is_ncmp = (tk->extra.code == 1) ? true : false;
    tre_lexer_next(ps->lex);
    check_token(tk);

    // CODE GENERATE
    // CMP_MULTI NUM [TYPE1 CODE1 NOOP], [TYPE2 CODE2 NOOP], [TYPE3 RANGE1 RANGE2] ...
    ps->m_cur->codes->ins = is_ncmp ? INS_NCMP_MULTI : INS_CMP_MULTI;
    data_start = tre_new(uint32_t, 3 * size_all + 1);

    data = (uint32_t*)data_start+1;

    while (tk->value == TK_CHAR || tk->value == TK_CHAR_SPE || tk->value == '-') {
        check_token(tk);

        if (num + 1 >= size_all) {
            size_all *= 2;
            tmp = data - data_start;
            data_start = realloc(data_start, (3 * size_all + 1)*sizeof(uint32_t));
            data = data_start + tmp;
        }

        (*data) = tk->value;
        *(data + 1) = tk->extra.code;

        if (tk->value == '-') {
            *(data + 2) = tk->extra.code2;
        }

        tre_lexer_next(ps->lex);
        data += 3;
        num++;
    }

    *data = num;
    ps->m_cur->codes->data = data_start;
    ps->m_cur->codes->len = (3 * num + 1);
    // END

    paser_accept(tk->value == ']');
    tre_lexer_next(ps->lex);

    data = (uint32_t*)ps->m_cur->codes->data;
    *(data++) = num;

    ps->m_cur->codes->next = tre_new(INS_List, 1);
    ps->m_cur->codes = ps->m_cur->codes->next;
    ps->m_cur->codes->next = NULL;
    return true;
}

bool parse_char(tre_Parser *ps) {
    TRE_DEBUG_PRINT("CHAR\n");

    bool ret = parse_single_char(ps);
    if (!ret) ret = parser_char_set(ps);
    if (ret && ps->is_count_width) ps->match_width += 1;
    return ret;
}

bool parse_other_tokens(tre_Parser *ps) {
    tre_Token *tk = &(ps->lex->token);

    if (tk->value == '^' || tk->value == '$') {
        // CODE GENERATE
        // MATCH_START/MATCH_END
        ps->m_cur->codes->ins = tk->value == '^' ? INS_MATCH_START : INS_MATCH_END;
        ps->m_cur->codes->len = 0;
        ps->m_cur->codes->next = tre_new(INS_List, 1);
        ps->m_cur->codes = ps->m_cur->codes->next;
        ps->m_cur->codes->next = NULL;
        // END
        tre_lexer_next(ps->lex);
        return true;
    }
    return false;
}

bool parse_or(tre_Parser *ps) {
    // try to catch |
    tre_Token *tk = &(ps->lex->token);
    OR_List *or_list, *or2_list;

    paser_accept(tk->value == '|');
    tre_lexer_next(ps->lex);

    // code for conditional backref
    if (ps->m_cur->group_type >= GT_BACKREF_CONDITIONAL_INDEX) {
        if (ps->m_cur->or_num) {
            ps->error_code = ERR_PARSER_CONDITIONAL_BACKREF;
            return false;
        }
    }
    // end

    or_list = tre_new(OR_List, 1);
    or_list->codes = ps->m_cur->codes;
    or_list->next = NULL;

    if (ps->m_cur->or_list) {
        or2_list = ps->m_cur->or_list;
        while (or2_list->next) {
            or2_list = or2_list->next;
        }
        or2_list->next = or_list;
    } else {
        ps->m_cur->or_list = or_list;
    }

    // CODE GENERATE
    // GROUP_END -1
    ps->m_cur->codes->ins = INS_GROUP_END;
    ps->m_cur->codes->data = tre_new(uint32_t, 1);
    ps->m_cur->codes->len = 1;
    *ps->m_cur->codes->data = -1;
    ps->m_cur->codes->next = tre_new(INS_List, 1);
    ps->m_cur->codes = ps->m_cur->codes->next;
    ps->m_cur->codes->next = NULL;
    ps->m_cur->or_num++;
    // END
    return true;
}

bool parse_back_ref(tre_Parser *ps) {
    tre_Token *tk = &(ps->lex->token);

    if (tk->value == TK_BACK_REF) {
        // CODE GENERATE
        // CMP_BACKREF INDEX
        ps->m_cur->codes->ins = INS_CMP_BACKREF;
        ps->m_cur->codes->data = tre_new(uint32_t, 1);
        ps->m_cur->codes->len = 1;
        *ps->m_cur->codes->data = tk->extra.index;
        ps->m_cur->codes->next = tre_new(INS_List, 1);
        ps->m_cur->codes = ps->m_cur->codes->next;
        ps->m_cur->codes->next = NULL;
        // END
        tre_lexer_next(ps->lex);
        return true;
    }
    return false;
}

bool parse_block(tre_Parser* ps) {
    bool ret;
    INS_List* last_ins;
    tre_Token *tk = &(ps->lex->token);

    TRE_DEBUG_PRINT("BLOCK\n");

    last_ins = ps->m_cur->codes;

    switch (tk->value) {
        case TK_CHAR: case TK_CHAR_SPE: case '[':
            ret = parse_char(ps);
            break;
        case TK_BACK_REF:
            ret = parse_back_ref(ps);
            break;
        case '|':
            ret = parse_or(ps);
            if (ret) return ret;
            if (ps->error_code) return false;
            break;
        case '(':
            ret = parse_group(ps);
            break;
        default:
            ret = false;
            break;
    }

    if (ret) {
        bool need_checkpoint = false, greed = true;
        int llimit, rlimit;

        if (tk->value == '?') {
            llimit = 0;
            rlimit = 1;
            need_checkpoint = true;
            tre_lexer_next(ps->lex);
        } else if (tk->value == '+') {
            llimit = 1;
            rlimit = -1;
            need_checkpoint = true;
            tre_lexer_next(ps->lex);
        } else if (tk->value == '*') {
            llimit = 0;
            rlimit = -1;
            need_checkpoint = true;
            tre_lexer_next(ps->lex);
        } else if (tk->value == '{') {
            llimit = tk->extra.code;
            rlimit = tk->extra.code2;
            need_checkpoint = true;
            tre_lexer_next(ps->lex);
        }

        if (need_checkpoint) {
            if (tk->value == '?') {
                // ?? +? *?
                tre_lexer_next(ps->lex);
                greed = false;
            }

            if (ps->is_count_width) {
                if (llimit != rlimit) {
                    ps->error_code = ERR_PARSER_REQUIRES_FIXED_WIDTH_PATTERN;
                    return false;
                }
                if (llimit > 0) {
                    ps->match_width += llimit - 1;
                }
            }
        }

        // CODE GENERATE
        // CHECK_POINT LLIMIT RLIMIT
        if (need_checkpoint) {
            // 将上一条指令（必然是一条匹配指令复制一遍）
            memcpy(ps->m_cur->codes, last_ins, sizeof(INS_List));
            // 为这条指令创建新的后继节点
            ps->m_cur->codes->next = tre_new(INS_List, 1);
            ps->m_cur->codes = ps->m_cur->codes->next;
            ps->m_cur->codes->next = NULL;

            last_ins->ins = greed ? INS_CHECK_POINT : INS_CHECK_POINT_NO_GREED;
            last_ins->data = tre_new(uint32_t, 2);
            last_ins->len = 2;
            *last_ins->data = llimit;
            *(last_ins->data + 1) = rlimit;
        }
        // END
    } else {
        ret = parse_other_tokens(ps);
    }

    if (!ret) {
        if (!ps->error_code) ps->error_code = ERR_PARSER_NOTHING_TO_REPEAT;
        return false;
    }
    
    TRE_DEBUG_PRINT("BLOCK_END\n");
    return true;
}


bool parse_group(tre_Parser* ps) {
    int gindex;
    int group_type;
    int group_extra;
    tre_Token *tk = &(ps->lex->token);
    ParserMatchGroup* last_group;
    uint32_t *gname = tk->extra.group_name;
    int gname_len = tk->extra.group_name_len;

    int match_width_record;
    bool is_count_width_record = ps->is_count_width;

    TRE_DEBUG_PRINT("GROUP\n");

    paser_accept(tk->value == '(');
    group_type = tk->extra.group_type;

    // code for back reference (?P=), backref group is not real group
    if (group_type == GT_BACKREF) {
        int i = 1;
        for (ParserMatchGroup* pg = ps->m_start->next; pg; pg = pg->next) {
            if (pg->name && (memcmp(tk->extra.group_name, pg->name, tk->extra.group_name_len*sizeof(uint32_t)) == 0)) {
                ps->m_cur->codes->ins = INS_CMP_BACKREF;
                ps->m_cur->codes->data = tre_new(uint32_t, 1);
                ps->m_cur->codes->len = 1;
                *ps->m_cur->codes->data = i;
                ps->m_cur->codes->next = tre_new(INS_List, 1);
                ps->m_cur->codes = ps->m_cur->codes->next;
                ps->m_cur->codes->next = NULL;
                free(tk->extra.group_name);

                tre_lexer_next(ps->lex); // skip (
                tre_lexer_next(ps->lex); // skip ）
                return true;
            }
            i++;
        }
        ps->error_code = ERR_PARSER_UNKNOWN_GROUP_NAME;
        return false;
    }
    // end

    if (group_type == GT_NORMAL) {
        gindex = 1; // 注意 gindex 不等于 avaliable_group 这里我犯过一次错误
    } else {
        gindex = ps->lex->max_normal_group_num;
        // used by condition backref (index)
        group_extra = tk->extra.index;
    }

    // code for (?<=...) (?<!...)
    if (group_type == GT_IF_PRECEDED_BY || group_type == GT_IF_NOT_PRECEDED_BY) {
        if (!ps->is_count_width) ps->is_count_width = true;
        match_width_record = ps->match_width;
    }
    if (ps->is_count_width && (group_type == GT_IF_MATCH || group_type == GT_IF_NOT_MATCH)) {
        ps->is_count_width = false;
    }
    // end

    tre_lexer_next(ps->lex);
    check_token(tk);

    // 前进至最后
    last_group = ps->m_cur;
    ps->m_cur = ps->m_start;

    while (ps->m_cur->next) {
        ps->m_cur = ps->m_cur->next;
        if (group_type == GT_NORMAL && ps->m_cur->group_type == GT_NORMAL) gindex++;
        if (group_type != GT_NORMAL && ps->m_cur->group_type != GT_NORMAL) gindex++;
    }

    // 创建新节点
    ps->m_cur->next = tre_new(ParserMatchGroup, 1);
    ps->m_cur->next->codes = ps->m_cur->next->codes_start = tre_new(INS_List, 1);
    ps->m_cur = ps->m_cur->next;
    ps->m_cur->next = NULL;
    ps->m_cur->or_num = 0;
    ps->m_cur->or_list = NULL;
    ps->m_cur->name = (group_type == GT_NORMAL) ? gname : NULL;
    ps->m_cur->name_len = gname_len;
    ps->m_cur->group_type = group_type;
    ps->m_cur->codes->next = NULL;

    // code for conditional backref
    if (group_type == GT_BACKREF_CONDITIONAL_INDEX) {
        ps->m_cur->group_extra = group_extra;
        ps->m_cur->group_type = GT_BACKREF_CONDITIONAL_INDEX;
    }
    if (group_type == GT_BACKREF_CONDITIONAL_GROUPNAME) {
        int i = 1;
        bool ok = false;
        for (ParserMatchGroup* pg = ps->m_start->next; pg; pg = pg->next) {
            if (pg->group_type == GT_NORMAL && pg->name && (memcmp(gname, pg->name, gname_len*sizeof(uint32_t)) == 0)) {
                ps->m_cur->group_extra = i;
                ps->m_cur->group_type = GT_BACKREF_CONDITIONAL_INDEX;
                ok = true;
                free(tk->extra.group_name);
                break;
            }
            i++;
        }
        if (!ok) {
            ps->error_code = ERR_PARSER_UNKNOWN_GROUP_NAME;
            return false;
        }
    }
    // end

    while (ps->lex->token.value != TK_END && ps->lex->token.value != ')') {
        if (!parse_block(ps)) return false;
    }

    if (ps->lex->token.value == TK_END) {
        ps->error_code = ERR_LEXER_UNBALANCED_PARENTHESIS;
        return false;
    }

    paser_accept(tk->value == ')');

    // code for (?<=...) (?<!...)
    if (group_type == GT_IF_PRECEDED_BY || group_type == GT_IF_NOT_PRECEDED_BY) {
        ps->m_cur->group_extra = ps->match_width - match_width_record;
    }

    if (is_count_width_record != ps->is_count_width) {
        ps->is_count_width = is_count_width_record;
    }
    // end

    // code for conditional backref
    if (group_type >= GT_BACKREF_CONDITIONAL_INDEX) {
        // without "no" branch
        if (!ps->m_cur->or_list) {
            OR_List* or_list = tre_new(OR_List, 1);
            or_list->codes = ps->m_cur->codes;
            or_list->next = NULL;
            ps->m_cur->or_list = or_list;
            ps->m_cur->or_num++;

            // GROUP_END -1
            ps->m_cur->codes->ins = INS_GROUP_END;
            ps->m_cur->codes->data = tre_new(uint32_t, 1);
            ps->m_cur->codes->len = 1;
            *ps->m_cur->codes->data = -1;
            ps->m_cur->codes->next = tre_new(INS_List, 1);
            ps->m_cur->codes = ps->m_cur->codes->next;
            ps->m_cur->codes->next = NULL;
            // END
        }

    }
    // end

    if (group_type == GT_NORMAL) ps->avaliable_group++;

    // CODE GENERATE
    // CMP_GROUP INDEX
    last_group->codes->ins = INS_CMP_GROUP;
    last_group->codes->data = tre_new(uint32_t, 1);
    last_group->codes->len = 1;
    *last_group->codes->data = gindex;
    last_group->codes->next = tre_new(INS_List, 1);
    last_group->codes = last_group->codes->next;
    last_group->codes->next = NULL;
    // END

    ps->m_cur = last_group;
    tre_lexer_next(ps->lex);
    TRE_DEBUG_PRINT("GROUP_END\n");
    return true;
}

bool parse_blocks(tre_Parser* ps) {
    while (ps->lex->token.value != TK_END) {
        if (!parse_block(ps)) return false;
    }
    return true;
}

/** return length of groups */
static _INLINE
int group_sort(ParserMatchGroup* parser_groups) {
    int gnum = 0;
    ParserMatchGroup *pg, *pg_last = NULL, *pg_tmp;
    ParserMatchGroup *new_tail = NULL, *new_tail_cur = NULL;

    for (pg = parser_groups; pg;) {
        if (pg->group_type > GT_NORMAL) {
            if (pg_last) pg_last->next = pg->next;

            if (!new_tail_cur) new_tail = new_tail_cur = pg;
            else {
                new_tail_cur->next = pg;
                new_tail_cur = new_tail_cur->next;
            }
            pg_tmp = pg->next;
            pg->next = NULL;
            pg = pg_tmp;
        } else {
            pg_last = pg;
            pg = pg->next;
        }
    }

    for (pg = parser_groups; pg->next; pg = pg->next);
    pg->next = new_tail;
    return gnum;
}

void clear_parser(ParserMatchGroup* parser_groups) {
    ParserMatchGroup *pg, *pg_tmp;
    INS_List *code, *code_tmp;
    OR_List *or_list, *or_list_tmp;

    for (pg = parser_groups; pg; ) {
        for (code = pg->codes_start; code->next; ) {
            code_tmp = code;
            code = code->next;
            free(code_tmp->data);
            free(code_tmp);
        }
        // the final one
        free(code);

        free(pg->name);

        for (or_list = pg->or_list; or_list; ) {
            or_list_tmp = or_list;
            or_list = or_list->next;
            free(or_list_tmp);
        }
        
        pg_tmp = pg;
        pg = pg->next;
        free(pg_tmp);
    }
}

void parser_init(tre_Parser* pas) {
    pas->error_code = 0;
    pas->avaliable_group = 1;
    pas->match_width = 0;
    pas->is_count_width = false;
}

tre_Pattern* tre_parser(tre_Lexer *lexer, int* perror_code) {
    tre_Parser *ps = tre_new(tre_Parser, 1);
    tre_Pattern *ret;

    parser_init(ps);
    ps->lex = lexer;

    ParserMatchGroup *m_cur, *m_start;
    m_cur = m_start = tre_new(ParserMatchGroup, 1);
    m_start->codes = m_start->codes_start = tre_new(INS_List, 1);
    m_start->codes->next = NULL;
    m_start->group_type = 0;
    m_cur->next = NULL;
    m_cur->or_num = 0;
    m_cur->or_list = NULL;
    m_cur->name = NULL;

    ps->m_cur = m_cur;
    ps->m_start = m_start;

    tre_lexer_next(ps->lex);
    parse_blocks(ps);

    *perror_code = ps->error_code;

    if (ps->lex->token.value == TK_END && (!ps->error_code)) {
        group_sort(m_start);
#ifdef TRE_DEBUG
        debug_ins_list_print(m_start);
#endif
        ret = compact_group(m_start);
        ret->num = ps->lex->max_normal_group_num;
        free(ps);
        return ret;
    }
    
    free(ps);
    clear_parser(m_start);
    return NULL;
}


tre_Pattern* compact_group(ParserMatchGroup* parser_groups) {
    uint32_t* data;
    int gnum = 0;
    MatchGroup* g;
    MatchGroup* groups;
    ParserMatchGroup *pg, *pg_tmp;
    INS_List *code, *code_tmp;
    OR_List *or_lst, *or_tmp;
    tre_Pattern* ret = tre_new(tre_Pattern, 1);

    for (pg = parser_groups; pg; pg = pg->next) gnum++;
    groups = tre_new(MatchGroup, gnum);

    gnum = 0;
    for (pg = parser_groups; pg; ) {
        int code_lens = pg->or_num * 2;
        or_lst = pg->or_list;
        g = groups + gnum;

        for (code = pg->codes_start; code->next; code = code->next) {
            code_lens += (code->len + 1);
        }

        // sizeof(int)*2 is space for group_end
        g->codes = tre_new(uint32_t, code_lens + 2);
        g->name = pg->name;
        g->name_len = pg->name_len;
        g->type = pg->group_type;
        g->extra = pg->group_extra;

        if (or_lst) {
            code_lens = pg->or_num * 2; // recount

            data = g->codes + (pg->or_num-1) * 2;
            for (code = pg->codes_start; true; code = code->next) {
                while (or_lst && or_lst->codes == code) {
                    // code for conditional backref
                    if (pg->group_type == GT_BACKREF_CONDITIONAL_INDEX) {
                        *data++ = INS_JMP;
                    // end
                    } else {
                        *data++ = INS_SAVE_SNAP;
                    }
                    *data = code_lens;
                    data -= 3;
                    or_tmp = or_lst;
                    or_lst = or_lst->next;
                    free(or_tmp);
                }
                if (!code->next) break;
                code_lens += (code->len + 1);
            }
        }

        data = g->codes + pg->or_num * 2;

        for (code = pg->codes_start; code->next; ) {
            *data++ = code->ins;

            if (code->len) {
                memcpy(data, code->data, code->len * sizeof(uint32_t));
                data += code->len;
                free(code->data);
            }

            code_tmp = code;
            code = code->next;
            free(code_tmp);
        }
        // the final one
        free(code);

        *data = INS_GROUP_END;
        *(data + 1) = gnum;

        gnum++;

        pg_tmp = pg;
        pg = pg->next;
        free(pg_tmp);
    }

    ret->groups = groups;
    ret->num_all = gnum;

    return ret;
}
