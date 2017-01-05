
#ifndef TINYRE_VM_H
#define TINYRE_VM_H

#include "tinyre.h"
#include "tutils.h"

#define INS_CMP                   1
#define INS_CMP_SPE               2
#define INS_CMP_MULTI             3
#define INS_NCMP_MULTI            4
#define INS_CMP_BACKREF           5
#define INS_CMP_GROUP             6
#define INS_GROUP_END             7

#define INS_CHECK_POINT           8
#define INS_CHECK_POINT_NO_GREED  9
#define INS_SAVE_SNAP             10
#define INS_JMP                   11

#define INS_MATCH_START           12
#define INS_MATCH_END             13



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

//int vm_step(VMState* vms);
void vm_free(VMState* vms);
tre_GroupResult* vm_exec(VMState* vms);

#endif

