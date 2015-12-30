
#include "tutils.h"
#include "tvm.h"
#include "tlexer.h"
#include "tdebug.h"

_INLINE static
VMSnap* snap_dup(VMSnap* src) {
    VMSnap* snap = _new(VMSnap, 1);
    RunCache *rc, *rc_src;
    memcpy(snap, src, sizeof(VMSnap));

    if (src->run_cache) {
        snap->run_cache = rc = _new(RunCache, 1);
        for (rc_src = src->run_cache; rc_src; rc_src = rc_src->prev) {
            rc->codes_cache = rc_src->codes_cache;
            rc->mr = rc_src->mr;
            rc->cur_group = rc_src->cur_group;
            rc->prev = rc_src->prev ? _new(RunCache, 1) : NULL;
            rc = rc->prev;
        }
    } else {
        snap->run_cache = NULL;
    }

    return snap;
}

_INLINE static
void snap_free(VMSnap* snap) {
    RunCache *rc, *rc2;
    if (snap->run_cache) {
        for (rc = snap->run_cache; rc;) {
            rc2 = rc;
            rc = rc->prev;
            free(rc2);
        }
    }
    free(snap);
}

void vm_check_text_end(VMState* vms) {
    if (vms->snap->str_pos == (vms->input_str + vms->input_len)) {
        if (!vms->snap->text_end) vms->snap->text_end = true;
    } else {
        if (vms->snap->text_end) vms->snap->text_end = false;
    }
}

bool vm_input_next(VMState* vms) {
    vms->snap->str_pos++;
    vms->snap->chrcode = *vms->snap->str_pos;
    return true;
}

_INLINE static
int jump_one_cmp(VMState* vms) {
    int num;
    int ins = *vms->snap->codes;

    if (ins >= ins_cmp && ins <= ins_cmp_group) {
        if (ins >= ins_cmp_multi && ins <= ins_ncmp_multi) {
            num = *(vms->snap->codes + 1);
            vms->snap->codes += (num * 3 + 2);
        } else {
            vms->snap->codes += 2;
        }
    }
    return 1;
}

_INLINE static bool
char_cmp(int chrcode_re, int chrcode, int flag) {
    if (flag & FLAG_IGNORECASE) {
        return (tolower(chrcode_re) == tolower(chrcode));
    } else {
        return (chrcode_re == chrcode);
    }
}

_INLINE static
int do_ins_cmp(VMState* vms) {
    int char_code = *(vms->snap->codes + 1);

#ifdef TRE_DEBUG
    printf_u8("INS_CMP ");
    putcode(char_code);
    printf(" ");
    putcode(vms->snap->chrcode);
    putchar('\n');
#endif
    if (vms->snap->text_end) return 0;
    if (char_cmp(char_code, vms->snap->chrcode, vms->flag))
        return 2;
    return 0;
}

