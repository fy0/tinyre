
#ifndef TINYRE_PARSER_H
#define TINYRE_PARSER_H

#include "tinyre.h"
#include "tlexer.h"

typedef struct INS_List {
    int len;
    int ins;
    void* data;
    struct INS_List* next;
} INS_List;

typedef struct OR_List {
    INS_List* codes;
    struct OR_List* next;
} OR_List;

typedef struct ParserMatchGroup {
    char* name;
    INS_List* codes;
    INS_List* codes_start;
    int group_type;
    int group_extra; // used by (?<=) (?<!)
    int or_num;
    OR_List* or_list;
    struct ParserMatchGroup* next;
} ParserMatchGroup;

tre_Pattern* compact_group(ParserMatchGroup* parser_groups);
tre_Pattern* tre_parser(TokenInfo* _tki, tre_Token** last_token);

int tre_last_parser_error();

// look-behind requires fixed-width pattern
#define ERR_PARSER_REQUIRES_FIXED_WIDTH_PATTERN   -11
// bad character range
#define ERR_PARSER_BAD_CHARACTER_RANGE            -12
// nothing to repeat
#define ERR_PARSER_NOTHING_TO_REPEAT              -13
// impossible token
#define ERR_PARSER_IMPOSSIBLE_TOKEN               -14
// unknow group name
#define ERR_PARSER_UNKNOWN_GROUP_NAME             -15
// conditional backref with more than two branches
#define ERR_PARSER_CONDITIONAL_BACKREF            -16

#endif
