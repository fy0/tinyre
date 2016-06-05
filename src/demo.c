/*
 * start : 2012-4-8 09:57
 * update: 2015-12-10 v0.9.0
 *
 * tinyre
 * fy, 2012-2015
 *
 */

#include "tutils.h"
#include "tdebug.h"

#include "tinyre.h"


#ifdef DEMO

int main(int argc,char* argv[])
{
    int i;
    int err_code;
    tre_Pattern* pattern;
    tre_Match* match = NULL;
    platform_init();

    pattern = tre_compile("1(2)[3]", FLAG_DOTALL, &err_code);

    if (pattern) {
        match = tre_match(pattern, "123", 5000);

        if (match->groups) {
            putchar('\n');
            for (i = 0; i < match->groupnum; i++) {
                printf("Group %2d: ", i);
                if (match->groups[i].name) {
                    printf("(");
                    output_str(match->groups[i].name, match->groups[i].name_len);
                    printf(") ");
                } else printf("(null) ");
                printf("%d %d\n", match->groups[i].head, match->groups[i].tail);
                if (match->groups[i].head != -1) {
                    debug_printstr(match->str, match->groups[i].head, match->groups[i].tail);
                } else {
                    printf("match failed.");
                }
                printf("\n");
            }
        }
    } else {
        tre_err(err_code);
    }

    if (pattern) {
        tre_pattern_free(pattern);
        if (match) {
            tre_match_free(match);  
        }
    }

    return 0;
}

#endif
