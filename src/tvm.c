
#include "tutils.h"
#include "tvm.h"
#include "tlexer.h"
#include "tdebug.h"

_INLINE static
VMSnap* snap_dup(VMSnap* src) {
    VMSnap* snap = _new(VMSnap, 1);
    memcpy(snap, src, sizeof(VMSnap));
    if (!stack_empty(src->run_cache)) {
        stack_copy(src->run_cache, snap->run_cache, RunCache);
    } else {
        snap->run_cache.len = 0;
        snap->run_cache.data = NULL;
    }
    return snap;
}

_INLINE static
void snap_free(VMSnap* snap) {
    stack_free(snap->run_cache);
    free(snap);
}

_INLINE static
void cache_old_snap(VMState* vms, VMSnap* snap) {
    snap->prev = (vms->snap_used) ? vms->snap_used : NULL;
    vms->snap_used = snap;
}

_INLINE static
VMSnap* get_cached_snap(VMState* vms) {
    VMSnap *tmp = vms->snap_used;
    if (tmp) vms->snap_used = tmp->prev;
    return tmp;
}


void vm_check_text_end(VMState* vms) {
    VMSnap* snap = vms->snap;
    if (snap->str_pos == (vms->input_str + vms->input_len)) {
        if (!snap->text_end) snap->text_end = true;
    } else {      
        if (snap->text_end) snap->text_end = false;
    }
}

bool vm_input_next(VMState* vms) {
    VMSnap* snap = vms->snap;
    snap->str_pos++;
    snap->chrcode = *snap->str_pos;
    return true;
}

_INLINE static
int jump_one_cmp(VMState* vms) {
    int num;
    VMSnap* snap = vms->snap;
    int ins = *snap->codes;

    if (ins >= ins_cmp && ins <= ins_cmp_group) {
        if (ins >= ins_cmp_multi && ins <= ins_ncmp_multi) {
            num = *(snap->codes + 1);
            snap->codes += (num * 3 + 2);
        } else {
            snap->codes += 2;
        }
    }
    return 1;
}

_INLINE static bool
char_cmp(uint32_t chrcode_re, uint32_t chrcode, int flag) {
    if (flag & FLAG_IGNORECASE) {
        return (tolower(chrcode_re) == tolower(chrcode));
    } else {
        return (chrcode_re == chrcode);
    }
}

_INLINE static
int do_ins_cmp(VMState* vms) {
    VMSnap* snap = vms->snap;
    uint32_t char_code = *(snap->codes + 1);

#ifdef TRE_DEBUG
    printf_u8("INS_CMP ");
    putcode(char_code);
    printf(" ");
    putcode(snap->chrcode);
    putchar('\n');
#endif
    if (snap->text_end) return 0;
    if (char_cmp(char_code, snap->chrcode, vms->flag))
        return 2;
    return 0;
}

_INLINE static bool
char_cmp_spe(uint32_t chrcode_re, uint32_t chrcode, int flag) {
    switch (chrcode_re) {
        case '.':
            if (flag & FLAG_DOTALL) return true;
            else if (chrcode != '\n') return true;
            break;
        case 'd': if (isdigit(chrcode)) return true; break;
        case 'D': if (!isdigit(chrcode)) return true; break;
        case 'w': if (isalnum(chrcode) || chrcode == '_') return true; break;
        case 'W': if (!isalnum(chrcode) || chrcode != '_') return true; break;
        case 's': if (isspace(chrcode)) return true; break;
        case 'S': if (!(isspace(chrcode))) return true; break;
        default: if (chrcode_re == chrcode) return true;
    }
    return false;
}

_INLINE static
int do_ins_cmp_spe(VMState* vms) {
    VMSnap* snap = vms->snap;
    uint32_t char_code = *(snap->codes + 1);

    TRE_DEBUG_PRINT("INS_CMP_SPE\n");

    if (snap->text_end) return 0;
    if (char_cmp_spe(char_code, snap->chrcode, vms->flag)) {
        return 2;
    }
    return 0;
}

_INLINE static bool
char_cmp_range(int range1, int range2, int chrcode, int flag) {
    if (flag & FLAG_IGNORECASE) {
        if (!('Z' < chrcode && chrcode < 'a')) {
            range1 = tolower(range1);
            range2 = tolower(range2);
            chrcode = tolower(chrcode);
        }
    }
    return (range1 <= chrcode) && (chrcode <= range2);
}

