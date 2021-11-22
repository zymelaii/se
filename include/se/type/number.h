#pragma once

#include <se/token.h>
#include <stdint.h>
#include <stddef.h>

// eNumber
#define EN_BIN T_NUM_BIN
#define EN_OCT T_NUM_OCT
#define EN_DEC T_NUM_DEC
#define EN_HEX T_NUM_HEX
#define EN_FLT T_NUM_FLT

typedef struct number_s
{
	union
	{
		int32_t i;
	  	double  f;
	};
	uint16_t type;
	uint8_t  nan;
	uint8_t  inf;
} se_number_t;

#ifdef __cplusplus
extern "C" {
#endif

se_number_t parse_int_number(int32_t x, int type);
se_number_t parse_flt_number(double x);
se_number_t parse_number(const token_t *pt);

#ifdef __cplusplus
}
#endif