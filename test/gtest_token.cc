#include <se/token.h>
#include <gtest/gtest.h>

TEST(tokenTest, Reset)
{
	token_t token;

	reset_token(&token);

	EXPECT_EQ(token.type, T_NULL);
	EXPECT_EQ(token.sub_type, 0);
	EXPECT_EQ(token.p, nullptr);
	EXPECT_EQ(token.q, nullptr);
	EXPECT_EQ(token.r, nullptr);
}

TEST(tokenTest, Predict)
{
	EXPECT_EQ(predict(nullptr),              T_INVALID);
	EXPECT_EQ(predict("\xff"),               T_INVALID);
	EXPECT_EQ(predict(""),                   T_NULL);
	EXPECT_EQ(predict("  \t"),               T_INDENT);
	EXPECT_EQ(predict("\n Hello World! \r"), T_INDENT);
	EXPECT_EQ(predict("."),                  T_NUMBER);
	EXPECT_EQ(predict("0x01ffa"),            T_NUMBER);
	EXPECT_EQ(predict("_fnCall"),            T_SYMBOL);
	EXPECT_EQ(predict(">>="),                T_OPERATOR);
}

TEST(tokenTest, GetIndent)
{
	token_t token;

	EXPECT_EQ(get_indent("", &token), 0);

	EXPECT_EQ(get_indent("    \t", &token), 4);
	EXPECT_EQ(token.type, T_INDENT);
	EXPECT_EQ(token.sub_type, T_IDT_BLANK);

	EXPECT_EQ(get_indent("\t\t \\ \n\r ", &token), 2);
	EXPECT_EQ(token.type, T_INDENT);
	EXPECT_EQ(token.sub_type, T_IDT_TAB);

	EXPECT_EQ(get_indent("\n\r\r\r\n\nnn\nn\ads\n ", &token), 6);
	EXPECT_EQ(token.type, T_INDENT);
	EXPECT_EQ(token.sub_type, T_IDT_NEWLINE);
}

TEST(tokenTest, GetSingleNumber)
{
	token_t token;

	const char *expr_0_single    = "0";
	const char *expr_0_extra     = "0+1";
	const char *expr_1to9_single = "2";
	const char *expr_1to9_extra  = "7_";

	EXPECT_EQ(get_number(expr_0_single, &token), 1);
	EXPECT_EQ(token.type, T_NUMBER);
	EXPECT_EQ(token.sub_type, T_NUM_DEC);
	EXPECT_STREQ(token.q + 1, "");

	EXPECT_EQ(get_number(expr_0_extra, &token), 1);
	EXPECT_EQ(token.type, T_NUMBER);
	EXPECT_EQ(token.sub_type, T_NUM_DEC);
	EXPECT_STREQ(token.q + 1, "+1");

	EXPECT_EQ(get_number(expr_1to9_single, &token), 1);
	EXPECT_EQ(token.type, T_NUMBER);
	EXPECT_EQ(token.sub_type, T_NUM_DEC);
	EXPECT_STREQ(token.q + 1, "");

	EXPECT_EQ(get_number(expr_1to9_extra, &token), 1);
	EXPECT_EQ(token.type, T_NUMBER);
	EXPECT_EQ(token.sub_type, T_NUM_DEC);
	EXPECT_STREQ(token.q + 1, "_");
}