_INLINE static
int do_ins_cmp_multi(VMState* vms, bool is_ncmp) {
    int i;
    uint32_t _type, _code;
    bool match = false;
    VMSnap* snap = vms->snap;
    uint32_t* data = snap->codes + 1;
    int num = *data++;

    TRE_DEBUG_PRINT("INS_CMP_MULTI\n");

    if (snap->text_end) return 0;

    for (i = 0; i < num; i++) {
        _type = *(data + i * 3);
        _code = *(data + i * 3 + 1);

        if (_type == TK_CHAR) {
            if (char_cmp(_code, snap->chrcode, vms->flag)) {
                match = true;
                break;
            }
        } else if (_type == TK_CHAR_SPE) {
            if (char_cmp_spe(_code, snap->chrcode, vms->flag)) {
                match = true;
                break;
            }
        } else if (_type == '-') {
            if (char_cmp_range(_code, *((int*)data + i * 3 + 2), snap->chrcode, vms->flag)) {
                match = true;
                break;
            }
        }
    }

    if (is_ncmp) {
        return match ? 0 : (num * 3 + 2);
    } else {
        return match ? (num * 3 + 2) : 0;
    }
}

_INLINE static
int do_ins_cmp_backref(VMState* vms) {
    int index = *(vms->snap->codes + 1);

    TRE_DEBUG_PRINT("INS_CMP_BACKREF\n");

    if (vms->snap->text_end) return 0;
    if (index >= vms->group_num)
        return 0;

    if (vms->match_results[index].head && vms->match_results[index].tail) {
        uint32_t *p = vms->match_results[index].head;
        uint32_t *tail = vms->match_results[index].tail;
        uint32_t *p2 = vms->snap->str_pos;

        while (p < tail) {
            if (!char_cmp(*p, *p2, vms->flag)) return 0;
            p++;
            p2++;
        }

        // ƥ����ɺ󽫱�+1�����ȼ�ȥ
        vms->snap->str_pos = p2-1;
        return 2;
    }

    return 0;
}

_INLINE static
void save_snap(VMState* vms) {
    TRE_DEBUG_PRINT("INS_SAVE_SNAP\n");
    VMSnap* snap = get_cached_snap(vms);
    if (snap) {
        VMSnap *src = vms->snap;
        tre_Stack rc = snap->run_cache;
        memcpy(snap, src, sizeof(VMSnap));

        if (rc.len < src->run_cache.top+1) {
            free(rc.data);
            rc.len = src->run_cache.top+1;
            rc.data = _new(RunCache , rc.len);
        }
        rc.top = src->run_cache.top;
        if (rc.top != -1) memcpy(rc.data, src->run_cache.data, sizeof(RunCache) * (src->run_cache.top+1));
        snap->run_cache = rc;
        //stack_copy(src->run_cache, snap->run_cache, RunCache);
    } else {
        snap = snap_dup(vms->snap);
    }
    vms->snap->prev = snap;
}

_INLINE static
int do_ins_jmp(VMState* vms) {
    VMSnap* snap = vms->snap;
    int offset = *(snap->codes + 1);
    TRE_DEBUG_PRINT("INS_JMP\n");
    snap->codes = vms->groups[snap->cur_group].codes + offset + 2;
    return 1;
}

_INLINE static
int do_ins_cmp_group(VMState* vms) {
    RunCache *rc;
    VMSnap* snap = vms->snap;
    int index = *(snap->codes + 1);
    MatchGroup* g = vms->groups + index;

#ifdef TRE_DEBUG
    printf("INS_CMP_GROUP %d\n", index);
#endif

    // works for special groups (?=) (?!) (?<=) (?<!)
    if (g->type == GT_IF_MATCH) {
        vms->input_cache[index - vms->group_num] = snap->str_pos;
    } else if (g->type == GT_IF_NOT_MATCH || g->type == GT_IF_NOT_PRECEDED_BY) {
        if (snap->mr.enable == 1) {
            save_snap(vms);
        } else {
            snap->codes += 2;
            save_snap(vms);
            snap->codes -= 2;
        }

        if (g->type == GT_IF_NOT_PRECEDED_BY) {
            // current matched length less than group's length
            if (snap->str_pos - vms->input_str < g->extra) {
                return 0;
            }
            snap->str_pos = snap->str_pos - g->extra;
            snap->chrcode = *snap->str_pos;
        }
    } else if (g->type == GT_IF_PRECEDED_BY) {
        // current matched length less than group's length
        if (snap->str_pos - vms->input_str < g->extra) {
            return 0;
        }
        vms->input_cache[index - vms->group_num] = snap->str_pos;
        snap->str_pos = snap->str_pos - g->extra;
        snap->chrcode = *snap->str_pos;
    }

    // save cache
    stack_check(snap->run_cache, RunCache, 5 + snap->run_cache.len);
    rc = stack_push(snap->run_cache, RunCache);
    rc->codes_cache = snap->codes;
    rc->mr = snap->mr;
    rc->cur_group = snap->cur_group;

    // load group code
    snap->codes = g->codes;
    snap->mr.enable = 0;
    snap->cur_group = index;

    // code for conditional backref
    if (g->type == GT_BACKREF_CONDITIONAL_INDEX) {
        if (vms->match_results[g->extra].head && vms->match_results[g->extra].tail) {
            snap->codes += 2;
        }
    }
    // end

    // set match result, value of head
    if (index < vms->group_num_all) {
        vms->match_results[index].tmp = snap->str_pos;
    }

    return 1;
}

