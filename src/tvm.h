
#ifndef TINYRE_VM_H
#define TINYRE_VM_H

#include "tinyre.h"

#define ins_cmp                   1
#define ins_cmp_spe               2
#define ins_cmp_multi             3
#define ins_ncmp_multi            4
#define ins_cmp_backref           5
#define ins_cmp_group             6

#define ins_check_point           20
#define ins_check_point_no_greed  21
#define ins_save_snap             22
#define ins_jmp                   23

#define ins_match_start           30
#define ins_match_end             31

#define ins_group_end             10


typedef struct MatchRepeat {
    // enable == 1 greedy match
    // enable == 2 non-greedy match
    short int enable;
    int cur_repeat;
    int llimit;
    int rlimit;
} MatchRepeat;

typedef struct RunCache {
    int cur_group;
    uint32_t* codes_cache;
    struct RunCache* prev;
    MatchRepeat mr;
} RunCache;


typedef struct GroupResultTemp {
    uint32_t* head;
    uint32_t* tail;
    uint32_t* tmp;
} GroupResultTemp;


// Ö´ÐÐ×´Ì¬

typedef struct VMSnap {
    uint32_t* codes;
    uint32_t* str_pos;
    uint32_t chrcode;
    int cur_group;
    bool text_end;
    MatchRepeat mr;
    tre_Stack run_cache;
    struct VMSnap* prev;
} VMSnap;

typedef struct VMState {
    const char* raw_input_str;
    int input_len;
    uint32_t* input_str;
    int group_num;
    int group_num_all;
    MatchGroup* groups;
    int flag;

    int backtrack_num;
    int backtrack_limit;
    uint32_t** input_cache;
    GroupResultTemp* match_results;
    VMSnap* snap;
    VMSnap* snap_used;
} VMState;

VMState* vm_init(tre_Pattern* groups, const char* input_str, int backtrack_limit);

int vm_step(VMState* vms);
void vm_free(VMState* vms);
tre_GroupResult* vm_exec(VMState* vms);

#endif
