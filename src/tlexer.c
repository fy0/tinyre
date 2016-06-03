
#include "tlexer.h"
#include "tutils.h"

uint32_t char_next(tre_Lexer *lex) {
    return lex->s[lex->scur++];
}

uint32_t char_nextn(tre_Lexer *lex, int n) {
    return lex->s[lex->scur+=n];
}

uint32_t char_lookahead(tre_Lexer *lex) {
    return lex->s[lex->scur+1];
}

_INLINE static
bool token_check(uint32_t code) {
    switch (code) {
        case '^': case '$': case '*': case '+': case '?':
        case '[': case ']': case '{': case '(': case ')':
        case '|':
            return true;
    }
    return false;
}

_INLINE static
bool is_spe_char(uint32_t code) {
    const char other_tokens[] = "DdWwSs";
    for (const char *p = other_tokens; *p; p++) {
        if (code == *p) return true;
    }
    return false;
}

_INLINE static
int try_get_escape(uint32_t code) {
    const char other_tokens[] = "abfnrtv";
    const int other_codes[] = { 7, 8, 12, 10, 13, 9, 11 };
    for (const char *p = other_tokens; *p; p++) {
        if (code == *p) return other_codes[p-other_tokens];
    }
    return code;
}


_INLINE static uint8_t _hex(uint32_t code) {
    if (code >= '0' && code <= '9') return code - '0';
    else if (code >= 'A' && code <= 'F') return code - 'A' + 10;
    else if (code >= 'a' && code <= 'f') return code - 'a' + 10;
    return 255;
}

_INLINE static uint8_t _oct(uint32_t code) {
    if (code >= '0' && code <= '7') return code - '0';
    return 255;
}

_INLINE static uint8_t _bin(uint32_t code) {
    if (code >= '0' && code <= '1') return code - '0';
    return 255;
}

_INLINE static uint8_t _dec(uint32_t code) {
    if (code >= '0' && code <= '9') return code - '0';
    return 255;
}


_INLINE static
int _read_x_int(const uint32_t *start, const uint32_t *end, int n, uint8_t(*func)(uint32_t code), int max_size) {
    const uint32_t *p = start;
    const uint32_t *e = (max_size > 0) ? start + max_size : end;
    int ret = 0, val = (int)pow(n, e - p - 1);

    do {
        ret += (*func)(*p++) * val;
        val /= n;
    } while (p != e);

    return ret;
}


_INLINE static
int token_char_accept(tre_Lexer *lex, uint32_t code, bool use_back_ref) {
    if (code == '\\') {
        // 对转义字符做特殊处理
        if (lex->scur == lex->slen) {
            // 如果已经是最后一个字符，那么当作普通字符即可
            lex->token.extra.code = code;
            lex->token.value = TK_CHAR;
        } else {
            // 如果不是，读下一个字符
            code = char_next(lex);
            if (is_spe_char(code)) {
                // 能确定为特殊匹配字符的话，读取结束
                lex->token.extra.code = code;
                lex->token.value = TK_CHAR_SPE;
            } else {
                // 否则当做 hex/unicode 转义处理
                // ... 麻烦得很，暂时先报个错结束
                return ERR_LEXER_HEX_ESCAPE;
                /*if (code == 'x') {

                }*/
            }
        }
    } else {
        // 若非转义字符，那么一切都很简单
        lex->token.extra.code = code;
        lex->token.value = TK_CHAR;
    }
    return 0;
}

_INLINE static
int char_to_flag(uint32_t code) {
    if (code == 'i') return FLAG_IGNORECASE;
    else if (code == 'm') return FLAG_MULTILINE;
    else if (code == 's') return FLAG_DOTALL;
    return 0;
}

#define lex_isidentfirst(c) ((c >= 'A' && c<= 'Z') || (c >= 'a' && c<= 'z') || (c >= '_') || (c >= 128))
#define lex_isidentletter(c) ((c >= 'A' && c<= 'Z') || (c >= 'a' && c<= 'z') || (c >= '0' && c<= '9') || (c == '_') || (c >= 128))

uint32_t* read_group_name(tre_Lexer *lex, char end_terminal, int *plen) {
    uint32_t code;
    uint32_t *name;
    const uint32_t *p = lex->s + lex->scur;
    const uint32_t *start = p;


    code = *p++;
    if (!lex_isidentfirst(code)) return NULL;

    while (true) {
        if (!lex_isidentletter(code)) break;
        code = *p++;
    }

    if (code != end_terminal) {
        return NULL;
    }

    name = _new(uint32_t, p - start);
    memcpy(name, start, p - start - 1);
    name[p - start - 1] = '\0';

    if (plen) *plen = p - start - 1;
    return name;
}

int read_int(tre_Lexer *lex, char end_terminal, int *plen) {
    const uint32_t *p = lex->s + lex->scur;
    const uint32_t *start = p;

    while (isdigit(*p)) ++p;

    int count = _read_x_int(start, p, 10, _dec, 0);
    if (plen) {
        if (end_terminal && (*(p + *plen) != end_terminal)) {
            return -1;
        }
    }
    if (plen) *plen = p - start;
    return count;
}