_INLINE static
int do_ins_group_end(VMState* vms) {
    RunCache *rc;
    VMSnap* snap = vms->snap;
    int index = *(snap->codes + 1);
    if (index == -1) index = snap->cur_group;
    MatchGroup* g = vms->groups + index;
    VMSnap* snap_tmp;

#ifdef TRE_DEBUG
    printf("INS_GROUP_END %d\n", *(snap->codes + 1));
#endif

    // load cache
    if (!stack_empty(snap->run_cache)) {
        rc = stack_pop(snap->run_cache, RunCache);
        snap->codes = rc->codes_cache;
        snap->mr = rc->mr;
        snap->cur_group = rc->cur_group;
    }

    // works for special groups (?=) (?!) (?<=) (?<!)
    if (g->type == GT_IF_MATCH || g->type == GT_IF_PRECEDED_BY) {
        snap->str_pos = vms->input_cache[index - vms->group_num];
        snap->chrcode = *snap->str_pos;
    } else if (g->type == GT_IF_NOT_MATCH || g->type == GT_IF_NOT_PRECEDED_BY) {
        uint32_t* next_ins = snap->codes + 2;
        if (snap->mr.enable == 1) next_ins -= 2;
        do {
            snap_tmp = snap;
            snap = snap->prev;
            free(snap_tmp);
        } while (snap->codes != next_ins);
        vms->snap = snap;
        return 0;
    }

    // set match result
    if (index < vms->group_num_all) {
        vms->match_results[index].head = vms->match_results[index].tmp;
        vms->match_results[index].tail = snap->str_pos;
    }

    // end if GROUP(0) matched
    // 2 is length of CMP_GROUP
    return (index == 0) ? -1 : 2;
}

int backtracking_length(VMState* vms) {
    int len = 0;
    VMSnap* snap = vms->snap->prev;
    while (snap) {
        len++;
        snap = snap->prev;
    }
    return len;
}

_INLINE static
int try_backtracking(VMState* vms) {
    VMSnap* snap = vms->snap;
    if (snap->prev) {
        bool greed;
        VMSnap* tmp;

        if (vms->backtrack_limit && vms->backtrack_num >= vms->backtrack_limit) {
            return -2;
        }
        vms->backtrack_num += 1;

        tmp = snap;
        snap = snap->prev;
        vms->snap = snap;
        for (int i=0; i<vms->group_num; i++) {
            if (vms->match_results[i].head && vms->match_results[i].head > snap->str_pos) {
                vms->match_results[i].head = NULL;
            }
        }
        cache_old_snap(vms, tmp);
        greed = snap->mr.enable == 1 ? true : false;

        if (greed) TRE_DEBUG_PRINT("INS_BACKTRACK\n");
        else TRE_DEBUG_PRINT("INS_BACKTRACK_NG\n");

        if (greed) {
            snap->mr.enable = 0;
            return jump_one_cmp(vms);
        }
        return 1;
    }
    TRE_DEBUG_PRINT("INS_BACKTRACK\n");
    return 0;
}

