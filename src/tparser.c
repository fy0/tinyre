
#include "tutils.h"
#include "tlexer.h"
#include "tparser.h"
#include "tvm.h"

#define paser_accept(__stat) if (!((__stat))) return NULL;
#define check_token(tk) if (tk == NULL || tk->token == NULL) return NULL;

ParserMatchGroup* m_start = NULL;
ParserMatchGroup* m_cur = NULL;

tre_Token* parser_char_set(tre_Token* tk);
tre_Token* parser_char(tre_Token* tk);
tre_Token* parser_block(tre_Token* tk);
tre_Token* parser_group(tre_Token* tk);

tre_Token* parser_single_char(tre_Token* tk) {
	if (tk->token == TK_CHAR || tk->token == TK_SPE_CHAR) {
		// 代码生成
		// CMP/CMP_SPE CODE
		m_cur->codes->ins = tk->token == TK_CHAR ? ins_cmp : ins_cmp_spe;
		m_cur->codes->data = _new(int, 1);
		m_cur->codes->len = sizeof(int) * 1;
		*(int*)m_cur->codes->data = tk->code;
		m_cur->codes->next = _new(INS_List, 1);
		m_cur->codes = m_cur->codes->next;
		m_cur->codes->next = NULL;
		// END
		return tk + 1;
	}
	return NULL;
}

tre_Token* parser_char_set(tre_Token* tk) {
	int num;
	int* data;
	tre_Token* start;
    printf_u8("CHAR_SET\n");

	paser_accept((tk++)->token == '[');
	check_token(tk);
	start = tk;
	paser_accept(tk->token == TK_CHAR || tk->token == TK_SPE_CHAR);
    tk++;
	check_token(tk);
	while ((tk->token == TK_CHAR || tk->token == TK_SPE_CHAR)) {
		tk++;
		check_token(tk);
	}
	paser_accept(tk->token == ']');

	// 代码生成
	// CMP_MULTI NUM [TYPE1 CODE1], [TYPE2, CODE2], ...
	num = tk - start;
	m_cur->codes->ins = ins_cmp_multi;
	m_cur->codes->data = _new(int, 2 * num + 1);
	m_cur->codes->len = sizeof(int) * (2 * num + 1);
	// END

	data = (int*)m_cur->codes->data;
	*(data++) = num;

	for (; start != tk; start++) {
		*data = start->token;
		*(data + 1) = start->code;
		data += 2;
	}

	m_cur->codes->next = _new(INS_List, 1);
	m_cur->codes = m_cur->codes->next;
	m_cur->codes->next = NULL;
    return tk + 1;
}

tre_Token* parser_char(tre_Token* tk) {
    printf_u8("CHAR\n");
	tre_Token* ret = parser_single_char(tk);
	if (ret) return ret;
	return parser_char_set(tk);
}

tre_Token* parser_other_tokens(tre_Token* tk) {
	if (tk->token == '^' || tk->token == '$') {
		// 代码生成
		// MATCH_START/MATCH_END
		m_cur->codes->ins = tk->token == '^' ? ins_match_start : ins_match_end;
		m_cur->codes->len = 0;
		m_cur->codes->next = _new(INS_List, 1);
		m_cur->codes = m_cur->codes->next;
		m_cur->codes->next = NULL;
		// END
		return tk + 1;
	}
	return NULL;
}

