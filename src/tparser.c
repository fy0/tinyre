
#include "tutils.h"
#include "tlexer.h"
#include "tparser.h"
#include "tvm.h"
#include "tdebug.h"

#define paser_accept(__stat) if (!((__stat))) return NULL;
#define check_token(tk) if (tk == NULL || tk->token == 0) return NULL;

tre_Token* parser_single_char(ParserInfo* pi, tre_Token* tk);
tre_Token* parser_char_set(ParserInfo* pi, tre_Token* tk);
tre_Token* parser_char(ParserInfo* pi, tre_Token* tk);
tre_Token* parser_block(ParserInfo* pi, tre_Token* tk);
tre_Token* parser_group(ParserInfo* pi, tre_Token* tk);

tre_Token* parser_single_char(ParserInfo* pi, tre_Token* tk) {
    if (tk->token == TK_CHAR || tk->token == TK_SPE_CHAR) {
        // CODE GENERATE
        // CMP/CMP_SPE CODE
        pi->m_cur->codes->ins = tk->token == TK_CHAR ? ins_cmp : ins_cmp_spe;
        pi->m_cur->codes->data = _new(uint32_t, 1);
        pi->m_cur->codes->len = 1;
        *pi->m_cur->codes->data = tk->info.code;
        pi->m_cur->codes->next = _new(INS_List, 1);
        pi->m_cur->codes = pi->m_cur->codes->next;
        pi->m_cur->codes->next = NULL;
        // END
        return tk + 1;
    }
    return NULL;
}

static _INLINE
tre_Token* parser_char_set_test_single(ParserInfo* pi, tre_Token* tk) {
    paser_accept(tk->token == TK_CHAR || tk->token == TK_SPE_CHAR || tk->token == '-');
    if (tk->token == '-') {
        tk->info.code = '-';
        tk->token = TK_CHAR;
    }
    return tk + 1;
}

static _INLINE
tre_Token* parser_char_set_test_range(ParserInfo* pi, tre_Token* tk) {
    paser_accept(tk->token == TK_CHAR || tk->token == '-');
    tk++;
    paser_accept((tk++)->token == '-');
    paser_accept(tk->token == TK_CHAR || tk->token == '-');
    tk++;
    // reset token
    if ((tk - 1)->token == '-') {
        (tk - 1)->info.code = '-';
        (tk - 1)->token = TK_CHAR;
    }
    if ((tk - 3)->token == '-') {
        (tk - 3)->info.code = '-';
        (tk - 3)->token = TK_CHAR;
    }
    // swap tk-2, tk-3 => [a-z] to [-az]
    (tk - 2)->info.code = (tk - 3)->info.code;
    (tk - 2)->token = TK_CHAR;
    (tk - 3)->token = '-';
    return tk;
}

tre_Token* parser_char_set(ParserInfo* pi, tre_Token* tk) {
    int num = 0;
    int* data;
    bool is_ncmp;
    tre_Token* ret;
    tre_Token* start;

    TRE_DEBUG_PRINT("CHAR_SET\n");

    paser_accept(tk->token == '[');
    is_ncmp = (tk->info.code == 1) ? true : false;
    tk++;
    check_token(tk);
    start = tk;
    
    while (tk->token == TK_CHAR || tk->token == TK_SPE_CHAR || tk->token == '-') {
        ret = parser_char_set_test_range(pi, tk);
        if (!ret) ret = parser_char_set_test_single(pi, tk);
        tk = ret;
        check_token(tk);
        num++;
    }

    paser_accept(tk->token == ']');

    // CODE GENERATE
    // CMP_MULTI NUM [TYPE1 CODE1 NOOP], [TYPE2 CODE2 NOOP], [TYPE3 RANGE1 RANGE2] ...
    pi->m_cur->codes->ins = is_ncmp ? ins_ncmp_multi : ins_cmp_multi;
    pi->m_cur->codes->data = _new(uint32_t, 3 * num + 1);
    pi->m_cur->codes->len = (3 * num + 1);
    // END

    data = (int*)pi->m_cur->codes->data;
    *(data++) = num;

    for (; start != tk; start++) {
        *data = start->token;
        if (start->token == '-') {
            *(data + 1) = (start+1)->info.code;
            *(data + 2) = (start+2)->info.code;
            if ((start + 2)->info.code < (start + 1)->info.code) {
                pi->error_code = ERR_PARSER_BAD_CHARACTER_RANGE;
                return NULL;
            }
            start += 2;
        } else {
            *(data + 1) = start->info.code;
        }
        data += 3;
    }

    pi->m_cur->codes->next = _new(INS_List, 1);
    pi->m_cur->codes = pi->m_cur->codes->next;
    pi->m_cur->codes->next = NULL;
    return tk + 1;
}