_INLINE static
int do_ins_checkpoint(VMState* vms, bool greed) {
    VMSnap* snap = vms->snap;
    int llimit = *(snap->codes + 1);
    int rlimit = *(snap->codes + 2);

    if (greed) TRE_DEBUG_PRINT("INS_CHECK_POINT\n");
    else TRE_DEBUG_PRINT("INS_CHECK_POINT_NG\n");

    snap->codes += 3;

    // a{0,0}
    if (rlimit == 0) {
        return jump_one_cmp(vms);
    }

    // a{2,1}
    if (rlimit != -1 && llimit > rlimit) {
        // sre_constants.error: bad repeat interval
        return -2;
    }

    snap->mr.enable = greed ? 1 : 2;
    snap->mr.llimit = llimit;
    snap->mr.rlimit = rlimit;
    snap->mr.cur_repeat = 0;

    // save snap when reach llimit
    if (llimit == 0) {
        save_snap(vms);

        // no greedy match
        if (!greed) {
            return jump_one_cmp(vms);
        }
    }

    return 1;
}


_INLINE static
int do_ins_save_snap(VMState* vms) {
    uint32_t* tmp;
    tmp = vms->snap->codes;
    // group start + offset + length of group_end
    vms->snap->codes = vms->groups[vms->snap->cur_group].codes + *(vms->snap->codes + 1) + 2;
    save_snap(vms);
    vms->snap->codes = tmp + 2;
    return 2;
}


int vm_step(VMState* vms) {
    int ret;
    VMSnap* snap = vms->snap;
    uint32_t cur_ins = *snap->codes;
    vm_check_text_end(vms);

    if (cur_ins >= ins_cmp && cur_ins <= ins_group_end) {
        // no greedy match
        int group_cache;

        if (cur_ins == ins_cmp) {
            ret = do_ins_cmp(vms);
        } else if (cur_ins == ins_cmp_spe) {
            ret = do_ins_cmp_spe(vms);
        } else if (cur_ins == ins_cmp_multi) {
            ret = do_ins_cmp_multi(vms, false);
        } else if (cur_ins == ins_ncmp_multi) {
            ret = do_ins_cmp_multi(vms, true);
        } else if (cur_ins == ins_cmp_backref) {
            ret = do_ins_cmp_backref(vms);
        } else if (cur_ins == ins_cmp_group) {
            ret = do_ins_cmp_group(vms);
        } else if (cur_ins == ins_group_end) {
            group_cache = snap->cur_group;
            ret = do_ins_group_end(vms);
        }

        if (ret) {
            if (cur_ins < ins_cmp_group) {
                vm_input_next(vms);
            }
            // CMP_GROUP is very different,
            // It's begin of match, and end at GROUP_END.
            // Other CMPs is both begin and end of one match.
            if (cur_ins != ins_cmp_group) {
                // match again when a? a+ a* a{x,y}
                if (snap->mr.enable) {
                    bool greed = snap->mr.enable == 1 ? true : false;
                    snap->mr.cur_repeat++;

                    if (greed) {
                        // ̰��ģʽ �ﵽ��߽磬���ʸ񱣴����
                        if (snap->mr.cur_repeat >= snap->mr.llimit && snap->mr.llimit != snap->mr.rlimit) {
                            save_snap(vms);
                        }
                        // �ﵽ�ұ߽磬Խ��ƥ��ָ�����ѭ��
                        if (snap->mr.cur_repeat == snap->mr.rlimit) {
                            snap->mr.enable = 0;
                            snap->codes += ret;
                        }
                        // �ر�ģ�ƥ��մ����鱻ǿ������
                        if ((cur_ins == ins_group_end) && (snap->mr.cur_repeat >= snap->mr.llimit) &&
                                (snap->mr.cur_repeat != snap->mr.rlimit)) {
                            // match "nothing" is no sense : (|)
                            if (vms->match_results[group_cache].head == vms->match_results[group_cache].tail) {
                                snap->mr.enable = 0;
                                snap->codes += 2;
                            }
                        }
                    } else {
                        if (snap->mr.cur_repeat >= snap->mr.llimit) {
                            if (((cur_ins != ins_group_end) && (snap->mr.cur_repeat != snap->mr.rlimit)) ||
                                ((cur_ins == ins_group_end) && (snap->mr.cur_repeat != snap->mr.rlimit) && 
                                 (vms->match_results[group_cache].head != vms->match_results[group_cache].tail))) {
                                save_snap(vms);
                            }
                            snap->mr.enable = 0;
                            snap->codes += ret;
                        }
                    }
                } else {
                    snap->codes += ret;
                }
            }
        } else {
            ret = try_backtracking(vms);
        }
    } else if (cur_ins == ins_check_point) {
        ret = do_ins_checkpoint(vms, true);
    } else if (cur_ins == ins_check_point_no_greed) {
        ret = do_ins_checkpoint(vms, false);
    } else if (cur_ins == ins_save_snap) {
        ret = do_ins_save_snap(vms);
    } else if (cur_ins == ins_jmp) {
        ret = do_ins_jmp(vms);
    } else if (cur_ins == ins_match_start) {
        // ^
        // Tip for multiline: \n is newline, \r is nothing, tested.
        TRE_DEBUG_PRINT("INS_MATCH_START\n");
        if (snap->str_pos == vms->input_str || ((vms->flag & FLAG_MULTILINE) && (*(snap->str_pos - 1) == '\n'))) {
            snap->codes += 1;
            ret = 1;
        } else {
            ret = try_backtracking(vms);
        }
    } else if (cur_ins == ins_match_end) {
        // $
        TRE_DEBUG_PRINT("INS_MATCH_END\n");
        if (snap->text_end || ((vms->flag & FLAG_MULTILINE) && (snap->chrcode == '\n'))) {
            snap->codes += 1;
            ret = 1;
        } else {
            ret = try_backtracking(vms);
        }
    }

    return ret;
}