tre_Token* parser_block(tre_Token* tk) {
    tre_Token* ret;
	INS_List* last_ins;

    printf_u8("BLOCK\n");
	check_token(tk);
	last_ins = m_cur->codes;

    ret = parser_char(tk);
	if (!ret) ret = parser_group(tk);

	if (ret) {
		bool need_checkpoint = false;
		int llimit, rlimit;

		tk = ret;

		if (tk->token == '?') {
			ret++;
			llimit = 0;
			rlimit = 1;
			need_checkpoint = true;
		} else if (tk->token == '+') {
			ret++;
			llimit = 1;
			rlimit = -1;
			need_checkpoint = true;
		} else if (tk->token == '*') {
			ret++;
			llimit = 0;
			rlimit = -1;
			need_checkpoint = true;
		} else if (tk->token == '{') {
			;
		}

		// 代码生成
		// CHECK_POINT LLIMIT RLIMIT
		if (need_checkpoint) {
			// 将上一条指令（必然是一条匹配指令复制一遍）
			memcpy(m_cur->codes, last_ins, sizeof(INS_List));
			// 为这条指令创建新的后继节点
			m_cur->codes->next = _new(INS_List, 1);
			m_cur->codes = m_cur->codes->next;
			m_cur->codes->next = NULL;

			last_ins->ins = ins_check_point;
			last_ins->data = _new(int, 2);
			last_ins->len = sizeof(int) * 2;
			*(int*)last_ins->data = llimit;
			*((int*)last_ins->data + 1) = rlimit;
		}
		// END
	} else {
		ret = parser_other_tokens(tk);
	}
	
    return ret;
}

tre_Token* parser_group(tre_Token* tk) {
	int gindex = 1;
    tre_Token* ret;
	ParserMatchGroup* last_group;

    printf_u8("GROUP\n");
	paser_accept((tk++)->token == '(');
	check_token(tk);

	last_group = m_cur;
	// 前进至最后一节
	m_cur = m_start;
	while (m_cur->next) {
		m_cur = m_cur->next;
		gindex++;
	}
	// 创建新节点
	m_cur->next = _new(ParserMatchGroup, 1);
	m_cur->next->codes = m_cur->next->codes_start = _new(INS_List, 1);
	m_cur = m_cur->next;
	m_cur->next = NULL;

    ret = tk;
    while ((ret = parser_block(ret))) tk = ret;

    paser_accept(tk->token == ')');

	// 代码生成
	// CMP_GROUP INDEX
	last_group->codes->ins = ins_cmp_group;
	last_group->codes->data = _new(int, 1);
	last_group->codes->len = sizeof(int) * 1;
	*(int*)last_group->codes->data = gindex;
	last_group->codes->next = _new(INS_List, 1);
	last_group->codes = last_group->codes->next;
	last_group->codes->next = NULL;
	// END

	m_cur = last_group;
    return tk+1;
}

tre_Token* parser_blocks(tre_Token* tk) {
	tre_Token* ret;
    //paser_accept(tk = parser_block(tk));
    ret = tk;
    while ((ret = parser_block(ret))) tk = ret;
	return tk;
}

tre_Pattern* tre_parser(tre_Token* tk, tre_Token** last_token) {
	m_cur = m_start = _new(ParserMatchGroup, 1);
	m_start->codes = m_start->codes_start = _new(INS_List, 1);
	m_start->codes->next = NULL;
	m_cur->next = NULL;

    tre_Token* ret = parser_blocks(tk);
	*last_token = ret;

	if (ret) {
		int group_num;
		debug_ins_list_print(m_start);
		return compact_group(m_start);
	}
	return NULL;
}


tre_Pattern* compact_group(ParserMatchGroup* parser_groups) {
	int* data;
	int gnum = 0;
	MatchGroup* g;
	MatchGroup* groups;
	ParserMatchGroup *pg;
	tre_Pattern* ret = _new(tre_Pattern, 1);

	for (pg = parser_groups; pg; pg = pg->next) gnum++;
	groups = _new(MatchGroup, gnum);

	gnum = 0;
	for (pg = parser_groups; pg; pg = pg->next) {
		int code_lens = 0;
		g = groups + gnum;

		for (INS_List* code = pg->codes_start; code->next; code = code->next) {
			code_lens += (code->len + sizeof(int));
		}
		// sizeof(int)*2 is space for group_end
		data = g->codes = malloc(code_lens + sizeof(int)*2);
		g->name = NULL;

		for (INS_List* code = pg->codes_start; code->next; code = code->next) {
			*data++ = code->ins;
			if (code->len) {
				memcpy(data, code->data, code->len);
				data += (code->len / sizeof(int));
			}
		}

		*data = ins_group_end;
		*(data + 1) = gnum;

		gnum++;
	}

	ret->groups = groups;
	ret->num = gnum;

	return ret;
}
