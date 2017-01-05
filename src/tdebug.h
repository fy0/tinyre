
#ifndef TINYRETRE_DEBUG_H
#define TINYRETRE_DEBUG_H

#include "tlexer.h"

//#define TRE_DEBUG

struct tre_Token;
struct ParserMatchGroup;

void putcode(uint32_t code);
void output_str(uint32_t *str, int len);

void debug_token_print(tre_Lexer *lexer);
void debug_ins_list_print(struct ParserMatchGroup* groups);

void debug_printstr(uint32_t *str, int head, int tail);

#ifdef TRE_DEBUG
#define TRE_DEBUG_PRINT( ...) printf_u8(__VA_ARGS__)
#else
#define TRE_DEBUG_PRINT(expression, ...)
#endif

#endif

