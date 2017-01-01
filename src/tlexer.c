
#include "tlexer.h"
#include "tutils.h"

int read_int(tre_Lexer *lex, char end_terminal, int *plen);
int read_hex(tre_Lexer *lex, int len, bool *p_isok);

uint32_t char_next(tre_Lexer *lex) {
    return lex->s[lex->scur++];
}

uint32_t char_nextn(tre_Lexer *lex, int n) {
    uint32_t code = lex->s[lex->scur+n-1];
    lex->scur+=n;
    return code;
}

uint32_t char_lookahead(tre_Lexer *lex) {
    return lex->s[lex->scur];
}

uint32_t char_lookaheadn(tre_Lexer *lex, int n) {
    return lex->s[lex->scur + n - 1];
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

int read_int(tre_Lexer *lex, char end_terminal, int *plen) {
    const uint32_t *p = lex->s + lex->scur;
    const uint32_t *start = p;

    while (isdigit(*p)) ++p;

    if (p == start) {
        if (plen) *plen = 0;
        return -1;
    }

    int num = _read_x_int(start, p, 10, _dec, 0);
    if (plen) {
        if (end_terminal && (*(p + (p - start) - 1) != end_terminal)) {
            return -1;
        }
    }
    if (plen) *plen = p - start;
    return num;
}

int read_hex(tre_Lexer *lex, int len, bool *p_isok) {
    const uint32_t *p = lex->s + lex->scur;
    const uint32_t *start = p;
    int count = 0;

    while (_hex(*p) != 255) {
        ++p;
        if (++count == len) break;
    }

    if (count != len) {
        p_isok = false;
        return 0;
    }

    *p_isok = true;
    return _read_x_int(start, p, 16, _hex, 0);
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
            code = char_lookahead(lex);
            if (is_spe_char(code)) {
                // 能确定为特殊匹配字符的话，读取结束
                lex->token.extra.code = code;
                lex->token.value = TK_CHAR_SPE;
                code = char_next(lex);
            } else {
                // 否则当做 hex/unicode 转义处理
                int num, len;
                bool is_ok = false;

                if (code == 'x') {
                    code = char_next(lex);
                    num = read_hex(lex, 2, &is_ok);
                    if (!is_ok) return ERR_LEXER_HEX_ESCAPE;
                    char_nextn(lex, 2);
                } else if (code == 'u') {
                    code = char_next(lex);
                    num = read_hex(lex, 4, &is_ok);
                    if (!is_ok) return ERR_LEXER_UNICODE_ESCAPE;
                    char_nextn(lex, 4);
                } else if (code == 'U') {
                    code = char_next(lex);
                    num = read_hex(lex, 8, &is_ok); // unicode 6.0 \U0000000A
                    if (!is_ok) return ERR_LEXER_UNICODE6_ESCAPE;
                    char_nextn(lex, 8);
                }

                if (is_ok) {
                    lex->token.value = TK_CHAR;
                    lex->token.extra.code = num;
                } else {
                    num = read_int(lex, 0, &len);
                    if (num != -1) {
                        // back reference or normal char
                        if (use_back_ref) {
                            if (num == 0) {
                                lex->token.value = TK_CHAR;
                                lex->token.extra.code = 0;
                            } else {
                                lex->token.value = TK_BACK_REF;
                                lex->token.extra.index = num;
                            }
                        } else {
                            lex->token.value = TK_CHAR;
                            lex->token.extra.code = num;
                        }
                        char_nextn(lex, len);
                    } else {
                        // 既不是转义，也不是前向引用，只是一个字符罢了
                        lex->token.value = TK_CHAR;
                        lex->token.extra.code = code;
                        char_next(lex);
                    }
                }
            }
        }
    } else {
        // 若非转义字符，那么一切都很简单
        lex->token.extra.code = code;
        lex->token.value = (code == '.') ? TK_CHAR_SPE : TK_CHAR;
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

    name = tre_new(uint32_t, p - start);
    memcpy(name, start, (p - start) * sizeof(uint32_t));
    name[p - start - 1] = '\0';

    if (plen) *plen = p - start - 1;
    return name;
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
                        if (count == 0) goto __bad_token;
                        code = char_nextn(lex, count+1);

                        // read comma a{1,
                        if ((char)code == ',') {
                            //char_next(lex);
                        } else if ((char)code == '}') {
                            rlimit = llimit;
                            goto __write_code;
                        } else {
                            // falied, rollback
                            goto __bad_token;
                        }

                        // read left limit a{1, 2
                        rlimit = read_int(lex, 0, &count);
                        code = char_nextn(lex, count+1);

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
                            lex->token.extra.group_name = NULL;
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
                                        code = char_nextn(lex, len);
                                        lex->token.extra.group_type = GT_BACKREF_CONDITIONAL_GROUPNAME;
                                        lex->token.extra.group_name = name;
                                        lex->token.extra.group_name_len = len;
                                    } else {
                                        int i = read_int(lex, ')', &len);
                                        if (i == -1) {
                                            return ERR_LEXER_INVALID_GROUP_NAME_OR_INDEX;
                                        } else {
                                            code = char_nextn(lex, len);
                                            lex->token.extra.group_type = GT_BACKREF_CONDITIONAL_INDEX;
                                            lex->token.extra.index = i;
                                        }
                                    }
                                    code = char_next(lex);
                                    break;
                                case 'P':
                                    // group name
                                    code = char_lookahead(lex);
                                    if (code == '<') {
                                        code = char_next(lex);
                                        name = read_group_name(lex, '>', &len);
                                        if (!name) return ERR_LEXER_BAD_GROUP_NAME;
                                        code = char_nextn(lex, len+1); // name and '>'

                                        lex->token.extra.group_type = GT_NORMAL;
                                        lex->token.extra.group_name = name;
                                        lex->token.extra.group_name_len = len;
                                    } else if (code == '=') {
                                        // code for back reference (?P=)
                                        code = char_next(lex);
                                        name = read_group_name(lex, ')', &len);
                                        if (!name) return ERR_LEXER_BAD_GROUP_NAME_IN_BACKREF;
                                        code = char_nextn(lex, len); // skip name

                                        lex->token.extra.group_type = GT_BACKREF;
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
                                    lex->token.value = TK_NOP;
                                    break;
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
            int ret = token_char_accept(lex, code, false);
            if (ret) return ret;

            if (!is_escape && lex->token.value == TK_CHAR) {
                // end the state
                if (code == ']') {
                    lex->state = 0;
                    lex->token.value = ']';
                    break;
                }
            }

            // [a-z] grammar
            code = char_lookahead(lex);
            if (code == '-') {
                uint32_t code2 = char_lookaheadn(lex, 2);
                // [a-]
                if (code2 == ']') break;

                // [\s-1] -> error
                if (lex->token.value == TK_CHAR_SPE) {
                    return ERR_LEXER_BAD_CHARACTER_RANGE;
                }

                // [a-z]
                code2 = lex->token.extra.code;
                code = char_nextn(lex, 2);
                ret = token_char_accept(lex, code, false);
                if (ret) return ret;

                // [1-\s] -> error
                if (lex->token.value == TK_CHAR_SPE) {
                    return ERR_LEXER_BAD_CHARACTER_RANGE;
                }

                // [z-a] -> error
                if (lex->token.extra.code < code2) {
                    return ERR_LEXER_BAD_CHARACTER_RANGE;
                }

                // everything is ok
                lex->token.value = '-';
                lex->token.extra.code2 = lex->token.extra.code;
                lex->token.extra.code = code2;
            }
            break;
        }
    }
    return 0;
}

int tre_check_groups(uint32_t *s, int len) {
    int num = 0;
    for (int i = 0; i < len; ++i) {
        if (s[i] == '\\') i++;
        else if (s[i] == '(') {
            if (s[i + 1] == '?') {
                if (s[i + 2] == 'P') {
                    if (s[i + 3] == '<') {
                        i += 2;
                        num++;
                    }
                }
                else if (s[i + 2] == '(') i += 2;
                i++;
            } else num++;
        } else if (s[i] == '[') {
            while (i++) {
                if (s[i] == ']') break;
                else if (s[i] == '\0') return -1;
                else if (s[i] == '\\') i++;
            }
        }
    }
    return num;
}

tre_Lexer* tre_lexer_new(uint32_t *s, int len) {
    tre_Lexer* lex = tre_new(tre_Lexer, 1);
    lex->extra_flag = 0;
    lex->max_normal_group_num = tre_check_groups(s, len) + 1;
    //printf("AAAAAAAAA %d\n", lex->max_normal_group_num);
    lex->state = 0;

    if (s) {
        lex->s = s;
        lex->scur = 0;
        lex->slen = len;
    }
    return lex;
}

void tre_lexer_free(tre_Lexer *lex) {
    free(lex);
}

