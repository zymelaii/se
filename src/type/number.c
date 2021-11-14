#include <se/type.h>
#include <se/token.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

se_number_t parse_int_number(int32_t x, int type)
{
	se_number_t ret = { 0 };

	if (   type != EN_BIN
		&& type != EN_OCT
		&& type != EN_DEC
		&& type != EN_HEX)
	{
		type = EN_DEC;
	}

	ret.type = type;
	ret.i    = x;

	return ret;
}

se_number_t parse_flt_number(double x)
{
	se_number_t ret = { 0 };
	ret.type = EN_FLT;
	ret.f    = x;
	ret.inf  = isinf(ret.f);
	ret.nan  = isnan(ret.f);
	return ret;
}

se_number_t parse_number(const token_t *pt)
{
	se_number_t ret = { 0 };

	if (pt->type != T_NUMBER || pt->q == 0L)
	{
		ret.type = EN_DEC;
		ret.nan  = 1;
	} else
	{
		const int len = pt->q - pt->p + 1;
		char *s = (char*)calloc(len + 1, sizeof(char));
		memcpy(s, pt->p, len);
		s[len] = '\0';
		if (pt->sub_type == T_NUM_FLT || pt->sub_type == T_NUM_EEEE)
		{
			ret.f    = strtod(s, 0L);
			ret.inf  = isinf(ret.f);
			ret.type = EN_FLT;
		} else
		{
			int radix, offset;
			switch (pt->sub_type)
			{
				case EN_BIN: radix = 2;  offset = 2; break;
				case EN_OCT: radix = 8;  offset = 1; break;
				case EN_DEC: radix = 10; offset = 0; break;
				case EN_HEX: radix = 16; offset = 2; break;
			}
			ret.i    = strtol(s + offset, 0L, radix);
			ret.inf  = ret.i == LONG_MAX && errno == ERANGE;
			ret.type = pt->sub_type;
		}
	}

	return ret;
}