tre_Token* parser_char(ParserInfo* pi, tre_Token* tk) {
    TRE_DEBUG_PRINT("CHAR\n");

    tre_Token* ret = parser_single_char(pi, tk);
    if (!ret) ret = parser_char_set(pi, tk);
    if (ret && pi->is_count_width) pi->match_width += 1;
    return ret;
}

tre_Token* parser_other_tokens(ParserInfo* pi, tre_Token* tk) {
    if (tk->token == '^' || tk->token == '$') {
        // CODE GENERATE
        // MATCH_START/MATCH_END
        pi->m_cur->codes->ins = tk->token == '^' ? ins_match_start : ins_match_end;
        pi->m_cur->codes->len = 0;
        pi->m_cur->codes->next = _new(INS_List, 1);
        pi->m_cur->codes = pi->m_cur->codes->next;
        pi->m_cur->codes->next = NULL;
        // END
        return tk + 1;
    }
    return NULL;
}

tre_Token* parser_or(ParserInfo* pi, tre_Token* tk) {
    // try to catch a |
    OR_List *or_list, *or2_list;
    paser_accept(tk->token == '|');

    // code for conditional backref
    if (pi->m_cur->group_type >= GT_BACKREF_CONDITIONAL_INDEX) {
        if (pi->m_cur->group_extra) {
            pi->error_code = ERR_PARSER_CONDITIONAL_BACKREF;
            return NULL;
        }
        pi->m_cur->group_extra = 1;
    }
    // end

    or_list = _new(OR_List, 1);
    or_list->codes = pi->m_cur->codes;
    or_list->next = NULL;

    if (pi->m_cur->or_list) {
        or2_list = pi->m_cur->or_list;
        while (or2_list->next) {
            or2_list = or2_list->next;
        }
        or2_list->next = or_list;
    } else {
        pi->m_cur->or_list = or_list;
    }

    // CODE GENERATE
    // GROUP_END -1
    pi->m_cur->codes->ins = ins_group_end;
    pi->m_cur->codes->data = _new(uint32_t, 1);
    pi->m_cur->codes->len = 1;
    *pi->m_cur->codes->data = -1;
    pi->m_cur->codes->next = _new(INS_List, 1);
    pi->m_cur->codes = pi->m_cur->codes->next;
    pi->m_cur->codes->next = NULL;
    pi->m_cur->or_num++;
    // END
    return tk + 1;
}

tre_Token* parser_back_ref(ParserInfo* pi, tre_Token* tk) {
    if (tk->token == TK_BACK_REF) {
        // CODE GENERATE
        // CMP_BACKREF INDEX
        pi->m_cur->codes->ins = ins_cmp_backref;
        pi->m_cur->codes->data = _new(uint32_t, 1);
        pi->m_cur->codes->len = 1;
        *pi->m_cur->codes->data = tk->info.index;
        pi->m_cur->codes->next = _new(INS_List, 1);
        pi->m_cur->codes = pi->m_cur->codes->next;
        pi->m_cur->codes->next = NULL;
        // END
        return tk + 1;
    }
    return NULL;
}

