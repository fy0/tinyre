
#include "tutils.h"
#include "tlexer.h"
#include "tparser.h"
#include "tvm.h"

void debug_token_print(tre_Token* tokens, int len) {
    for (tre_Token *p = tokens; p != tokens + len; p++) {
        printf("%8d %12d ", p->token, p->code);
        if (p->token < FIRST_TOKEN) {
            printf("T ");
            putchar(p->token);
        } else {
            if (p->token == TK_CHAR) {
                printf("C ");
                putcode(p->code);
            } else if (p->token == TK_SPE_CHAR) {
                printf("S ");
                if (p->code != '.') putchar('\\');
                putcode(p->code);
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
        else printf_u8("\nInstructions : Group %d (%d)\n", gnum, g->group_type);
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
            } else if (code->ins == ins_cmp_multi) {
                printf_u8("%15s %d ", "CMP_MULTI", *(int*)code->data);
                printf_u8("%6d    ", *((int*)code->data + 1));
                putcode(*((int*)code->data + 2));
                putchar('\n');
                for (int i = 1; i < *(int*)code->data; i++) {
                    printf_u8("                    %4d    ", *((int*)code->data + i * 2 + 1));
                    putcode(*((int*)code->data + i * 2 + 2));
                    putchar('\n');
                }
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

void debug_printstr(int* head, int* tail) {
    int *p = head;
    if (tail <= head) return;

    while (p != tail) {
        putcode(*p++);
    }
}