int tre_lexer_next(tre_Lexer* lex) {
    int len;
    uint32_t code;
    uint32_t* name;
    if (lex->scur == lex->slen) {
        lex->token.value = TK_END;
        return 0;
    }
    code = char_next(lex);
    bool is_lastone = (lex->scur == lex->slen);

    switch (lex->state) {
        case 0: // NORMAL STATE
            if (token_check(code)) {
                lex->token.extra.code = 0;
                lex->token.value = code; // token val is it's own ascii.

                switch (code) {
                    case '[':
                        lex->state = 1;
                        if ((!is_lastone) && char_lookahead(lex) == '^') {
                            lex->token.extra.code = 1;
                        }
                        break;
                    case '{': {
                        int count;
                        int scur_bak = lex->scur;
                        int llimit = 0, rlimit = -1;

                        // read left limit a{1
                        llimit = read_int(lex, 0, &count);
                        code = char_nextn(lex, count);

                        // read comma a{1,
                        if ((char)code == ',') {
                            code = char_next(lex);
                        } else if ((char)code == '}') {
                            rlimit = llimit;
                            char_next(lex);
                            goto __write_code;
                        } else {
                            // falied, rollback
                            goto __bad_token;
                        }

                        // read left limit a{1, 2
                        rlimit = read_int(lex, 0, &count);
                        code = char_nextn(lex, count);

                        // read right brace a{1,2} or a{1,}
                        if ((char)code == '}') {
                            // ok, rlimit is -1
                        } else {
                            // falied, rollback
                            goto __bad_token;
                        }

                    __write_code:
                        lex->token.extra.code = llimit;
                        lex->token.extra.code2 = rlimit;
                        break;

                    __bad_token:
                        lex->token.value = TK_CHAR;
                        lex->token.extra.code = '{';
                        lex->scur = scur_bak;
                        break;
                    }
                    case '(': {
                        code = char_lookahead(lex);
                        // if next char is not ?
                        if (code != '?') {
                            lex->token.extra.group_type = GT_NORMAL;
                            lex->max_normal_group_num++;
                            break;
                        } else {
                            code = char_nextn(lex, 2);
                            switch (code) {
                                case '#': { // just comment
                                    bool is_escape = false;
                                    code = char_next(lex);
                                    while (!(!is_escape && code == ')')) {
                                        code = char_next(lex);
                                        if (is_escape) is_escape = false;
                                        if (code == '\\') is_escape = true;
                                        if (code == '\0') return ERR_LEXER_UNBALANCED_PARENTHESIS;
                                    }
                                    lex->token.value = TK_COMMENT;
                                    break;
                                }
                                case ':': lex->token.extra.group_type = GT_NONGROUPING; break;
                                case '=': lex->token.extra.group_type = GT_IF_MATCH; break;
                                case '!': lex->token.extra.group_type = GT_IF_NOT_MATCH; break;
                                case '(':
                                    // code for conditional backref
                                    name = read_group_name(lex, ')', &len);
                                    if (name) {
                                        lex->token.extra.group_type = GT_BACKREF_CONDITIONAL_GROUPNAME;
                                        lex->token.extra.group_name = name;
                                        lex->token.extra.group_name_len = len;
                                    } else {
                                        int i = read_int(lex, ')', &len);
                                        if (i == -1) {
                                            return ERR_LEXER_INVALID_GROUP_NAME_OR_INDEX;
                                        } else {
                                            lex->token.extra.group_type = GT_BACKREF_CONDITIONAL_INDEX;
                                            lex->token.extra.index = i;
                                        }
                                    }
                                    break;
                                case 'P':
                                    code = char_next(lex);
                                    // group name
                                    if (code == '<') {
                                        name = read_group_name(lex, '>', &len);
                                        if (!name) return ERR_LEXER_BAD_GROUP_NAME;
                                        lex->token.extra.group_name = name;
                                        lex->token.extra.group_name_len = len;
                                        lex->max_normal_group_num++;
                                    } else if (code == '=') {
                                        // code for back reference (?P=)
                                        name = read_group_name(lex, ')', &len);
                                        if (!name) return ERR_LEXER_BAD_GROUP_NAME_IN_BACKREF;

                                        lex->token.value = TK_BACK_REF;
                                        lex->token.extra.group_type = 2;
                                        lex->token.extra.group_name = name;
                                        lex->token.extra.group_name_len = len;
                                    } else {
                                        return ERR_LEXER_UNKNOW_SPECIFIER;
                                    }
                                    break;
                                case '<':
                                    code = char_next(lex);
                                    if (code == '=') {
                                        lex->token.extra.group_type = GT_IF_PRECEDED_BY;
                                    } else if (code == '!') {
                                        lex->token.extra.group_type = GT_IF_NOT_PRECEDED_BY;
                                    } else {
                                        return ERR_LEXER_UNKNOW_SPECIFIER;
                                    }
                                    break;
                                default:
                                    if (char_to_flag(code)) {
                                        int flag = 0;
                                        while (true) {
                                            flag = char_to_flag(code);
                                            if (flag) lex->extra_flag |= flag;
                                            else break;
                                            code = char_next(lex);
                                        }
                                    } else {
                                        return ERR_LEXER_UNEXPECTED_END_OF_PATTERN;
                                    }
                            }
                        }
                    }
                };
            } else {
                int ret = token_char_accept(lex, code, true);
                if (ret) return ret;
            }
            break;
        case 1: { // [...]
            bool is_escape = code == '\\';
            if (code == ']') {
                lex->state = 0;
                lex->token.value = ']';
                break;
            }
            int ret = token_char_accept(lex, code, false);
            if (ret) return ret;

            // TODO: 为 a-z 语法做处理
            break;
        }
    }
    return 0;
}

tre_Lexer* tre_lexer_new(uint32_t* s, int len) {
    tre_Lexer* lex = _new(tre_Lexer, 1);
    lex->extra_flag = 0;
    lex->max_normal_group_num = 1;
    lex->group_names = NULL;
    lex->state = 0;

    if (s) {
        lex->s = s;
        lex->scur = 0;
        lex->slen = len;
    }
    return lex;
}
