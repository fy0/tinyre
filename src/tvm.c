
#include "tutils.h"
#include "tvm.h"
#include "tlexer.h"

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
            rc->prev = rc_src->prev ? _new(RunCache, 1) : NULL;
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

void vm_input_next(VMState* vms) {
    vms->snap->str_pos++;
    vms->snap->chrcode = *vms->snap->str_pos;
}

_INLINE static
int jump_one_cmp(VMState* vms) {
    int num;
    int ins = *vms->snap->codes;

    if (ins >= 1 && ins <= 5) {
        if (ins >= 3 && ins <= 4) {
            num = *(vms->snap->codes + 1);
            vms->snap->codes += (num * 2 + 2);
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

#ifdef _DEBUG
    printf_u8("INS_CMP ");
    putcode(char_code);
    putchar('\n');
#endif
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

    if (char_cmp_spe(char_code, vms->snap->chrcode, vms->flag)) {
        return 2;
    }
    return 0;
}

_INLINE static
int do_ins_cmp_multi(VMState* vms, bool is_ncmp) {
    int i;
    int _type, _code;
    bool match = false;
    int* data = vms->snap->codes + 1;
    int num = *data++;

    TRE_DEBUG_PRINT("INS_CMP_MULTI\n");

    for (i = 0; i < num; i++) {
        _type = *((int*)data + i * 2);
        _code = *((int*)data + i * 2 + 1);

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
        }
    }

    if (is_ncmp) {
        return match ? 0 : (num * 2 + 2);
    } else {
        return match ? (num * 2 + 2) : 0;
    }
}

_INLINE static
int do_ins_cmp_group(VMState* vms) {
    RunCache *rc;
    int index = *(vms->snap->codes + 1);
    MatchGroup* g = vms->groups + index;

    TRE_DEBUG_PRINT("INS_CMP_GROUP\n");

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

    // set match result, value of head
    vms->match_results[index].tmp = vms->snap->str_pos;

    return 1;
}

_INLINE static
int do_ins_group_end(VMState* vms) {
    RunCache *rc;
    int index = *(vms->snap->codes + 1);
    if (index == -1) index = vms->snap->cur_group;

    TRE_DEBUG_PRINT("INS_GROUP_END\n");

    // load cache
    rc = vms->snap->run_cache;
    if (rc) {
        vms->snap->codes = rc->codes_cache;
        vms->snap->run_cache = rc->prev;
        vms->snap->mr = rc->mr;
        vms->snap->cur_group = rc->cur_group;
        free(rc);
    }

    // set match result
    vms->match_results[index].head = vms->match_results[index].tmp;
    vms->match_results[index].tail = vms->snap->str_pos;

    // end if GROUP(0) matched
    // 2 is length of CMP_GROUP
    return (index == 0) ? -1 : 2;
}

_INLINE static
void save_snap(VMState* vms) {
    TRE_DEBUG_PRINT("INS_SAVE_SNAP\n");
    vms->snap->prev = snap_dup(vms->snap);
}

_INLINE static
int try_backtracking(VMState* vms) {
    if (vms->snap->prev) {
        bool greed;
        vms->snap = vms->snap->prev;
        greed = vms->snap->mr.enable == 1 ? true : false;

        if (greed) TRE_DEBUG_PRINT("INS_BACKTRACK\n");
        else TRE_DEBUG_PRINT("INS_BACKTRACK_NG\n");

        if (greed) {
            vms->snap->mr.enable = 0;
            return jump_one_cmp(vms);
        }
        return 1;
    }
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
        } else if (cur_ins == ins_cmp_group) {
            ret = do_ins_cmp_group(vms);
        } else if (cur_ins == ins_group_end) {
            group_cache = vms->snap->cur_group;
            ret = do_ins_group_end(vms);
        }

        if (ret) {
            if (cur_ins < ins_cmp_group) vm_input_next(vms);
            // CMP_GROUP is very different,
            // It's begin of match, and end at GROUP_END.
            // Other CMPs is both begin and end of one match.
            if (cur_ins != ins_cmp_group) {
                // match again when a? a+ a* a{x,y}
                if (vms->snap->mr.enable) {
                    bool greed = vms->snap->mr.enable == 1 ? true : false;
                    vms->snap->mr.cur_repeat++;

                    if (greed) {
                        if (vms->snap->mr.cur_repeat >= vms->snap->mr.llimit) {
                            save_snap(vms);
                        }
                        if (vms->snap->mr.cur_repeat == vms->snap->mr.rlimit) {
                            vms->snap->mr.enable = 0;
                            vms->snap->codes += ret;
                        }
                        if (cur_ins == ins_group_end && vms->snap->mr.cur_repeat >= vms->snap->mr.llimit && vms->snap->mr.cur_repeat != vms->snap->mr.rlimit) {
                            // match "nothing" is no sense : (|)
                            if (vms->match_results[group_cache].head == vms->match_results[group_cache].tail) {
                                vms->snap->mr.enable = 0;
                                vms->snap->codes += 2;
                            }
                        }
                    } else {
                        if (vms->snap->mr.cur_repeat >= vms->snap->mr.llimit) {
                            if (vms->snap->mr.cur_repeat != vms->snap->mr.rlimit && 
                                vms->match_results[group_cache].head != vms->match_results[group_cache].tail)
                                save_snap(vms);
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
    } else if (cur_ins == ins_match_start) {
        // ^
        if (vms->snap->str_pos != vms->input_str) {
            ret = try_backtracking(vms);
        } else {
            vms->snap->codes += 1;
            ret = 1;
        }
    } else if (cur_ins == ins_match_end) {
        // $
        if (vms->snap->chrcode != 0) {
            ret = try_backtracking(vms);
        } else {
            vms->snap->codes += 1;
            ret = 1;
        }
    }

    return ret;
}

int* u8str_to_u32str(const char* p) {
    int *ret, *p2;
    int i, code, slen = utf8_len(p);

    ret = p2 = _new(int, slen + 1);

    for (i = 0; i < slen; i++) {
        p = utf8_decode(p, &code);
        *p2++ = code;
    }

    *p2 = 0;
    return ret;
}

VMState* vm_init(tre_Pattern* groups_info, const char* input_str) {
    VMState* vms = _new(VMState, 1);
    vms->raw_input_str = input_str;
    vms->input_str = u8str_to_u32str(input_str);

    vms->group_num = groups_info->num;
    vms->groups = groups_info->groups;
    vms->flag = groups_info->flag;

    // init match results of groups
    vms->match_results = _new(GroupResultTemp, groups_info->num);
    memset(vms->match_results, 0, sizeof(GroupResultTemp) * groups_info->num);
    vms->match_results[0].tmp = vms->input_str;

    // init first snap
    vms->snap = _new(VMSnap, 1);
    vms->snap->codes = groups_info->groups[0].codes;
    vms->snap->str_pos = vms->input_str;
    vms->snap->chrcode = *vms->input_str;
    vms->snap->cur_group = 0;
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
                results[i].head = vms->match_results[i].head;
                results[i].tail = vms->match_results[i].tail;
                results[i].name = vms->groups[i].name;
            }
        }
        return results;
    }

    return NULL;
}

void vm_free(VMState* vms)
{
    free(vms->match_results);
    free(vms);
}