TEST(tokenTest, GetCommonNumber)
{
	token_t token;

	const char *expr_bin       = "0b100010100100101";
	const char *expr_bin_cut   = "0b1001021010";
	const char *expr_bin_bad_1 = "0b";
	const char *expr_bin_bad_2 = "0b9";
	const char *expr_oct       = "0345651234";
	const char *expr_oct_cut   = "013740124871364";
	const char *expr_dec       = "317980141";
	const char *expr_hex_pd    = "0x14713945";
	const char *expr_hex_lc    = "0xbadfd";
	const char *expr_hex_rc    = "0xBADFD";
	const char *expr_hex_mix   = "0x0BADf00d";
	const char *expr_hex_cut   = "0xd013r103hjqh1";
	const char *expr_hex_bad_1 = "0x";
	const char *expr_hex_bad_2 = "0xlqp";

	EXPECT_EQ(get_number(expr_bin, &token), 17);
	EXPECT_EQ(token.type, T_NUMBER);
	EXPECT_EQ(token.sub_type, T_NUM_BIN);
	EXPECT_STREQ(token.q + 1, "");

	EXPECT_EQ(get_number(expr_bin_cut, &token), 7);
	EXPECT_EQ(token.type, T_NUMBER);
	EXPECT_EQ(token.sub_type, T_NUM_BIN);
	EXPECT_STREQ(token.q + 1, "21010");

	EXPECT_EQ(get_number(expr_bin_bad_1, &token), 0);
	EXPECT_EQ(token.type, T_NUMBER);
	EXPECT_EQ(token.sub_type, 0);
	EXPECT_EQ(token.q, nullptr);

	EXPECT_EQ(get_number(expr_bin_bad_2, &token), 0);
	EXPECT_EQ(token.type, T_NUMBER);
	EXPECT_EQ(token.sub_type, 0);
	EXPECT_EQ(token.q, nullptr);

	EXPECT_EQ(get_number(expr_oct, &token), 10);
	EXPECT_EQ(token.type, T_NUMBER);
	EXPECT_EQ(token.sub_type, T_NUM_OCT);
	EXPECT_STREQ(token.q + 1, "");

	EXPECT_EQ(get_number(expr_oct_cut, &token), 9);
	EXPECT_EQ(token.type, T_NUMBER);
	EXPECT_EQ(token.sub_type, T_NUM_OCT);
	EXPECT_STREQ(token.q + 1, "871364");

	EXPECT_EQ(get_number(expr_dec, &token), 9);
	EXPECT_EQ(token.type, T_NUMBER);
	EXPECT_EQ(token.sub_type, T_NUM_DEC);
	EXPECT_STREQ(token.q + 1, "");

	EXPECT_EQ(get_number(expr_hex_pd, &token), 10);
	EXPECT_EQ(token.type, T_NUMBER);
	EXPECT_EQ(token.sub_type, T_NUM_HEX);
	EXPECT_STREQ(token.q + 1, "");

	EXPECT_EQ(get_number(expr_hex_lc, &token), 7);
	EXPECT_EQ(token.type, T_NUMBER);
	EXPECT_EQ(token.sub_type, T_NUM_HEX);
	EXPECT_STREQ(token.q + 1, "");
	
	EXPECT_EQ(get_number(expr_hex_rc, &token), 7);
	EXPECT_EQ(token.type, T_NUMBER);
	EXPECT_EQ(token.sub_type, T_NUM_HEX);
	EXPECT_STREQ(token.q + 1, "");
	
	EXPECT_EQ(get_number(expr_hex_mix, &token), 10);
	EXPECT_EQ(token.type, T_NUMBER);
	EXPECT_EQ(token.sub_type, T_NUM_HEX);
	EXPECT_STREQ(token.q + 1, "");

	EXPECT_EQ(get_number(expr_hex_cut, &token), 6);
	EXPECT_EQ(token.type, T_NUMBER);
	EXPECT_EQ(token.sub_type, T_NUM_HEX);
	EXPECT_STREQ(token.q + 1, "r103hjqh1");

	EXPECT_EQ(get_number(expr_hex_bad_1, &token), 0);
	EXPECT_EQ(token.type, T_NUMBER);
	EXPECT_EQ(token.sub_type, 0);
	EXPECT_EQ(token.q, nullptr);

	EXPECT_EQ(get_number(expr_hex_bad_2, &token), 0);
	EXPECT_EQ(token.type, T_NUMBER);
	EXPECT_EQ(token.sub_type, 0);
	EXPECT_EQ(token.q, nullptr);
}

