
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

typedef struct tre_TokenGroupName {
    tre_Token* tk;
    char* name;
    struct tre_TokenGroupName *next;
} tre_TokenGroupName;

int tre_lexer(char* s, tre_Token** ppt, int* p_extra_flag, tre_TokenGroupName** p_group_names);

#define ERR_LEXER_UNBALANCED_PARENTHESIS        -3
#define ERR_LEXER_UNEXPECTED_END_OF_PATTERN     -4
#define ERR_LEXER_UNKNOW_SPECIFIER              -5
#define ERR_LEXER_BAD_GROUP_NAME                -6

#endif
