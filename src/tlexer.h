
#ifndef TINYRE_LEXER_H
#define TINYRE_LEXER_H

#define FIRST_TOKEN    128

enum TOKEN_LIST {
    TK_CHAR = FIRST_TOKEN,
    TK_SPE_CHAR,
};

typedef struct tre_Token {
    int token;
    int code;
} tre_Token;

int tre_lexer(char* s, tre_Token** ppt);

#endif
