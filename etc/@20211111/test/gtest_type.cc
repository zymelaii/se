#include <se/type.h>
#include <se/stack.h>
#include <se/alloc.h>
#include <gtest/gtest.h>

TEST(typeTest, FunctionCall)
{
	int state;
	char buf[64];
	se_object_t ret = se_call({ 0 }, 0L, &state);
	EXPECT_EQ(state, EF_CALL) << obj2str(ret, buf, 64);
}

TEST(typeTest, ObjectToString)
{
	token_t token;

	get_number("0b10010010100001010", &token);
	se_number_t _num_bin = parse_number(&token);

	get_number("0114514", &token);
	se_number_t _num_oct = parse_number(&token);

	get_number("5201314", &token);
	se_number_t _num_dec = parse_number(&token);

	get_number("0x0badf00d", &token);
	se_number_t _num_hex = parse_number(&token);

	get_number(".123e-2", &token);
	se_number_t _num_flt = parse_number(&token);

	se_function_t _func  = { 0L, "empty", 4 };

	se_object_t
	nil      = wrap2obj(0L, EO_NIL),
	num_bin  = wrap2obj(&_num_bin, EO_NUM),
	num_oct  = wrap2obj(&_num_oct, EO_NUM),
	num_dec  = wrap2obj(&_num_dec, EO_NUM),
	num_hex  = wrap2obj(&_num_hex, EO_NUM),
	num_flt  = wrap2obj(&_num_flt, EO_NUM),
	func     = wrap2obj(&_func, EO_FUNC);

	se_stack_t stack = se_stack_create(0);
	se_stack_push(&stack, num_flt);
	se_stack_push(&stack, func);
	se_stack_push(&stack, wrap2obj(&nil, EO_OBJ));
	se_stack_push(&stack, num_oct);
	se_array_t _array_dim1 = stack2array(&stack, 1);
	se_stack_push(&stack, num_bin);
	se_stack_push(&stack, wrap2obj(&_array_dim1, EO_ARRAY));
	se_stack_push(&stack, num_dec);
	se_stack_push(&stack, wrap2obj(&_array_dim1, EO_ARRAY));
	se_stack_push(&stack, nil);
	se_array_t _array_dim2 = stack2array(&stack, 1);
	se_stack_free(&stack);

	se_object_t
	array_dim1 = wrap2obj(&_array_dim1, EO_ARRAY),
	array_dim2 = wrap2obj(&_array_dim2, EO_ARRAY);

	char buffer[1024] = { 0 };

	EXPECT_STREQ(obj2str(nil, buffer, 64),     "nil");
	EXPECT_STREQ(obj2str(num_bin, buffer, 64), "0b10010010100001010");
	EXPECT_STREQ(obj2str(num_oct, buffer, 64), "0114514");
	EXPECT_STREQ(obj2str(num_dec, buffer, 64), "5201314");
	EXPECT_STREQ(obj2str(num_hex, buffer, 64), "0xbadf00d");
	EXPECT_STREQ(obj2str(num_flt, buffer, 64), "0.00123");
	EXPECT_STREQ(obj2str(func, buffer, 64),    "[Function<4>: empty]");

	EXPECT_STREQ(obj2str(array_dim1, buffer, 4),  "Arra");
	EXPECT_STREQ(obj2str(array_dim1, buffer, 8),  "Array<4>");
	EXPECT_STREQ(obj2str(array_dim1, buffer, 16), "Array<4> { ... }");
	EXPECT_STREQ(obj2str(array_dim1, buffer, 32), "Array<4> { 0.00123, ... }");
	EXPECT_STREQ(obj2str(array_dim1, buffer, 64), "Array<4> {"
		" 0.00123, [Function<4>: empty], Object<void>, 0114514 "
	"}");

	EXPECT_STREQ(obj2str(array_dim2, buffer, 64), "Array<9> {"
		" 0.00123, [Function<4>: empty], Object<void>, 0114514, ... "
	"}");

	EXPECT_STREQ(obj2str(array_dim2, buffer, 128), "Array<9> {"
		" 0.00123, [Function<4>: empty], Object<void>, 0114514,"
		" 0b10010010100001010, Array<4>, 5201314, Array<4>, nil "
	"}");

	se_free(_array_dim1.data);
	se_free(_array_dim2.data);
}