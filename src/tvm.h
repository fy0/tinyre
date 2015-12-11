
#ifndef TINYRE_VM_H
#define TINYRE_VM_H

#include "tinyre.h"

#define ins_cmp           1
#define ins_cmp_spe       2
#define ins_cmp_multi     3
#define ins_ncmp_multi    4
#define ins_cmp_group     5

#define ins_check_point  20

#define ins_match_start  30 // need a better name
#define ins_match_end    31

#define ins_group_end    10

typedef struct MatchRepeat {
	bool enable;
	int cur_repeat;
	int llimit;
	int rlimit;
} MatchRepeat;

typedef struct RunCache {
	int* codes_cache;
	struct RunCache* prev;
	MatchRepeat mr;
} RunCache;


// Ö´ÐÐ×´Ì¬

typedef struct VMSnap {
	int last_chrcode;
	const char* last_pos;
	const char* str_pos;
	RunCache* run_cache;
	int* codes;
	MatchRepeat mr;
	struct VMSnap* prev;
} VMSnap;

typedef struct VMState {
	const char* input_str;
	int group_num;
	tre_group* match_results;
	MatchGroup* groups;

	VMSnap* snap;
} VMState;

VMState* vm_init(tre_Pattern* groups, const char* input_str);

int vm_step(VMState* vms);
tre_group* vm_exec(VMState* vms);

#endif
