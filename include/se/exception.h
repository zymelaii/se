#pragma once

#include <stdint.h>

typedef struct se_exception_s
{
	uint32_t etype;    // 错误类型
	uint32_t error;    // 错误代码
	uint64_t extra;    // 附加信息
	uint64_t reserved; // 保留字段
} se_exception_t;

#ifdef __cplusplus
extern "C" {
#endif

void se_throw(uint32_t etype, uint32_t error, uint64_t extra, uint64_t reserved);
int se_caught();
int se_catch(se_exception_t *e, uint32_t type);
int se_catch_err(se_exception_t *e, uint32_t type, uint32_t error);
int se_catch_any(se_exception_t *e);

#ifdef __cplusplus
}
#endif

// type of se exception
#define UnknownError           0x01 // 未知错误，通常产生于于se以外的环境
#define SyntaxError            0x02 // 语法错误
#define TypeError              0x03 // 类型错误
#define IndexError             0x04 // 数组索引错误
#define RuntimeError           0x05 // 运行时错误
#define CustomError            0x06 // 自定义错误类型（起始值）

// UnknownError
#define ArgumentErrorInCSrc    0x01 // C源码函数调用参数出现问题

// SyntaxError
#define InvalidSyntax          0x01 // 不可枚举的语法错误
#define UndefinedIdentifier    0x02 // 未定义的标识符
#define UnicodeChar            0x03 // 出现Unicode字符（不支持）
#define InvalidNumberLiteral   0x04 // 非法数字字面量
#define ExpectSeperator        0x05 // 连续符号、字面量间缺少分隔符
#define SymbolTooLong          0x06 // 符号名称过长（大于32）
#define NoLeftBracket          0x07 // 缺少匹配的左括号
#define NoRightBracket         0x08 // 缺少匹配的右括号
#define CrossedBrackets        0x09 // 不同类别的括号交叉
#define MissingComma           0x0a // 缺少逗号
#define TooManyCommas          0x0b // 存在无效多余的逗号
#define MissingOperand         0x0c // 缺少操作数
#define BeyondCharset          0x0d // 代码出现se字符集以外的字符

// TypeError
#define NonCallableObject      0x01 // 待调用对象不是函数
#define NonIndexableObject     0x02 // 待索引对象不是数组
#define NonExpandableObject    0x03 // 待解构对象不是数组
#define MathOperationAmongNonNumbers 0x04 // 对非数字对象进行数学运算
#define ModuloWithFloat        0x05 // 对浮点数进行取模
#define BitwiseOpWithFloat     0x06 // 对浮点数进行位运算

// IndexError
#define NoIndex                0x01 // 缺少下标索引
#define MissingArray           0x02 // 缺少待索引的数组
#define ExpectNonNegativeIntegerIndex 0x03 // 索引数字需要为非负整数
#define IndexOutOfRange        0x04 // 索引越界

// RuntimeError
#define ExpectFunction         0x01 // 缺少函数调用名或函数对象无效
#define BadFunctionCallArgs    0x02 // 函数调用参数列表无效
#define BadFunctionCallArgc    0x03 // 函数调用参数个数不匹配
#define BadFunctionCallArgType 0x04 // 函数调用参数类型不匹配
#define ExpandEmptyArray       0x05 // 试图解构空数组
#define AssignLeftValue        0x06 // 尝试给左值赋值
#define MathOperationWithNaNOrInf 0x07 // NaN或Inf值参与数学运算
#define IntDivOrModByZero      0x08 // 整数除法、求模以零为右操作数
#define NoAvailableID          0x09 // 运行时ID分配失败
#define BadAlloc               0x0a // 内存分配失败
#define BadSymbolInsertion     0x0b // 添加符号失败