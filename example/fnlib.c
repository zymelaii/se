#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

se_context_t *__CONTEXT__;

#define PICKNUM(NAME)                                          \
se_number_t NAME;                                              \
{                                                              \
	se_object_t tmp = se_stack_pop(args), *_ = &tmp;           \
	while (_->type == EO_OBJ)                                  \
	{                                                          \
		_ = (se_object_t*)_->data;                             \
	}                                                          \
	if (_->type != EO_NUM)                                     \
	{                                                          \
		se_throw(RuntimeError, BadFunctionCallArgType,         \
			(uint64_t)EO_NUM << 32 | _->type, 0);              \
		return wrap2obj(0L, EO_NIL);                           \
	}                                                          \
	NAME = *(se_number_t*)_->data;                             \
}

#define FN_1NUM_1NUM(NAME, FNSRC)                              \
se_object_t sefnlib_##NAME(se_stack_t *args)                   \
{                                                              \
	PICKNUM(x);                                                \
	se_number_t y = parse_flt_number(FNSRC(                    \
		x.type == EN_FLT ? x.f : x.i * 1.0));                  \
	se_number_t *ret;                                          \
	se_ctx_savetmp(__CONTEXT__, &y, EO_NUM, (void**)&ret);     \
	return wrap2obj(ret, EO_NUM);                              \
}

FN_1NUM_1NUM(sin,  sin);
FN_1NUM_1NUM(cos,  cos);
FN_1NUM_1NUM(tan,  tan);
FN_1NUM_1NUM(exp,  exp);
FN_1NUM_1NUM(asin, asin);
FN_1NUM_1NUM(acos, acos);
FN_1NUM_1NUM(atan, atan);

se_object_t sefnlib_id(se_stack_t *args)
{
	se_object_t obj = se_stack_pop(args);
	se_number_t y = parse_int_number(obj.id, EN_DEC);
	se_number_t *ret;
	se_ctx_savetmp(__CONTEXT__, &y, EO_NUM, (void**)&ret);
	return wrap2obj(ret, EO_NUM);
}

se_object_t sefnlib_int(se_stack_t *args)
{
	PICKNUM(x);
	if (x.type == EN_FLT)
	{
		x.i = (int32_t)x.f;
	}
	x.type = EN_DEC;
	se_number_t *ret;
	se_ctx_savetmp(__CONTEXT__, &x, EO_NUM, (void**)&ret);
	return wrap2obj(ret, EO_NUM);
}

se_object_t sefnlib_floor(se_stack_t *args)
{
	PICKNUM(x);
	if (x.type == EN_FLT)
	{
		x.i = (int32_t)floor(x.f);
	}
	x.type = EN_DEC;
	se_number_t *ret;
	se_ctx_savetmp(__CONTEXT__, &x, EO_NUM, (void**)&ret);
	return wrap2obj(ret, EO_NUM);
}

se_object_t sefnlib_ceil(se_stack_t *args)
{
	PICKNUM(x);
	if (x.type == EN_FLT)
	{
		x.i = (int32_t)ceil(x.f);
	}
	x.type = EN_DEC;
	se_number_t *ret;
	se_ctx_savetmp(__CONTEXT__, &x, EO_NUM, (void**)&ret);
	return wrap2obj(ret, EO_NUM);
}

se_object_t sefnlib_factorial(se_stack_t *args)
{
	PICKNUM(x);

	if (x.type == EN_FLT || x.i < 0)
	{
		se_throw(RuntimeError, BadFunctionCallArgType,
			(uint64_t)EO_NUM << 32 | EO_NUM, x.type);
		return wrap2obj(0L, EO_NIL);
	}

	double result = 1.;
	for (int i = 2; i <= x.i; ++i)
	{
		result *= i;
	}

	se_number_t y = parse_flt_number(result);
	y.inf = isinf(y.f);

	se_number_t *ret;
	se_ctx_savetmp(__CONTEXT__, &y, EO_NUM, (void**)&ret);

	return wrap2obj(ret, EO_NUM);
}

se_object_t sefnlib_sum(se_stack_t *args)
{
	if (args->size == 0)
	{
		se_throw(RuntimeError, BadFunctionCallArgc, 0, 0);
		return wrap2obj(0L, EO_NIL);
	}

	int count = args->size;
	double fsum = 0.;
	for (int i = 0; i < count; ++i)
	{
		PICKNUM(x);
		fsum += x.type == EN_FLT ? x.f : x.i * 1.0;
	}

	se_number_t y, *ret;
	if (fsum == (int32_t)(fsum) * 1.0)
	{
		y = parse_int_number((int32_t)fsum, EN_DEC);
	} else
	{
		y = parse_flt_number(fsum);
	}

	se_ctx_savetmp(__CONTEXT__, &y, EO_NUM, (void**)&ret);

	return wrap2obj(ret, EO_NUM);
}

se_object_t sefnlib_mul(se_stack_t *args)
{
	if (args->size == 0)
	{
		se_throw(RuntimeError, BadFunctionCallArgc, 0, 0);
		return wrap2obj(0L, EO_NIL);
	}

	int count = args->size;
	double fsum = 1.;
	for (int i = 0; i < count; ++i)
	{
		PICKNUM(x);
		fsum *= x.type == EN_FLT ? x.f : x.i * 1.0;
	}

	se_number_t y, *ret;
	if (fsum == (int32_t)(fsum) * 1.0)
	{
		y = parse_int_number((int32_t)fsum, EN_DEC);
	} else
	{
		y = parse_flt_number(fsum);
	}

	se_ctx_savetmp(__CONTEXT__, &y, EO_NUM, (void**)&ret);

	return wrap2obj(ret, EO_NUM);
}

se_object_t sefnlib_random(se_stack_t *args)
{
	se_number_t x = parse_flt_number(rand() * 1.0 / RAND_MAX), *ret;
	se_ctx_savetmp(__CONTEXT__, &x, EO_NUM, (void**)&ret);
	return wrap2obj(ret, EO_NUM);
}

void import_all(se_context_t *ctx)
{
	__CONTEXT__ = ctx;

	srand(time(0L));

#define IMPORT(NAME, FUNC, ARGC)                               \
{                                                              \
	char *ss = (char*)se_ctx_request(ctx, strlen(NAME) + 1);   \
	strcpy(ss, NAME);                                          \
	se_function_t fn = { FUNC, ss, ARGC };                     \
	se_ctx_bind(ctx, &fn, EO_FUNC, ss);                        \
}

	IMPORT("id",   sefnlib_id,   1);
	IMPORT("int",  sefnlib_int,  1);
	IMPORT("floor",sefnlib_floor,1);
	IMPORT("ceil", sefnlib_ceil, 1);
	IMPORT("sin",  sefnlib_sin,  1);
	IMPORT("cos",  sefnlib_cos,  1);
	IMPORT("tan",  sefnlib_tan,  1);
	IMPORT("exp",  sefnlib_exp,  1);
	IMPORT("asin", sefnlib_asin, 1);
	IMPORT("acos", sefnlib_acos, 1);
	IMPORT("atan", sefnlib_atan, 1);
	IMPORT("factorial", sefnlib_factorial, 1);
	IMPORT("sum", sefnlib_sum, -1);
	IMPORT("mul", sefnlib_mul, -1);
	IMPORT("random", sefnlib_random, 0);

#undef IMPORT
}