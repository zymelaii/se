#include <se/ref.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

refreq_t se_ref_request(se_object_t *obj, int writable)
{
	assert(obj != 0L);

	refreq_t req = { 0 };
	req.writable = !!writable;

	if (is_writable(obj))
	{
		obj = (se_object_t*)obj->data;
	}

	if (is_readonly(obj))
	{
		req.placer = req.writable ? obj : (se_object_t*)obj->data;
		req.reqmem = req.placer->data;
	} else
	{
		assert(!req.writable);
		switch (obj->type)
		{
			case EO_NUM  : req.reqsize = sizeof(se_number_t);   break;
			case EO_FUNC : req.reqsize = sizeof(se_function_t); break;
			case EO_ARRAY: req.reqsize = sizeof(se_array_t);    break;
		}
	}

	return req;
}

se_object_t se_refer(se_object_t *obj, refreq_t *req)
{
	assert(obj != 0L);
	assert(req != 0L);
	assert(req->placer != 0L);
	assert(!(req->reqsize > 0 && req->reqmem == 0L));
	assert(req->placer->id != 0);

	if (is_writable(obj))
	{
		obj = (se_object_t*)obj->data;
	}

	if (is_readonly(obj))
	{
		obj = (se_object_t*)obj->data;
	}
	
	if (!req->writable)
	{
		req->placer->data   = req->reqmem;
		req->placer->type   = obj->type;
		req->placer->refs   = obj->refs;
		req->placer->is_nil = obj->is_nil;

		if (req->reqmem != 0L && req->reqmem != obj->data)
		{
			memcpy(req->reqmem, obj->data, req->reqsize);
		}

		++req->placer->refs;
	} else
	{
		++obj->refs;
	}

	se_object_t ref =
	{
		.data   = req->placer,
		.type   = EO_OBJ,
		.refs   = req->writable,
		.is_nil = 0,
	};

	return ref;
}