tre_Token* parser_block(ParserInfo* pi, tre_Token* tk) {
    tre_Token *ret, *ret2;
    INS_List* last_ins;

    TRE_DEBUG_PRINT("BLOCK\n");

    check_token(tk);
    last_ins = pi->m_cur->codes;

    ret = parser_char(pi, tk);
    if (!ret) ret = parser_group(pi, tk);
    if (!ret) ret = parser_back_ref(pi, tk);
    if (!ret) {
        ret2 = parser_or(pi, tk);
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
                llimit = tk->info.index;
                rlimit = (tk + 1)->info.index;
                need_checkpoint = true;
            } else {
                pi->error_code = ERR_PARSER_IMPOSSIBLE_TOKEN;
                return NULL;
            }
        }

        if (need_checkpoint) {
            if (ret->token == '?') {
                ret ++;
                greed = false;
            }

            if (pi->is_count_width) {
                if (llimit != rlimit) {
                    pi->error_code = ERR_PARSER_REQUIRES_FIXED_WIDTH_PATTERN;
                    return NULL;
                }
                if (llimit > 0) {
                    pi->match_width += llimit - 1;
                }
            }
        }

        // CODE GENERATE
        // CHECK_POINT LLIMIT RLIMIT
        if (need_checkpoint) {
            // 将上一条指令（必然是一条匹配指令复制一遍）
            memcpy(pi->m_cur->codes, last_ins, sizeof(INS_List));
            // 为这条指令创建新的后继节点
            pi->m_cur->codes->next = _new(INS_List, 1);
            pi->m_cur->codes = pi->m_cur->codes->next;
            pi->m_cur->codes->next = NULL;

            last_ins->ins = greed ? ins_check_point : ins_check_point_no_greed;
            last_ins->data = _new(uint32_t, 2);
            last_ins->len = 2;
            *last_ins->data = llimit;
            *(last_ins->data + 1) = rlimit;
        }
        // END
    } else {
        ret = parser_other_tokens(pi, tk);
    }

    if (!ret) {
        if (!pi->error_code) pi->error_code = ERR_PARSER_NOTHING_TO_REPEAT;
    }
    
    return ret;
}

