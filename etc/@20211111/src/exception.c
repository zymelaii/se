#include <se/exception.h>

static se_exception_t g_exception = { 0 };

void se_throw(uint32_t etype, uint32_t error,
	uint64_t extra, uint64_t reserved)
{
	if (etype > 0 && error > 0)
	{
		g_exception = (se_exception_t){
			.etype    = etype,
			.error    = error,
			.extra    = extra,
			.reserved = reserved,
		};
	}
}

int se_caught()
{
	return g_exception.etype == 0;
}

int se_catch(se_exception_t *e, uint32_t type)
{
	if (se_caught()) return 0;
	if (e != 0L && type == g_exception.etype)
	{
		*e = g_exception;
		g_exception = (se_exception_t){ 0 };
		return 1;
	}
	return 0;
}

int se_catch_err(se_exception_t *e, uint32_t type, uint32_t error)
{
	if (se_caught()) return 0;
	if (e != 0L && type == g_exception.etype && error == g_exception.error)
	{
		*e = g_exception;
		g_exception = (se_exception_t){ 0 };
		return 1;
	}
	return 0;
}

int se_catch_any(se_exception_t *e)
{
	if (se_caught()) return 0;
	if (e != 0L)
	{
		*e = g_exception;
		g_exception = (se_exception_t){ 0 };
		return 1;
	}
	return 0;
}