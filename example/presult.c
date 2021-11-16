#include <stdio.h>

void se_ctx_print_result(se_context_t *ctx)
{
	if (ctx->state != ECTX_DONE) return;

	char buf[64];
	const se_object_t *ret = se_ctx_get_last_ret(ctx);
	if (ret == 0L)
	{
		printf("nil\n");
		return;
	}

	while (ret->type == EO_OBJ)
	{
		ret = (se_object_t*)ret->data;
	}

	if (ret->type != EO_ARRAY)
	{
		printf("%s\n", obj2str(*ret, buf, 64));
		return;
	}

	se_array_t *array = (se_array_t*)ret->data;
	if (array->size == 0)
	{
		printf("%s\n", obj2str(*ret, buf, 64));
		return;
	}

	se_array_t tmp;
	tmp.data = (se_object_t*)se_ctx_request(ctx, sizeof(se_object_t) * array->size);
	tmp.size = array->size;
	for (int i = 0; i < array->size; ++i)
	{
		se_object_t *obj = &array->data[i];
		while (obj->type == EO_OBJ)
		{
			obj = (se_object_t*)obj->data;
		}
		tmp.data[i] = *obj;
	}
	printf("%s\n", obj2str(wrap2obj(&tmp, EO_ARRAY), buf, 64));
	se_ctx_release(ctx, tmp.data);
}