tre_Token* parser_group(ParserInfo* pi, tre_Token* tk) {
    int gindex;
    int group_type;
    char* name = NULL;
    tre_Token* ret;
    ParserMatchGroup* last_group;

    int match_width_record;
    bool is_count_width_record = pi->is_count_width;

    TRE_DEBUG_PRINT("GROUP\n");

    paser_accept(tk->token == '(');
    group_type = tk->info.code;

    if (pi->tk_info->group_names && pi->tk_info->group_names->tk == tk) {
        name = pi->tk_info->group_names->name;
    }

    // code for back reference (?P=), backref group is not real group
    if (group_type == GT_BACKREF_CONDITIONAL_INDEX) {
        int i = 1;
        for (ParserMatchGroup* pg = pi->m_start->next; pg; pg = pg->next) {
            if (pg->name && (memcmp(name, pg->name, strlen(name)) == 0)) {
                pi->m_cur->codes->ins = ins_cmp_backref;
                pi->m_cur->codes->data = _new(uint32_t, 1);
                pi->m_cur->codes->len = 1;
                *pi->m_cur->codes->data = i;
                pi->m_cur->codes->next = _new(INS_List, 1);
                pi->m_cur->codes = pi->m_cur->codes->next;
                pi->m_cur->codes->next = NULL;
                pi->tk_info->group_names = pi->tk_info->group_names->next;
                return tk + 1;
            }
            i++;
        }
        pi->error_code = ERR_PARSER_UNKNOWN_GROUP_NAME;
        return NULL;
    }
    // end

    if (group_type == GT_NORMAL) {
        gindex = 1; // 注意 gindex 不等于 avaliable_group 这里我犯过一次错误
    } else {
        gindex = pi->tk_info->max_normal_group_num;
    }

    // code for (?<=...) (?<!...)
    if (group_type == GT_IF_PRECEDED_BY || group_type == GT_IF_NOT_PRECEDED_BY) {
        if (!pi->is_count_width) pi->is_count_width = true;
        match_width_record = pi->match_width;
    }
    if (pi->is_count_width && (group_type == GT_IF_MATCH || group_type == GT_IF_NOT_MATCH)) {
        pi->is_count_width = false;
    }
    // end

    tk++;
    check_token(tk);

    // 前进至最后
    last_group = pi->m_cur;
    pi->m_cur = pi->m_start;

    while (pi->m_cur->next) {
        pi->m_cur = pi->m_cur->next;
        if (group_type == GT_NORMAL && pi->m_cur->group_type == GT_NORMAL) gindex++;
        if (group_type != GT_NORMAL && pi->m_cur->group_type != GT_NORMAL) gindex++;
    }

    // 创建新节点
    pi->m_cur->next = _new(ParserMatchGroup, 1);
    pi->m_cur->next->codes = pi->m_cur->next->codes_start = _new(INS_List, 1);
    pi->m_cur = pi->m_cur->next;
    pi->m_cur->next = NULL;
    pi->m_cur->or_num = 0;
    pi->m_cur->or_list = NULL;
    pi->m_cur->name = (group_type == GT_NORMAL) ? name : NULL;
    pi->m_cur->group_type = group_type;
    pi->m_cur->codes->next = NULL;

    // code for conditional backref
    if (group_type == GT_BACKREF_CONDITIONAL_INDEX && (!name)) {
        // (?(0)...) is normal group
        group_type = pi->m_cur->group_type = 0;
    }
    if (group_type >= GT_BACKREF_CONDITIONAL_INDEX) {
        pi->m_cur->group_extra = 0; // flag
    }
    // end

    if (name) {
        if (group_type == GT_NORMAL) pi->tk_info->group_names->name = NULL; // fix for mem free
        pi->tk_info->group_names = pi->tk_info->group_names->next;
    }

    ret = tk;
    while ((ret = parser_block(pi, ret))) tk = ret;

    paser_accept(tk->token == ')');

    // code for (?<=...) (?<!...)
    if (group_type == GT_IF_PRECEDED_BY || group_type == GT_IF_NOT_PRECEDED_BY) {
        pi->m_cur->group_extra = pi->match_width - match_width_record;
    }

    if (is_count_width_record != pi->is_count_width) {
        pi->is_count_width = is_count_width_record;
    }
    // end

    // code for conditional backref
    if (group_type >= GT_BACKREF_CONDITIONAL_INDEX) {
        if (name) {
            int i = 1;
            bool ok = false;
            for (ParserMatchGroup* pg = pi->m_start->next; pg; pg = pg->next) {
                if (pg->group_type == GT_NORMAL && pg->name && (memcmp(name, pg->name, strlen(name)) == 0)) {
                    pi->m_cur->group_extra = i;
                    ok = true;
                    break;
                }
                i++;
            }
            if (!ok) {
                pi->error_code = ERR_PARSER_UNKNOWN_GROUP_NAME;
                return NULL;
            }
        } else {
            pi->m_cur->group_extra = group_type - GT_BACKREF_CONDITIONAL_INDEX;
            /* not an error
            if (m_cur->group_extra >= avaliable_group) {
                error_code = ERR_PARSER_INVALID_GROUP_INDEX;
                return NULL;
            }*/
            pi->m_cur->group_type = GT_BACKREF_CONDITIONAL_INDEX;
        }

        // without "no" branch
        if (!pi->m_cur->or_list) {
            OR_List* or_list = _new(OR_List, 1);
            or_list->codes = pi->m_cur->codes;
            or_list->next = NULL;
            pi->m_cur->or_list = or_list;
            pi->m_cur->or_num++;

            // GROUP_END -1
            pi->m_cur->codes->ins = ins_group_end;
            pi->m_cur->codes->data = _new(uint32_t, 1);
            pi->m_cur->codes->len = 1;
            *pi->m_cur->codes->data = -1;
            pi->m_cur->codes->next = _new(INS_List, 1);
            pi->m_cur->codes = pi->m_cur->codes->next;
            pi->m_cur->codes->next = NULL;
            // END
        }

    }
    // end

    if (group_type == GT_NORMAL) pi->avaliable_group++;

    // CODE GENERATE
    // CMP_GROUP INDEX
    last_group->codes->ins = ins_cmp_group;
    last_group->codes->data = _new(uint32_t, 1);
    last_group->codes->len = 1;
    *last_group->codes->data = gindex;
    last_group->codes->next = _new(INS_List, 1);
    last_group->codes = last_group->codes->next;
    last_group->codes->next = NULL;
    // END

    pi->m_cur = last_group;
    return tk+1;
}

