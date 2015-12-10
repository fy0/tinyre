
#include "tlexer.h"
#include "tutils.h"


_INLINE static
bool token_check(int code) {
    const char other_tokens[] = "^$*+?[]{}()|";
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

int tre_lexer(char* s, tre_Token** ppt) {
    int code;
    int len = utf8_len(s);
    int rlen = strlen(s);
    const char* s_end = s + rlen + 1;

    tre_Token* tokens = _new(tre_Token, len+1);
    tre_Token* pt = tokens;
    
    int state = 0; // 0 NOMRAL | 1 [...] | 2 {...}

    for (const char* p = utf8_decode(s, &code); p != s_end; p = utf8_decode(p, &code)) {
        if (state == 0) {
            if (token_check(code)) {
                pt->code = 0;
                (pt++)->token = code;

                if ((pt-1)->token == '[') state = 1;
                else if ((pt-1)->token == '{') state = 2;
            } else {
                token_char_accept(code, s_end, &p, &pt);
            }
        } else if (state == 1) {
            token_char_accept(code, s_end, &p, &pt);

            if ((pt-1)->token == TK_CHAR) {
                if ((pt-1)->code == ']') {
                    (pt-1)->token = ']';
                    state = 0;
                }
            }
        } else if (state == 2) {     
            pt->code = 0;
            if ((char)code == '}') {
                (pt++)->token = '}';
                state = 0;
            } else if ((char)code == ',' || isdigit(code)) {
				if (code == ',') (pt++)->token = code;
				else {
					pt->token = TK_DIGIT;
					(pt++)->code = code;
				}
            } else {
                free(tokens);
                *ppt = (tre_Token*)p;
                return -1;
            }
        }
    }

    pt->code = 0;
    pt->token = 0; // END OF TOKENS
    *ppt = tokens;
    return pt - tokens;
}
