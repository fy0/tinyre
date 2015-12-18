
#ifndef TINYRE_DEBUG_H
#define TINYRE_DEBUG_H

void debug_token_print(tre_Token* tokens, int len);
void debug_ins_list_print(ParserMatchGroup* groups);

void debug_printstr(int* head, int* tail);

#endif
