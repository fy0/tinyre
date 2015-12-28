
#ifndef TINYRE_LEXER_H
#define TINYRE_LEXER_H

#define FIRST_TOKEN    128

enum TOKEN_LIST {
    TK_CHAR = FIRST_TOKEN,
    TK_SPE_CHAR,
    TK_BACK_REF,
};

enum GROUP_TYPE {
    GT_NORMAL = 0,
    GT_NONGROUPING = 1,
    GT_IF_MATCH,
    GT_IF_NOT_MATCH,
    GT_IF_PRECEDED_BY,
    GT_IF_NOT_PRECEDED_BY,
    GT_BACKREF,
    GT_BACKREF_CONDITIONAL,
};

typedef struct tre_Token {
    int token;
    int code;
} tre_Token;

typedef struct TokenGroupName {
    tre_Token* tk;
    char* name;
    struct TokenGroupName *next;
} TokenGroupName;

typedef struct TokenInfo {
    tre_Token* tokens;
    int extra_flag;
    int max_normal_group_num;
    TokenGroupName* group_names;
} TokenInfo;

int tre_lexer(char* s, TokenInfo** ptki);
void token_info_free(TokenInfo* tki);

#define ERR_LEXER_UNBALANCED_PARENTHESIS        -3
#define ERR_LEXER_UNEXPECTED_END_OF_PATTERN     -4
#define ERR_LEXER_UNKNOW_SPECIFIER              -5
#define ERR_LEXER_BAD_GROUP_NAME                -6
#define ERR_LEXER_UNICODE_ESCAPE                -7
#define ERR_LEXER_UNICODE6_ESCAPE               -8
#define ERR_LEXER_HEX_ESCAPE                    -9
#define ERR_LEXER_BAD_GROUP_NAME_IN_BACKREF     -10
#define ERR_LEXER_INVALID_GROUP_NAME_OR_INDEX   -11
#define ERR_LEXER_REDEFINITION_OF_GROUP_NAME    -12
#endif