tre_Token* parser_blocks(ParserInfo* pi, tre_Token* tk) {
    tre_Token* ret;
    ret = tk;
    while ((ret = parser_block(pi, ret))) tk = ret;
    return tk;
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

void parser_info_init(ParserInfo* pi) {
    pi->error_code = 0;
    pi->avaliable_group = 1;
    pi->match_width = 0;
    pi->is_count_width = false;
}

tre_Pattern* tre_parser(TokenListInfo* tki, tre_Token** last_token, int* perror_code) {
    tre_Token *tokens;
    tre_Pattern *ret;
    ParserInfo *pi = _new(ParserInfo, 1);

    parser_info_init(pi);
    pi->tk_info = tki;
    pi->tk_info->group_names = tki->group_names;

    ParserMatchGroup *m_cur, *m_start;
    m_cur = m_start = _new(ParserMatchGroup, 1);
    m_start->codes = m_start->codes_start = _new(INS_List, 1);
    m_start->codes->next = NULL;
    m_start->group_type = 0;
    m_cur->next = NULL;
    m_cur->or_num = 0;
    m_cur->or_list = NULL;
    m_cur->name = NULL;

    pi->m_cur = m_cur;
    pi->m_start = m_start;

    tokens = parser_blocks(pi, tki->tokens);
    *last_token = tokens;
    *perror_code = pi->error_code;
    free(pi);

    if (tokens && (tokens >= tki->tokens + tki->token_num)) {
        group_sort(m_start);
#ifdef TRE_DEBUG
        debug_ins_list_print(m_start);
#endif
        ret = compact_group(m_start);
        ret->num = tki->max_normal_group_num;
        return ret;
    }

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
    tre_Pattern* ret = _new(tre_Pattern, 1);

    for (pg = parser_groups; pg; pg = pg->next) gnum++;
    groups = _new(MatchGroup, gnum);

    gnum = 0;
    for (pg = parser_groups; pg; ) {
        int code_lens = pg->or_num * 2;
        or_lst = pg->or_list;
        g = groups + gnum;

        for (code = pg->codes_start; code->next; code = code->next) {
            code_lens += (code->len + 1);
        }

        // sizeof(int)*2 is space for group_end
        g->codes = _new(uint32_t, code_lens + 2);
        g->name = pg->name;
        g->type = pg->group_type;
        g->extra = pg->group_extra;

        if (or_lst) {
            code_lens = pg->or_num * 2; // recount

            data = g->codes + (pg->or_num-1) * 2;
            for (code = pg->codes_start; true; code = code->next) {
                while (or_lst && or_lst->codes == code) {
                    // code for conditional backref
                    if (pg->group_type == GT_BACKREF_CONDITIONAL_INDEX) {
                        *data++ = ins_jmp;
                    // end
                    } else {
                        *data++ = ins_save_snap;
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

        *data = ins_group_end;
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
