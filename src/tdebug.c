﻿
#include "tutils.h"
#include "tlexer.h"
#include "tparser.h"
#include "tvm.h"

void debug_token_print(tre_Lexer *lex) {
    int err;
    uint32_t tval;

    printf("token list:\n");
    while (true) {
        err = tre_lexer_next(lex);
        if (err) {
            tre_err(err);
            return;
        }
        tval = lex->token.value;

        //printf("    %12d ", tval, lex->token.extra.code);
        if (tval < FIRST_TOKEN) {
            printf("%12s  ", "<SIGN>");
            putchar(tval);
            switch (tval) {
                case '(':
                    printf("    GroupType:%d    ", lex->token.extra.group_type);
                    if (lex->token.extra.group_type == GT_BACKREF_CONDITIONAL_INDEX) {
                        printf("Index:%d", lex->token.extra.index);
                    }
                    break;
                case '{':
                    printf("    {%d, %d}", lex->token.extra.code, lex->token.extra.code2);
                    break;
                case '[':
                    printf("%5d", lex->token.extra.code);
                    break;
                case '-':
                    printf("    ");
                    putcode(lex->token.extra.code);
                    putchar('-');
                    putcode(lex->token.extra.code2);
                    break;
            }
        } else {
            if (tval == TK_CHAR) {
                printf("%12s  ", "<CHAR>");
                putcode(lex->token.extra.code);
            } else if (tval == TK_CHAR_SPE) {
                printf("%12s  ", "<CHAR_SPE>");
                if (lex->token.extra.code != '.') putchar('\\');
                putcode(lex->token.extra.code);
            } else if (tval == TK_BACK_REF) {
                printf("%12s  ", "<BACK_REF>");
                printf("%d", lex->token.extra.code);
            } else if (tval == TK_COMMENT) {
                printf("%12s  ", "<COMMENT>");
                printf("#");
            } else if (tval == TK_NOP) {
                printf("%12s  ", "<NOP>");
                printf("@");
            } else if (tval == TK_END) {
                putchar('\n');
                break;
            }
        }
        putchar('\n');
    }
    putchar('\n');
}

void debug_ins_list_print(ParserMatchGroup* groups) {
    int gnum = 0;

    for (ParserMatchGroup *g = groups; g; g = g->next) {
        if (gnum == 0) printf_u8("\nInstructions : Group 0\n");
        else {
            printf_u8("\nInstructions : Group %d (%d)", gnum, g->group_type);
            if (g->group_type == GT_IF_PRECEDED_BY || g->group_type == GT_IF_NOT_PRECEDED_BY || g->group_type == GT_BACKREF_CONDITIONAL_INDEX 
                    || g->group_type == GT_BACKREF_CONDITIONAL_GROUPNAME) {
                printf_u8(" [%d]", g->group_extra);
            }
            putchar('\n');
        }
        gnum++;

        for (INS_List* code = g->codes_start; code->next; code = code->next) {
            if (code->ins == ins_cmp) {
                printf_u8("%15s ", "CMP");
                putcode(*(int*)code->data);
                putchar('\n');
            } else if (code->ins == ins_cmp_spe) {
                printf_u8("%15s ", "CMP_SPE");
                putcode(*(int*)code->data);
                putchar('\n');
            } else if (code->ins == ins_cmp_multi || code->ins == ins_ncmp_multi) {
                if (code->ins == ins_cmp_multi) printf_u8("%15s %d ", "CMP_MULTI", *(int*)code->data);
                else printf_u8("%15s %d ", "NCMP_MULTI", *(int*)code->data);
                printf_u8("%6d    ", *((int*)code->data + 1));
                putcode(*((int*)code->data + 2));

                if (*((int*)code->data + 1) == '-') {
                    printf(" ");
                    putcode(*((int*)code->data + 3));
                }

                putchar('\n');
                for (int i = 1; i < *(int*)code->data; i++) {
                    printf_u8("                    %4d    ", *((int*)code->data + i * 3 + 1));
                    putcode(*((int*)code->data + i * 3 + 2));

                    if (*((int*)code->data + i * 3 + 1) == '-') {
                        printf(" ");
                        putcode(*((int*)code->data + i * 3 + 3));
                    }

                    putchar('\n');
                }
            } else if (code->ins == ins_cmp_backref) {
                printf_u8("%15s %d\n", "CMP_BACKREF", *(int*)code->data);
            } else if (code->ins == ins_cmp_group) {
                printf_u8("%15s %d\n", "CMP_GROUP", *(int*)code->data);
            } else if (code->ins == ins_check_point) {
                printf_u8("%15s %d %d\n", "CHECK_POINT", *(int*)code->data, *((int*)code->data + 1));
            } else if (code->ins == ins_check_point_no_greed) {
                printf_u8("%15s %d %d\n", "CHECK_POINT_NG", *(int*)code->data, *((int*)code->data + 1));
            } else if (code->ins == ins_match_start) {
                printf_u8("%15s\n", "MATCH_START");
            } else if (code->ins == ins_match_end) {
                printf_u8("%15s\n", "MATCH_END");
            } else if (code->ins == ins_group_end) {
                printf_u8("%15s %d\n", "GROUP_END", *(int*)code->data);
            }
        }

        printf_u8("\n");
    }
}

void debug_printstr(uint32_t *str, int head, int tail) {
    uint32_t *p = str + head;
    if (tail <= head) return;

    while (p != str + tail) {
        putcode(*p++);
    }
}
