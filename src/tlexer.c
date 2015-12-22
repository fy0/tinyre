
#include "tlexer.h"
#include "tutils.h"


_INLINE static
bool token_check(int code) {
    const char other_tokens[] = "^$*+?[]{()|";
    for (const char *p = other_tokens; *p; p++) {
        if (code == *p) return true;
    }
    return false;
}

_INLINE static
void token_char_accept(int code, const char* s_end, const char** pp, tre_Token** ppt) {
    const char* p = *pp;
    tre_Token* pt = *ppt;
    
    if (code == '\\') {
        // 如果已经是最后一个字符，那么当作普通字符即可
        if (p+1 == s_end) {
            pt->code = code;
            (pt++)->token = TK_CHAR;
        // 如果不是，读下一个字符
        } else {
            p = utf8_decode(p, &code);
            pt->code = code;
            (pt++)->token = TK_SPE_CHAR;
        }
    } else {
        pt->code = code;
        if (pt->code == '.') (pt++)->token = TK_SPE_CHAR;
        else (pt++)->token = TK_CHAR;
    }

    *pp = p;
    *ppt = pt;
}

_INLINE static
int char_to_flag(int code) {
    if (code == 'i') return FLAG_IGNORECASE;
    else if (code == 'm') return FLAG_MULTILINE;
    else if (code == 's') return FLAG_DOTALL;
    return 0;
}

int tre_lexer(char* s, TokenInfo** ptki) {
    int i, code;
    int extra_flag = 0;
    int len = utf8_len(s);
    int rlen = strlen(s);
    int max_normal_group_num = 1;
    const char* s_end = s + rlen + 1;
    TokenInfo* token_info;
    TokenGroupName* group_names = NULL, *last_group_name = NULL;

    tre_Token* tokens = _new(tre_Token, len+1);
    tre_Token* pt = tokens;

    int state = 0; // 0 NOMRAL | 1 [...] | 2 {...} | 3 (?...)

    for (const char* p = utf8_decode(s, &code); p != s_end; ) {
        if (state == 0) {
            if (token_check(code)) {
                pt->code = 0;
                (pt++)->token = code;

                if ((pt - 1)->token == '[') state = 1;
                else if ((pt - 1)->token == '{') state = 2;
                else if ((pt - 1)->token == '(') state = 3;
            } else {
                token_char_accept(code, s_end, &p, &pt);
            }
        } else if (state == 1) {
            if ((pt - 1)->token == '[' && code == '^')
                (pt - 1)->code = 1;
            else {
                token_char_accept(code, s_end, &p, &pt);

                if ((pt-1)->token == TK_CHAR) {
                    if ((pt-1)->code == ']') {
                        (pt-1)->token = ']';
                        state = 0;
                    }
                }
            }
        } else if (state == 2) {
            int llimit = 0, rlimit = -1, start_code = code;
            const char *start = p, *start2=p;

            // read left limit a{1
            if (isdigit(code)) {
                while (isdigit(code)) {
                    p = utf8_decode(p, &code);
                }

                for (i = p - start - 1; i >= 0; i--) {
                    // tip: if regex is a{1,2}, now p point at 2, so -2
                    llimit += (*(p - i - 2) - '0') * (int)pow(10, i);
                }
            }
            
            // read comma a{1,
            if ((char)code == ',') {
                p = utf8_decode(p, &code);
                start = p;
            } else if ((char)code == '}') {
                rlimit = llimit;
                goto __write_code;
            } else {
                // falied, rollback
                goto __bad_token;
            }

            // read left limit a{1, 2
            if (isdigit(code)) {
                while (isdigit(code)) {
                    p = utf8_decode(p, &code);
                }
                rlimit = 0;
                for (i = p - start - 1; i >= 0; i--) {
                    rlimit += (*(p - i - 2) - '0') * (int)pow(10, i);
                }
            }
            
            // read right brace a{1,2} or a{1,}
            if ((char)code == '}') {
                // ok, rlimit is -1
            } else {
                // falied, rollback
                goto __bad_token;
            }

        __write_code:
            (pt-1)->code = llimit;
            pt->code = rlimit;
            (pt++)->token = '}';
            goto __end;

        __bad_token:
            (pt-1)->code = (pt-1)->token;
            (pt-1)->token = TK_CHAR;
            p = start2;
            code = start_code;
            state = 0;
            continue;

        __end:
            state = 0;
        } else if (state == 3) {
            state = 0;
            if (code != '?') {
                max_normal_group_num++;
                continue;
            }
            p = utf8_decode(p, &code);
            if (code == '#') {
                bool is_escape = false;

                while (!(!is_escape && code == ')')) {
                    p = utf8_decode(p, &code);
                    if (is_escape) is_escape = false;
                    if (code == '\\') is_escape = true;
                    if (code == '\0') return ERR_LEXER_UNBALANCED_PARENTHESIS;
                }
                pt--;
                pt->token = 0;
            } else if (code == ':') {
                (pt - 1)->code = GT_NONGROUPING;
            } else if (code == '=') {
                (pt - 1)->code = GT_IF_MATCH;
            } else if (code == '!') {
                (pt - 1)->code = GT_IF_NOT_MATCH;
            } else if (code == 'P') {
                p = utf8_decode(p, &code);
                // group name
                if (code == '<') {
                    char* name;
                    const char* start = p;
                    TokenGroupName* group_name;

                    p = utf8_decode(p, &code);
                    while (true) {
                        if (!(isalnum(code) || code == '_')) break;
                        p = utf8_decode(p, &code);
                    }

                    if (code != '>') {
                        return ERR_LEXER_BAD_GROUP_NAME;
                    }

                    name = _new(char, p - start);
                    memcpy(name, start, p - start - 1);
                    name[p - start - 1] = '\0';

                    group_name = _new(TokenGroupName, 1);
                    group_name->name = name;
                    group_name->tk = pt-1;
                    group_name->next = NULL;

                    if (!last_group_name) group_names = last_group_name = group_name;
                    else last_group_name->next = group_name;

                    max_normal_group_num++;
                } else {
                    return ERR_LEXER_UNKNOW_SPECIFIER;
                }
            } else if (char_to_flag(code)) {
                int flag = 0;
                while (true) {
                    flag = char_to_flag(code);
                    if (flag) extra_flag |= flag;
                    else break;
                    p = utf8_decode(p, &code);
                }
                if (code != ')') {
                    return ERR_LEXER_UNEXPECTED_END_OF_PATTERN;
                }
                pt--;
                pt->token = 0;
            } else {
                return ERR_LEXER_UNEXPECTED_END_OF_PATTERN;
            }
        }

        p = utf8_decode(p, &code);
    }

    pt->code = 0;
    pt->token = 0; // END OF TOKENS

    token_info = _new(TokenInfo, 1);
    token_info->tokens = tokens;
    token_info->extra_flag = extra_flag;
    token_info->group_names = group_names;
    token_info->max_normal_group_num = max_normal_group_num;
    *ptki = token_info;

    return pt - tokens;
}

void token_info_free(TokenInfo* tki) {
    free(tki->tokens);
    free(tki->group_names);
}
