
#ifndef TINYRE_PARSER_H
#define TINYRE_PARSER_H

#include "tinyre.h"
#include "tlexer.h"

typedef struct INS_List {
    int len;
    uint32_t ins;
    uint32_t* data;
    struct INS_List* next;
} INS_List;

typedef struct OR_List {
    INS_List* codes;
    struct OR_List* next;
} OR_List;

typedef struct ParserMatchGroup {
    uint32_t* name;
    int name_len;

    INS_List* codes;
    INS_List* codes_start;
    int group_type;
    int group_extra; // used by (?<=) (?<!)
    int or_num;
    OR_List* or_list;
    struct ParserMatchGroup* next;
} ParserMatchGroup;

typedef struct tre_Parser {
    tre_Lexer *lex;
    int error_code;
    int avaliable_group;

    ParserMatchGroup* m_start;
    ParserMatchGroup* m_cur;

    bool is_count_width;
    int match_width;
} tre_Parser;

tre_Pattern* compact_group(ParserMatchGroup* parser_groups);
tre_Pattern* tre_parser(tre_Lexer *lexer,int* perror_code);

// look-behind requires fixed-width pattern
#define ERR_PARSER_REQUIRES_FIXED_WIDTH_PATTERN   -51
// bad character range
#define ERR_PARSER_BAD_CHARACTER_RANGE            -52
// nothing to repeat
#define ERR_PARSER_NOTHING_TO_REPEAT              -53
// impossible token
#define ERR_PARSER_IMPOSSIBLE_TOKEN               -54
// unknow group name
#define ERR_PARSER_UNKNOWN_GROUP_NAME             -55
// conditional backref with more than two branches
#define ERR_PARSER_CONDITIONAL_BACKREF            -56
// invalid group index in conditional backref
#define ERR_PARSER_INVALID_GROUP_INDEX            -57

#endif
