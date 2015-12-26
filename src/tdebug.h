
#ifndef TINYRETRE_DEBUG_H
#define TINYRETRE_DEBUG_H

#define TRE_DEBUG

void debug_token_print(tre_Token* tokens, int len);
void debug_ins_list_print(ParserMatchGroup* groups);

void debug_printstr(int* head, int* tail);

#endif