TEST(tokenTest, GetFloatNumber)
{
	token_t token;

	const char *expr_float_prefix   = ".012124968";
	const char *expr_float_infix_1  = "0.123";
	const char *expr_float_infix_2  = "1247.0";
	const char *expr_float_infix_3  = "1247.13794";
	const char *expr_float_suffix_1 = "0.";
	const char *expr_float_suffix_2 = "1247.";
	const char *expr_float_bad      = ".";

	EXPECT_EQ(get_number(expr_float_prefix , &token), 10);
	EXPECT_EQ(token.type, T_NUMBER);
	EXPECT_EQ(token.sub_type, T_NUM_FLT);
	EXPECT_STREQ(token.q + 1, "");
	
	EXPECT_EQ(get_number(expr_float_infix_1, &token), 5);
	EXPECT_EQ(token.type, T_NUMBER);
	EXPECT_EQ(token.sub_type, T_NUM_FLT);
	EXPECT_STREQ(token.q + 1, "");
	
	EXPECT_EQ(get_number(expr_float_infix_2, &token), 6);
	EXPECT_EQ(token.type, T_NUMBER);
	EXPECT_EQ(token.sub_type, T_NUM_FLT);
	EXPECT_STREQ(token.q + 1, "");
	
	EXPECT_EQ(get_number(expr_float_infix_3 , &token), 10);
	EXPECT_EQ(token.type, T_NUMBER);
	EXPECT_EQ(token.sub_type, T_NUM_FLT);
	EXPECT_STREQ(token.q + 1, "");
	
	EXPECT_EQ(get_number(expr_float_suffix_1, &token), 2);
	EXPECT_EQ(token.type, T_NUMBER);
	EXPECT_EQ(token.sub_type, T_NUM_FLT);
	EXPECT_STREQ(token.q + 1, "");
	
	EXPECT_EQ(get_number(expr_float_suffix_2, &token), 5);
	EXPECT_EQ(token.type, T_NUMBER);
	EXPECT_EQ(token.sub_type, T_NUM_FLT);
	EXPECT_STREQ(token.q + 1, "");
	
	EXPECT_EQ(get_number(expr_float_bad, &token), 0);
	EXPECT_EQ(token.type, T_NUMBER);
	EXPECT_EQ(token.sub_type, 0);
	EXPECT_EQ(token.q, nullptr);
}

