
#ifndef TINYRE_PARSER_H
#define TINYRE_PARSER_H

#include "tinyre.h"

typedef struct INS_List {
	int len;
	int ins;
	void* data;
	struct INS_List* next;
} INS_List;

typedef struct ParserMatchGroup {
	char* name;
	INS_List* codes;
	INS_List* codes_start;
	struct ParserMatchGroup* next;
} ParserMatchGroup;

tre_Pattern* compact_group(ParserMatchGroup* parser_groups);
tre_Pattern* tre_parser(tre_Token* tk, tre_Token** last_token);

#endif
