
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
    int or_num;
    OR_List* or_list;
    struct ParserMatchGroup* next;
} ParserMatchGroup;

tre_Pattern* compact_group(ParserMatchGroup* parser_groups);
tre_Pattern* tre_parser(TokenInfo* _tki, tre_Token** last_token);

#endif