TEST(tokenTest, GetEEEENumber)
{
	token_t token;

	const char *expr_eeee_1     = "0e0";
	const char *expr_eeee_2     = "1207e17413";
	const char *expr_eeee_3     = "1.123e-123";
	const char *expr_eeee_4     = "114514.e37";
	const char *expr_eeee_5     = ".3e+1";
	const char *expr_eeee_6     = ".e0";
	const char *expr_eeee_bad_1 = "e";
	const char *expr_eeee_bad_2 = "0e";
	const char *expr_eeee_bad_3 = "123e+";
	const char *expr_eeee_bad_4 = "01e-";
	const char *expr_eeee_bad_5 = "1e-0b1001";

	EXPECT_EQ(get_number(expr_eeee_1, &token), 3);
	EXPECT_EQ(token.type, T_NUMBER);
	EXPECT_EQ(token.sub_type, T_NUM_EEEE);
	EXPECT_STREQ(token.q + 1, "");

	EXPECT_EQ(get_number(expr_eeee_2, &token), 10);
	EXPECT_EQ(token.type, T_NUMBER);
	EXPECT_EQ(token.sub_type, T_NUM_EEEE);
	EXPECT_STREQ(token.q + 1, "");

	EXPECT_EQ(get_number(expr_eeee_3, &token), 10);
	EXPECT_EQ(token.type, T_NUMBER);
	EXPECT_EQ(token.sub_type, T_NUM_EEEE);
	EXPECT_STREQ(token.q + 1, "");

	EXPECT_EQ(get_number(expr_eeee_4, &token), 10);
	EXPECT_EQ(token.type, T_NUMBER);
	EXPECT_EQ(token.sub_type, T_NUM_EEEE);
	EXPECT_STREQ(token.q + 1, "");

	EXPECT_EQ(get_number(expr_eeee_5, &token), 5);
	EXPECT_EQ(token.type, T_NUMBER);
	EXPECT_EQ(token.sub_type, T_NUM_EEEE);
	EXPECT_STREQ(token.q + 1, "");

	EXPECT_EQ(get_number(expr_eeee_6, &token), 3);
	EXPECT_EQ(token.type, T_NUMBER);
	EXPECT_EQ(token.sub_type, T_NUM_EEEE);
	EXPECT_STREQ(token.q + 1, "");

	EXPECT_EQ(get_number(expr_eeee_bad_1, &token), 0);
	EXPECT_EQ(token.type, T_NULL);
	EXPECT_EQ(token.sub_type, 0);
	EXPECT_EQ(token.p, nullptr);
	EXPECT_EQ(token.q, nullptr);
	EXPECT_EQ(token.r, nullptr);

	EXPECT_EQ(get_number(expr_eeee_bad_2, &token), 0);
	EXPECT_EQ(token.type, T_NUMBER);
	EXPECT_EQ(token.q, nullptr);

	EXPECT_EQ(get_number(expr_eeee_bad_3, &token), 0);
	EXPECT_EQ(token.type, T_NUMBER);
	EXPECT_EQ(token.q, nullptr);

	EXPECT_EQ(get_number(expr_eeee_bad_4, &token), 2);
	EXPECT_EQ(token.type, T_NUMBER);
	EXPECT_EQ(token.sub_type, T_NUM_OCT);
	EXPECT_STREQ(token.q + 1, "e-");

	EXPECT_EQ(get_number(expr_eeee_bad_5, &token), 4);
	EXPECT_EQ(token.type, T_NUMBER);
	EXPECT_EQ(token.sub_type, T_NUM_EEEE);
	EXPECT_STREQ(token.q + 1, "b1001");
}

TEST(tokenTest, GetNullSymbol)
{
	token_t token;

	const char *expr_null  = "";
	const char *expr_blank = " ";

	EXPECT_EQ(get_symbol(expr_null, &token), 0);
	EXPECT_EQ(token.type, T_NULL);
	EXPECT_EQ(token.sub_type, 0);
	EXPECT_EQ(token.p, nullptr);
	EXPECT_EQ(token.q, nullptr);

	EXPECT_EQ(get_symbol(expr_blank, &token), 0);
	EXPECT_EQ(token.type, T_NULL);
	EXPECT_EQ(token.sub_type, 0);
	EXPECT_EQ(token.p, nullptr);
	EXPECT_EQ(token.q, nullptr);
}

TEST(tokenTest, GetSingleSymbol)
{
	token_t token;

	const char *expr_startwith_underline    = "_//";
	const char *expr_startwith_alphabet_big = "A+czwq";
	const char *expr_startwith_alphabet_sml = "s ";
	const char *expr_startwith_alphabet_num = "4&08h1";

	EXPECT_EQ(get_symbol(expr_startwith_underline, &token), 1);
	EXPECT_EQ(token.type, T_SYMBOL);
	EXPECT_EQ(token.sub_type, 0);
	EXPECT_STREQ(token.q + 1, "//");

	EXPECT_EQ(get_symbol(expr_startwith_alphabet_big, &token), 1);
	EXPECT_EQ(token.type, T_SYMBOL);
	EXPECT_EQ(token.sub_type, 0);
	EXPECT_STREQ(token.q + 1, "+czwq");

	EXPECT_EQ(get_symbol(expr_startwith_alphabet_sml, &token), 1);
	EXPECT_EQ(token.type, T_SYMBOL);
	EXPECT_EQ(token.sub_type, 0);
	EXPECT_STREQ(token.q + 1, " ");

	EXPECT_EQ(get_symbol(expr_startwith_alphabet_num, &token), 0);
	EXPECT_EQ(token.type, T_NULL);
	EXPECT_EQ(token.sub_type, 0);
	EXPECT_EQ(token.p, nullptr);
	EXPECT_EQ(token.q, nullptr);
}