uint32_t* u8str_to_u32str(const char* p, int* len) {
    int *ret, *p2;
    int i, code, slen = utf8_len(p);

    ret = p2 = _new(int, slen + 1);

    for (i = 0; i < slen; i++) {
        p = utf8_decode(p, &code);
        *p2++ = code;
    }

    *len = slen;
    *p2 = 0;
    return ret;
}

VMState* vm_init(tre_Pattern* groups_info, const char* input_str, int backtrack_limit) {
    VMState* vms = _new(VMState, 1);
    vms->raw_input_str = input_str;
    vms->input_str = u8str_to_u32str(input_str, &(vms->input_len));

    vms->group_num = groups_info->num;
    vms->group_num_all = groups_info->num_all;
    vms->groups = groups_info->groups;
    if (groups_info->num_all > groups_info->num) {
        vms->input_cache = (uint32_t**)_new(int*, groups_info->num_all - groups_info->num);
    } else {
        vms->input_cache = NULL;
    }
    vms->flag = groups_info->flag;
    vms->backtrack_num = 0;
    vms->backtrack_limit = backtrack_limit;

    // init match results of groups
    vms->match_results = _new(GroupResultTemp, groups_info->num_all); //  tre.match(r'(?:a?)*y', 'z')
    memset(vms->match_results, 0, sizeof(GroupResultTemp) * groups_info->num_all);
    vms->match_results[0].tmp = vms->input_str;

    // init first snap
    vms->snap = _new(VMSnap, 1);
    vms->snap->codes = groups_info->groups[0].codes;
    vms->snap->str_pos = vms->input_str;
    vms->snap->chrcode = *vms->input_str;
    vms->snap->cur_group = 0;
    vms->snap->text_end = false;
    memset(&vms->snap->mr, 0, sizeof(MatchRepeat));
    stack_init(vms->snap->run_cache, RunCache, 0);
    vms->snap->prev = NULL;
    vms->snap_used = NULL;
    return vms;
}

tre_GroupResult* vm_exec(VMState* vms) {
    int i;
    int ret = 1;
    tre_GroupResult* results;

    while (ret > 0) {
        ret = vm_step(vms);
    }

    if (ret == -1) {
        results = _new(tre_GroupResult, vms->group_num);
        memset(results, 0, sizeof(tre_GroupResult) * vms->group_num);
        for (i = 0; i < vms->group_num; i++) {
            if (vms->match_results[i].head && vms->match_results[i].tail) {
                results[i].head = vms->match_results[i].head - vms->input_str;
                results[i].tail = vms->match_results[i].tail - vms->input_str;
                if (vms->groups[i].name) {
                    results[i].name = _new(char, strlen(vms->groups[i].name)+1);
                    memcpy(results[i].name, vms->groups[i].name, strlen(vms->groups[i].name)+1);
                }
            } else {
                results[i].head = -1;
            }
        }
        return results;
    }

    return NULL;
}

void vm_free(VMState* vms)
{
    VMSnap *snap, *snap_tmp;

    for (snap = vms->snap; snap;) {
        stack_free(snap->run_cache)
        snap_tmp = snap;
        snap = snap->prev;
        free(snap_tmp);
    }

    for (snap = vms->snap_used; snap;) {
        stack_free(snap->run_cache)
        snap_tmp = snap;
        snap = snap->prev;
        free(snap_tmp);
    }

    if (vms->input_cache) free(vms->input_cache);
    free(vms->match_results);
    free(vms);
}

