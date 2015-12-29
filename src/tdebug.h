
#ifndef TINYRETRE_DEBUG_H
#define TINYRETRE_DEBUG_H

//#define TRE_DEBUG

struct tre_Token;
struct ParserMatchGroup;

void debug_token_print(struct tre_Token* tokens, int len);
void debug_ins_list_print(struct ParserMatchGroup* groups);

void debug_printstr(int* str, int head, int tail);

#ifdef TRE_DEBUG
#define TRE_DEBUG_PRINT(str) printf_u8(str)
#else
#define TRE_DEBUG_PRINT(expression)
#endif

#endif