TEST(tokenTest, GetCompleteSymbol)
{
	token_t token;

	const char *expr_all = "_Hello_12er08fqwhin13gF_HEvn";
	const char *expr_part = "_Hello_12er08fqwhin13gF_HEvn 124erg0fub";

	EXPECT_EQ(get_symbol(expr_all, &token), 28);
	EXPECT_EQ(token.type, T_SYMBOL);
	EXPECT_EQ(token.sub_type, 0);
	EXPECT_STREQ(token.q + 1, "");

	EXPECT_EQ(get_symbol(expr_part, &token), 28);
	EXPECT_EQ(token.type, T_SYMBOL);
	EXPECT_EQ(token.sub_type, 0);
	EXPECT_STREQ(token.q + 1, " 124erg0fub");
}

TEST(tokenTest, GetOperator)
{
	token_t token;

	const char* TYPE[] = {
		"COMMA",     "SEMICOLON", "BRACKET0L", "BRACKET0R",
		"BRACKET1L", "BRACKET1R", "BRACKET2L", "BRACKET2R",
		"ADD",       "SUB",       "MOD",       "MUL",
		"DIV",       "ADD_ASS",   "SUB_ASS",   "MOD_ASS",
		"MUL_ASS",   "DIV_ASS",   "AND",       "OR",
		"XOR",       "LSH",       "RSH",       "NOT",
		"NOR",       "AND_ASS",   "OR_ASS",    "XOR_ASS",
		"LSH_ASS",   "RSH_ASS",   "GTR",       "LSS",
		"LEQ",       "GEQ",       "EQU",       "NEQ",
		"CMP_AND",   "CMP_OR",    "ASS",
	};

	EXPECT_EQ(get_operator(",", &token), 1);
	EXPECT_STREQ(TYPE[token.sub_type - 1], "COMMA");

	EXPECT_EQ(get_operator(";", &token), 1);
	EXPECT_STREQ(TYPE[token.sub_type - 1], "SEMICOLON");

	EXPECT_EQ(get_operator("(", &token), 1);
	EXPECT_STREQ(TYPE[token.sub_type - 1], "BRACKET0L");

	EXPECT_EQ(get_operator(")", &token), 1);
	EXPECT_STREQ(TYPE[token.sub_type - 1], "BRACKET0R");

	EXPECT_EQ(get_operator("[", &token), 1);
	EXPECT_STREQ(TYPE[token.sub_type - 1], "BRACKET1L");

	EXPECT_EQ(get_operator("]", &token), 1);
	EXPECT_STREQ(TYPE[token.sub_type - 1], "BRACKET1R");

	EXPECT_EQ(get_operator("{", &token), 1);
	EXPECT_STREQ(TYPE[token.sub_type - 1], "BRACKET2L");

	EXPECT_EQ(get_operator("}", &token), 1);
	EXPECT_STREQ(TYPE[token.sub_type - 1], "BRACKET2R");

	EXPECT_EQ(get_operator("+", &token), 1);
	EXPECT_STREQ(TYPE[token.sub_type - 1], "ADD");

	EXPECT_EQ(get_operator("-", &token), 1);
	EXPECT_STREQ(TYPE[token.sub_type - 1], "SUB");

	EXPECT_EQ(get_operator("%", &token), 1);
	EXPECT_STREQ(TYPE[token.sub_type - 1], "MOD");

	EXPECT_EQ(get_operator("*", &token), 1);
	EXPECT_STREQ(TYPE[token.sub_type - 1], "MUL");

	EXPECT_EQ(get_operator("/", &token), 1);
	EXPECT_STREQ(TYPE[token.sub_type - 1], "DIV");

	EXPECT_EQ(get_operator(">", &token), 1);
	EXPECT_STREQ(TYPE[token.sub_type - 1], "GTR");

	EXPECT_EQ(get_operator("<", &token), 1);
	EXPECT_STREQ(TYPE[token.sub_type - 1], "LSS");

	EXPECT_EQ(get_operator("=", &token), 1);
	EXPECT_STREQ(TYPE[token.sub_type - 1], "ASS");

	EXPECT_EQ(get_operator("!", &token), 1);
	EXPECT_STREQ(TYPE[token.sub_type - 1], "NOT");

	EXPECT_EQ(get_operator("&", &token), 1);
	EXPECT_STREQ(TYPE[token.sub_type - 1], "AND");

	EXPECT_EQ(get_operator("|", &token), 1);
	EXPECT_STREQ(TYPE[token.sub_type - 1], "OR");

	EXPECT_EQ(get_operator("^", &token), 1);
	EXPECT_STREQ(TYPE[token.sub_type - 1], "XOR");

	EXPECT_EQ(get_operator("~", &token), 1);
	EXPECT_STREQ(TYPE[token.sub_type - 1], "NOR");

	EXPECT_EQ(get_operator("~", &token), 1);
	EXPECT_STREQ(TYPE[token.sub_type - 1], "NOR");

	EXPECT_EQ(get_operator("+=", &token), 2);
	EXPECT_STREQ(TYPE[token.sub_type - 1], "ADD_ASS");

	EXPECT_EQ(get_operator("-=", &token), 2);
	EXPECT_STREQ(TYPE[token.sub_type - 1], "SUB_ASS");

	EXPECT_EQ(get_operator("%=", &token), 2);
	EXPECT_STREQ(TYPE[token.sub_type - 1], "MOD_ASS");

	EXPECT_EQ(get_operator("*=", &token), 2);
	EXPECT_STREQ(TYPE[token.sub_type - 1], "MUL_ASS");

	EXPECT_EQ(get_operator("/=", &token), 2);
	EXPECT_STREQ(TYPE[token.sub_type - 1], "DIV_ASS");

	EXPECT_EQ(get_operator(">=", &token), 2);
	EXPECT_STREQ(TYPE[token.sub_type - 1], "GEQ");

	EXPECT_EQ(get_operator("<=", &token), 2);
	EXPECT_STREQ(TYPE[token.sub_type - 1], "LEQ");

	EXPECT_EQ(get_operator("==", &token), 2);
	EXPECT_STREQ(TYPE[token.sub_type - 1], "EQU");

	EXPECT_EQ(get_operator("!=", &token), 2);
	EXPECT_STREQ(TYPE[token.sub_type - 1], "NEQ");

	EXPECT_EQ(get_operator("&=", &token), 2);
	EXPECT_STREQ(TYPE[token.sub_type - 1], "AND_ASS");

	EXPECT_EQ(get_operator("|=", &token), 2);
	EXPECT_STREQ(TYPE[token.sub_type - 1], "OR_ASS");

	EXPECT_EQ(get_operator("^=", &token), 2);
	EXPECT_STREQ(TYPE[token.sub_type - 1], "XOR_ASS");

	EXPECT_EQ(get_operator(">>", &token), 2);
	EXPECT_STREQ(TYPE[token.sub_type - 1], "RSH");

	EXPECT_EQ(get_operator("<<", &token), 2);
	EXPECT_STREQ(TYPE[token.sub_type - 1], "LSH");

	EXPECT_EQ(get_operator("&&", &token), 2);
	EXPECT_STREQ(TYPE[token.sub_type - 1], "CMP_AND");

	EXPECT_EQ(get_operator("||", &token), 2);
	EXPECT_STREQ(TYPE[token.sub_type - 1], "CMP_OR");

	EXPECT_EQ(get_operator(">>=", &token), 3);
	EXPECT_STREQ(TYPE[token.sub_type - 1], "RSH_ASS");

	EXPECT_EQ(get_operator("<<=", &token), 3);
	EXPECT_STREQ(TYPE[token.sub_type - 1], "LSH_ASS");
}