_INLINE static bool
char_cmp_spe(int chrcode_re, int chrcode, int flag) {
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
    int char_code = *(vms->snap->codes + 1);

    TRE_DEBUG_PRINT("INS_CMP_SPE\n");

    if (vms->snap->text_end) return 0;
    if (char_cmp_spe(char_code, vms->snap->chrcode, vms->flag)) {
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
    int _type, _code;
    bool match = false;
    int* data = vms->snap->codes + 1;
    int num = *data++;

    TRE_DEBUG_PRINT("INS_CMP_MULTI\n");

    if (vms->snap->text_end) return 0;

    for (i = 0; i < num; i++) {
        _type = *((int*)data + i * 3);
        _code = *((int*)data + i * 3 + 1);

        if (_type == TK_CHAR) {
            if (char_cmp(_code, vms->snap->chrcode, vms->flag)) {
                match = true;
                break;
            }
        } else if (_type == TK_SPE_CHAR) {
            if (char_cmp_spe(_code, vms->snap->chrcode, vms->flag)) {
                match = true;
                break;
            }
        } else if (_type == '-') {
            if (char_cmp_range(_code, *((int*)data + i * 3 + 2), vms->snap->chrcode, vms->flag)) {
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
        int *p = vms->match_results[index].head;
        int *tail = vms->match_results[index].tail;
        int *p2 = vms->snap->str_pos;

        while (p < tail) {
            if (!char_cmp(*p, *p2, vms->flag)) return 0;
            p++;
            p2++;
        }

        // 匹配完成后将被+1，故先减去
        vms->snap->str_pos = p2-1;
        return 2;
    }

    return 0;
}

_INLINE static
void save_snap(VMState* vms) {
    TRE_DEBUG_PRINT("INS_SAVE_SNAP\n");
    vms->snap->prev = snap_dup(vms->snap);
}

_INLINE static
int do_ins_jmp(VMState* vms) {
    int offset = *(vms->snap->codes + 1);
    TRE_DEBUG_PRINT("INS_JMP\n");
    vms->snap->codes = vms->groups[vms->snap->cur_group].codes + (offset / sizeof(int)) + 2;
    return 1;
}

_INLINE static
int do_ins_cmp_group(VMState* vms) {
    RunCache *rc;
    int index = *(vms->snap->codes + 1);
    MatchGroup* g = vms->groups + index;

#ifdef TRE_DEBUG
    printf("INS_CMP_GROUP %d\n", index);
#endif

    // works for special groups (?=) (?!) (?<=) (?<!)
    if (g->type == GT_IF_MATCH) {
        vms->input_cache[index - vms->group_num] = vms->snap->str_pos;
    } else if (g->type == GT_IF_NOT_MATCH || g->type == GT_IF_NOT_PRECEDED_BY) {
        if (vms->snap->mr.enable == 1) {
            save_snap(vms);
        } else {
            vms->snap->codes += 2;
            save_snap(vms);
            vms->snap->codes -= 2;
        }

        if (g->type == GT_IF_NOT_PRECEDED_BY) {
            // current matched length less than group's length
            if (vms->snap->str_pos - vms->input_str < g->extra) {
                return 0;
            }
            vms->snap->str_pos = vms->snap->str_pos - g->extra;
            vms->snap->chrcode = *vms->snap->str_pos;
        }
    } else if (g->type == GT_IF_PRECEDED_BY) {
        // current matched length less than group's length
        if (vms->snap->str_pos - vms->input_str < g->extra) {
            return 0;
        }
        vms->input_cache[index - vms->group_num] = vms->snap->str_pos;
        vms->snap->str_pos = vms->snap->str_pos - g->extra;
        vms->snap->chrcode = *vms->snap->str_pos;
    }

    // new cache
    rc = _new(RunCache, 1);
    rc->codes_cache = vms->snap->codes;
    rc->prev = vms->snap->run_cache;
    rc->mr = vms->snap->mr;
    rc->cur_group = vms->snap->cur_group;

    // load group code
    vms->snap->run_cache = rc;
    vms->snap->codes = g->codes;
    vms->snap->mr.enable = 0;
    vms->snap->cur_group = index;

    // code for conditional backref
    if (g->type == GT_BACKREF_CONDITIONAL) {
        if (vms->match_results[g->extra].head && vms->match_results[g->extra].tail) {
            vms->snap->codes += 2;
        }
    }
    // end

    // set match result, value of head
    if (index < vms->group_num_all) {
        vms->match_results[index].tmp = vms->snap->str_pos;
    }

    return 1;
}

_INLINE static
int do_ins_group_end(VMState* vms) {
    RunCache *rc;
    int index = *(vms->snap->codes + 1);
    if (index == -1) index = vms->snap->cur_group;
    MatchGroup* g = vms->groups + index;
    VMSnap* snap_tmp;

#ifdef TRE_DEBUG
    printf("INS_GROUP_END %d\n", *(vms->snap->codes + 1));
#endif

    // load cache
    rc = vms->snap->run_cache;
    if (rc) {
        vms->snap->codes = rc->codes_cache;
        vms->snap->run_cache = rc->prev;
        vms->snap->mr = rc->mr;
        vms->snap->cur_group = rc->cur_group;
        free(rc);
    }

    // works for special groups (?=) (?!) (?<=) (?<!)
    if (g->type == GT_IF_MATCH || g->type == GT_IF_PRECEDED_BY) {
        vms->snap->str_pos = vms->input_cache[index - vms->group_num];
        vms->snap->chrcode = *vms->snap->str_pos;
    } else if (g->type == GT_IF_NOT_MATCH || g->type == GT_IF_NOT_PRECEDED_BY) {
        int* next_ins = vms->snap->codes + 2;
        if (vms->snap->mr.enable == 1) next_ins -= 2;
        do {
            snap_tmp = vms->snap;
            vms->snap = vms->snap->prev;
            free(snap_tmp);
        } while (vms->snap->codes != next_ins);
        return 0;
    }

    // set match result
    if (index < vms->group_num_all) {
        vms->match_results[index].head = vms->match_results[index].tmp;
        vms->match_results[index].tail = vms->snap->str_pos;
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
    if (vms->snap->prev) {
        bool greed;
        VMSnap* tmp;

        if (vms->backtrack_limit && vms->backtrack_num >= vms->backtrack_limit) {
            return -2;
        }
        vms->backtrack_num += 1;

        tmp = vms->snap;
        vms->snap = vms->snap->prev;
        snap_free(tmp);
        greed = vms->snap->mr.enable == 1 ? true : false;

        if (greed) TRE_DEBUG_PRINT("INS_BACKTRACK\n");
        else TRE_DEBUG_PRINT("INS_BACKTRACK_NG\n");

        if (greed) {
            vms->snap->mr.enable = 0;
            return jump_one_cmp(vms);
        }
        return 1;
    }
    TRE_DEBUG_PRINT("INS_BACKTRACK\n");
    return 0;
}

_INLINE static
int do_ins_checkpoint(VMState* vms, bool greed) {
    int llimit = *(vms->snap->codes + 1);
    int rlimit = *(vms->snap->codes + 2);

    if (greed) TRE_DEBUG_PRINT("INS_CHECK_POINT\n");
    else TRE_DEBUG_PRINT("INS_CHECK_POINT_NG\n");

    vms->snap->codes += 3;

    // a{0,0}
    if (rlimit == 0) {
        return jump_one_cmp(vms);
    }

    // a{2,1}
    if (rlimit != -1 && llimit > rlimit) {
        // sre_constants.error: bad repeat interval
        return -2;
    }

    vms->snap->mr.enable = greed ? 1 : 2;
    vms->snap->mr.llimit = llimit;
    vms->snap->mr.rlimit = rlimit;
    vms->snap->mr.cur_repeat = 0;

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
    int* tmp;
    tmp = vms->snap->codes;
    // group start + offset + length of group_end
    vms->snap->codes = vms->groups[vms->snap->cur_group].codes + (*(vms->snap->codes + 1) / sizeof(int)) + 2;
    save_snap(vms);
    vms->snap->codes = tmp + 2;
    return 2;
}


int vm_step(VMState* vms) {
    int ret;
    int cur_ins = *vms->snap->codes;
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
            group_cache = vms->snap->cur_group;
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
                if (vms->snap->mr.enable) {
                    bool greed = vms->snap->mr.enable == 1 ? true : false;
                    vms->snap->mr.cur_repeat++;

                    if (greed) {
                        // 贪婪模式 达到左边界，有资格保存回溯
                        if (vms->snap->mr.cur_repeat >= vms->snap->mr.llimit && vms->snap->mr.llimit != vms->snap->mr.rlimit) {
                            save_snap(vms);
                        }
                        // 达到右边界，越过匹配指令，跳出循环
                        if (vms->snap->mr.cur_repeat == vms->snap->mr.rlimit) {
                            vms->snap->mr.enable = 0;
                            vms->snap->codes += ret;
                        }
                        // 特别的，匹配空串的组被强制跳过
                        if ((cur_ins == ins_group_end) && (vms->snap->mr.cur_repeat >= vms->snap->mr.llimit) &&
                                (vms->snap->mr.cur_repeat != vms->snap->mr.rlimit)) {
                            // match "nothing" is no sense : (|)
                            if (vms->match_results[group_cache].head == vms->match_results[group_cache].tail) {
                                vms->snap->mr.enable = 0;
                                vms->snap->codes += 2;
                            }
                        }
                    } else {
                        if (vms->snap->mr.cur_repeat >= vms->snap->mr.llimit) {
                            if (((cur_ins != ins_group_end) && (vms->snap->mr.cur_repeat != vms->snap->mr.rlimit)) ||
                                ((cur_ins == ins_group_end) && (vms->snap->mr.cur_repeat != vms->snap->mr.rlimit) && 
                                 (vms->match_results[group_cache].head != vms->match_results[group_cache].tail))) {
                                save_snap(vms);
                            }
                            vms->snap->mr.enable = 0;
                            vms->snap->codes += ret;
                        }
                    }
                } else {
                    vms->snap->codes += ret;
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
        if (vms->snap->str_pos == vms->input_str || ((vms->flag & FLAG_MULTILINE) && (*(vms->snap->str_pos - 1) == '\n'))) {
            vms->snap->codes += 1;
            ret = 1;
        } else {
            ret = try_backtracking(vms);
        }
    } else if (cur_ins == ins_match_end) {
        // $
        TRE_DEBUG_PRINT("INS_MATCH_END\n");
        if (vms->snap->text_end || ((vms->flag & FLAG_MULTILINE) && (vms->snap->chrcode == '\n'))) {
            vms->snap->codes += 1;
            ret = 1;
        } else {
            ret = try_backtracking(vms);
        }
    }

    return ret;
}

int* u8str_to_u32str(const char* p, int* len) {
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
        vms->input_cache = _new(int*, groups_info->num_all - groups_info->num);
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
    vms->snap->run_cache = NULL;
    vms->snap->prev = NULL;
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
    RunCache *rc, *rc_tmp;

    for (snap = vms->snap; snap;) {
        for (rc = snap->run_cache; rc; ) {
            rc_tmp = rc;
            rc = rc->prev;
            free(rc_tmp);
        }

        snap_tmp = snap;
        snap = snap->prev;
        free(snap_tmp);
    }

    free(vms->match_results);
    free(vms);
}

