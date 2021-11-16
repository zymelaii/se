#ifndef SE_CONTEXT_BUILD
#error action.c is only available in context.c
#endif

#include <math.h>

// 检查是否为合法数字类型
static int se_ctx_check_number(se_object_t *obj, se_object_t *placer)
{
	if (obj == 0L || placer == 0L)
	{
		se_throw(UnknownError, ArgumentErrorInCSrc, 0, 0);
		return 1;
	}

	while (obj->type == EO_OBJ)
	{
		obj = (se_object_t*)obj->data;
	}

	se_number_t *num = (se_number_t*)obj->data;
	if (obj->type != EO_NUM)
	{
		se_throw(TypeError, MathOperationAmongNonNumbers, obj->type, 0);
		return 1;
	} else if (num->nan || num->inf)
	{
		se_throw(RuntimeError, MathOperationWithNaNOrInf, num->nan, num->inf);
		return 1;
	}

	*placer = (se_object_t){ 0 };
	placer->data = obj->data;
	placer->type = obj->type;

	return 0;
}

static int se_ctx_action_assign_symbol(se_context_t *ctx, unit_t *unit)
{	// 符号分配
	assert(ctx != 0L);

	ctxmemory_t *ctxmem = (ctxmemory_t*)ctx->memory;
	assert(ctxmem != 0L);

	assert(unit != 0L);
	assert(SE_UNIT_TYPE(*unit) == T_SYMBOL);

	char *symbol = (char*)se_ctx_request(ctx, unit->len + 1);
	if (symbol == 0L)
	{
		se_throw(RuntimeError, BadAlloc, unit->len + 1, 0);
		return 1;
	}

	memcpy(symbol, unit->tok, unit->len);
	symbol[unit->len] = '\0';

	s2inode_t *pair = hashmap_find_by_key(&ctxmem->symmap, symbol);

	if (pair == 0L)
	{
		uint16_t id;
		if (se_ctx_allocid(ctx, &id) != 0)
		{
			se_throw(RuntimeError, NoAvailableID, ctxmem->prev_available_id, 0);
			return 1;
		}

		se_object_t *obj = (se_object_t*)se_ctx_request(ctx, sizeof(se_object_t));
		if (obj == 0L)
		{
			se_throw(RuntimeError, BadAlloc, sizeof(se_object_t), 0);
			return 1;
		}

		*obj = wrap2obj(0L, EO_NIL);
		obj->id = id;

		if (se_ctx_allocid(ctx, &id) != 0)
		{
			se_throw(RuntimeError, NoAvailableID, ctxmem->prev_available_id, 0);
			return 1;
		}

		pair = hashmap_insert(&ctxmem->symmap, symbol, id);
		if (pair == 0L)
		{
			se_throw(RuntimeError, BadSymbolInsertion, id, 0);
			return 1;
		}

		if (ctxmem->nilsym_size == ctxmem->nilsym_capacity)
		{
			const size_t size = sizeof(s2inode_t*) * ctxmem->nilsym_capacity;
			s2inode_t **pairs = (s2inode_t**)se_ctx_request(ctx, size * 2);
			if (pairs == 0L)
			{
				se_throw(RuntimeError, BadAlloc, size * 2, 0);
				return 1;
			}
			ctxmem->nilsym_capacity *= 2;
			memcpy(pairs, ctxmem->nilsym_pairs, size);
			se_ctx_release(ctx, ctxmem->nilsym_pairs);
			ctxmem->nilsym_pairs = pairs;
		}
		ctxmem->nilsym_pairs[ctxmem->nilsym_size++] = pair;

		if (id > ctxmem->idstorage_capacity)
		{
			const size_t size = sizeof(se_object_t) * ctxmem->idstorage_capacity;
			se_object_t *idstorage = (se_object_t*)se_ctx_request(ctx, size * 2);
			if (idstorage == 0L)
			{
				se_throw(RuntimeError, BadAlloc, size * 2, 0);
				return 1;
			}
			ctxmem->idstorage_capacity *= 2;
			memcpy(idstorage, ctxmem->idstorage, size);
			memset(idstorage + ctxmem->idstorage_capacity, 0, size);
			se_ctx_release(ctx, ctxmem->idstorage);
			ctxmem->idstorage = idstorage;
		}

		se_object_t *p = &ctxmem->idstorage[pair->id - 1];
		*p = wrap2obj(obj, EO_OBJ);
		p->id = pair->id;
		p->is_nil = 1;
	} else
	{
		se_ctx_release(ctx, symbol);
	}

	se_stack_push(&ctxmem->efs, ctxmem->idstorage[pair->id - 1]);

	return 0;
}

static int se_ctx_action_assign_number(se_context_t *ctx, unit_t *unit)
{	// 立即数分配
	assert(ctx != 0L);
	assert(unit != 0L);
	assert(SE_UNIT_TYPE(*unit) == T_NUMBER);

	ctxmemory_t *ctxmem = (ctxmemory_t*)ctx->memory;
	assert(ctxmem != 0L);

	token_t token = unit2tok(*unit);
	se_number_t num = parse_number(&token), *p;

	if (se_ctx_savetmp(ctx, &num, EO_NUM, (void**)&p) != 0)
	{
		return 1;
	}

	se_object_t obj = wrap2obj(p, EO_NUM);

	se_stack_push(&ctxmem->efs, obj);

	return 0;
}

static int se_ctx_action_bracketval(se_context_t *ctx, unit_t *unit)
{	// 括号表达式
	assert(ctx != 0L);
	assert(unit != 0L);
	assert(SE_UNIT_TYPE(*unit) == T_OPERATOR);
	assert(SE_UNIT_SUBTYPE(*unit) == OP_BRE);

	ctxmemory_t *ctxmem = (ctxmemory_t*)ctx->memory;
	assert(ctxmem != 0L);

	scopestate_t *state = &ctx->seus.ss[ctxmem->ssp--];

	if (ctxmem->efs.size <= state->sframe)
	{
		se_stack_push(&ctxmem->efs, wrap2obj(0L, EO_NIL));
	} else
	{
		int c = ctxmem->vfs.size;
		ctxmem->vfs.size -= state->accept;
		for (; c > ctxmem->vfs.size; --c)
		{
			se_ctx_mov2blc(ctx, &ctxmem->vfs.stack[c - 1]);
		}
	}

	return 0;
}

static int se_ctx_action_fncall(se_context_t *ctx, unit_t *unit)
{	// 函数调用
	assert(ctx != 0L);
	assert(unit != 0L);
	assert(SE_UNIT_TYPE(*unit) == T_OPERATOR);
	assert(SE_UNIT_SUBTYPE(*unit) == OP_ARG);

	ctxmemory_t *ctxmem = (ctxmemory_t*)ctx->memory;
	assert(ctxmem != 0L);

	scopestate_t *state = &ctx->seus.ss[ctxmem->ssp--];

	int len = ctxmem->efs.size <= state->sframe ? 0 : state->accept + 1;
	se_array_t as = { 0 };
	if (len > 0)
	{
		as.size = len;
		as.data = (se_object_t*)se_ctx_request(ctx, as.size * sizeof(se_object_t));
		as.data[as.size - 1] = se_stack_pop(&ctxmem->efs);
		for (int c = state->accept; c > 0; --c)
		{
			as.data[c - 1] = se_stack_pop(&ctxmem->vfs);
		}
	}

	se_object_t obj_fn = se_stack_pop(&ctxmem->efs);
	se_object_t *obj = &obj_fn;
	while (obj->type == EO_OBJ)
	{
		obj = (se_object_t*)obj->data;
	}

	if (obj->type == EO_FUNC)
	{
		se_function_t fn = *(se_function_t*)obj->data;

		se_stack_t args = { .stack = as.data, .size = as.size };
		se_object_t ret = se_call(fn, &args);

		se_stack_push(&ctxmem->efs, ret);

		for (int c = 0; c < args.size; ++c)
		{
			se_ctx_mov2blc(ctx, &args.stack[c]);
		}

		return !se_caught();
	} else
	{
		se_throw(TypeError, NonCallableObject, obj->type, 0);
		return 1;
	}
}

static int se_ctx_action_index(se_context_t *ctx, unit_t *unit)
{	// 数组索引
	assert(ctx != 0L);
	assert(unit != 0L);
	assert(SE_UNIT_TYPE(*unit) == T_OPERATOR);
	assert(SE_UNIT_SUBTYPE(*unit) == OP_IDX);

	ctxmemory_t *ctxmem = (ctxmemory_t*)ctx->memory;
	assert(ctxmem != 0L);

	scopestate_t *state = &ctx->seus.ss[ctxmem->ssp--];
	ctxmem->vfs.size -= state->accept;

	se_object_t obj_index = se_stack_pop(&ctxmem->efs);
	se_object_t obj_array = se_stack_pop(&ctxmem->efs);
	se_object_t *obj;

	obj = &obj_index;
	while (obj->type == EO_OBJ)
	{
		obj = (se_object_t*)obj->data;
	}
	se_number_t *index = (se_number_t*)obj->data;

	int is_index = obj->type == EO_NUM
		? index->type != EN_FLT && index->i >= 0 && !index->nan
		? 1 : 0 : 0;

	if (!is_index)
	{
		se_throw(IndexError, ExpectNonNegativeIntegerIndex, 0, 0);
		return 1;
	}

	obj = &obj_array;
	while (obj->type == EO_OBJ)
	{
		obj = (se_object_t*)obj->data;
	}
	se_array_t *array = (se_array_t*)obj->data;

	if (obj->type != EO_ARRAY)
	{
		se_throw(TypeError, NonIndexableObject, 0, 0);
		return 1;
	}

	if (index->inf || index->i >= array->size)
	{
		se_throw(IndexError, IndexOutOfRange, 0, 0);
		return 1;
	}

	se_object_t ret;
	obj = &array->data[index->i];

	int writable = obj_array.type == EO_OBJ;
	refreq_t req = se_ref_request(obj, writable);

	if (writable)
	{
		req.placer = obj;
	}
	ret = se_refer(obj, &req);

	while (obj->type == EO_OBJ)
	{
		obj = (se_object_t*)obj->data;
	}
	--obj->refs;

	if (writable)
	{
		if (se_ctx_allocid(ctx, &ret.id) != 0)
		{
			se_throw(RuntimeError, NoAvailableID, ctxmem->prev_available_id, 0);
			return 1;
		}
	}

	se_stack_push(&ctxmem->efs, ret);

	return 0;
}

static int se_ctx_action_makearray(se_context_t *ctx, unit_t *unit)
{	// 数组创建
	assert(ctx != 0L);
	assert(unit != 0L);
	assert(SE_UNIT_TYPE(*unit) == T_OPERATOR);
	assert(SE_UNIT_SUBTYPE(*unit) == OP_ARR);

	ctxmemory_t *ctxmem = (ctxmemory_t*)ctx->memory;
	assert(ctxmem != 0L);

	scopestate_t *state = &ctx->seus.ss[ctxmem->ssp--];

	int len = ctxmem->efs.size <= state->sframe ? 0 : state->accept + 1;
	se_array_t as = { 0 };
	if (len > 0)
	{
		as.size = len;
		as.data = (se_object_t*)se_ctx_request(ctx, as.size * sizeof(se_object_t));
		as.data[as.size - 1] = se_stack_pop(&ctxmem->efs);
		for (int c = state->accept; c > 0; --c)
		{
			as.data[c - 1] = se_stack_pop(&ctxmem->vfs);
		}
	}

	int c = 0;
	for (; c < len; ++c)
	{	// 转换为值引用
		se_object_t *obj = (se_object_t*)&as.data[c];
		refreq_t req = se_ref_request(obj, 0);
		if (req.placer == 0L)
		{
			req.placer = (se_object_t*)se_ctx_request(ctx, sizeof(se_object_t));
			memset(req.placer, 0, sizeof(se_object_t));
			if (se_ctx_allocid(ctx, &req.placer->id) != 0)
			{
				se_throw(RuntimeError, NoAvailableID, ctxmem->prev_available_id, 0);
				return 1;
			}
		}
		if (req.reqsize > 0)
		{
			req.reqmem = obj->data;
		}
		*obj = se_refer(obj, &req);
		if (se_ctx_allocid(ctx, &obj->id) != 0)
		{
			se_throw(RuntimeError, NoAvailableID, ctxmem->prev_available_id, 0);
			return 1;
		}
	}

	se_array_t *array = (se_array_t*)se_ctx_request(ctx, sizeof(se_array_t));
	if (array == 0L)
	{
		se_throw(RuntimeError, BadAlloc, sizeof(se_array_t), 0);
		return 1;
	}
	*array = as;

	se_object_t ret = wrap2obj(array, EO_ARRAY);

	se_stack_push(&ctxmem->efs, ret);

	return 0;
}

static int se_ctx_action_assign(se_context_t *ctx, unit_t *unit)
{	// 赋值
	assert(ctx != 0L);
	assert(unit != 0L);
	assert(SE_UNIT_TYPE(*unit) == T_OPERATOR);
	assert(SE_UNIT_SUBTYPE(*unit) == OP_ASS);

	ctxmemory_t *ctxmem = (ctxmemory_t*)ctx->memory;
	assert(ctxmem != 0L);

	se_object_t rhs = se_stack_pop(&ctxmem->efs);
	se_object_t lhs = se_stack_pop(&ctxmem->efs);

	if (lhs.type != EO_OBJ || lhs.id == 0)
	{
		se_throw(RuntimeError, AssignLeftValue, 0, 0);
		return 1;
	}

	se_object_t *obj = &lhs;
	if (is_writable(obj))
	{
		obj = (se_object_t*)obj->data;
	}

	se_object_t *ref = obj;
	while (ref->type == EO_OBJ)
	{
		ref = (se_object_t*)ref->data;
	}

	if (lhs.id != rhs.id)
	{
		int this_id = lhs.id;
		--ref->refs;
		refreq_t req = se_ref_request(&rhs, 0);
		if (req.placer == 0L)
		{
			req.placer = (se_object_t*)se_ctx_request(ctx, sizeof(se_object_t));
			memset(req.placer, 0, sizeof(se_object_t));
			if (se_ctx_allocid(ctx, &req.placer->id) != 0)
			{
				se_throw(RuntimeError, NoAvailableID, ctxmem->prev_available_id, 0);
				return 1;
			}
		}
		if (req.reqsize > 0)
		{
			req.reqmem = rhs.data;
		}
		int obj_id = obj->id;
		*obj = se_refer(&rhs, &req);
		obj->id = obj_id;
		if (!req.writable)
		{
			lhs.id = this_id;
			if (lhs.is_nil)
			{
				lhs.is_nil = 0;
			}
			ctxmem->idstorage[lhs.id - 1] = lhs;
		}
	} else
	{
		lhs = rhs;
	}

	se_stack_push(&ctxmem->efs, lhs);

	return 0;
}

static int se_ctx_action_mov2vfs(se_context_t *ctx, unit_t *unit)
{	// 逗号表达式
	assert(ctx != 0L);
	assert(unit != 0L);
	assert(SE_UNIT_TYPE(*unit) == T_OPERATOR);
	assert(SE_UNIT_SUBTYPE(*unit) == OP_CME);

	ctxmemory_t *ctxmem = (ctxmemory_t*)ctx->memory;
	assert(ctxmem != 0L);

	scopestate_t *state = &ctx->seus.ss[ctxmem->ssp];
	se_stack_push(&ctxmem->vfs, ctxmem->efs.stack[state->sframe]);

	int c = state->sframe;
	for (; c < ctxmem->efs.size - 1; ++c)
	{
		ctxmem->efs.stack[c] = ctxmem->efs.stack[c + 1];
	}

	--ctxmem->efs.size;
	++state->accept;

	return 0;
}

static int se_ctx_action_exparray(se_context_t *ctx, unit_t *unit)
{	// 数组解构（待实现）
	assert(ctx != 0L);
	assert(unit != 0L);
	assert(SE_UNIT_TYPE(*unit) == T_OPERATOR);
	assert(SE_UNIT_SUBTYPE(*unit) == OP_EPA);

	ctxmemory_t *ctxmem = (ctxmemory_t*)ctx->memory;
	assert(ctxmem != 0L);

	se_object_t lhs = se_stack_pop(&ctxmem->efs), *obj = &lhs;

	while (obj->type == EO_OBJ)
	{
		obj = (se_object_t*)obj->data;
	}

	if (obj->type != EO_ARRAY)
	{
		se_throw(TypeError, NonExpandableObject, obj->type, 0);
		return 1;
	}

	se_array_t *array = (se_array_t*)obj->data;
	if (array->size == 0)
	{
		se_throw(RuntimeError, ExpandEmptyArray, 0, 0);
		return 1;
	}

	scopestate_t *state = &ctx->seus.ss[ctxmem->ssp];
	se_stack_push(&ctxmem->efs, array->data[array->size - 1]);
	for (int c = 0; c < array->size - 1; ++c)
	{
		se_stack_push(&ctxmem->vfs, array->data[c]);
	}
	state->accept += array->size - 1;

	return 0;
}

static int se_ctx_action_sign(se_context_t *ctx, unit_t *unit)
{	// 正负符号
	assert(ctx != 0L);
	assert(unit != 0L);
	assert(SE_UNIT_TYPE(*unit) == T_OPERATOR);
	assert(SE_UNIT_SUBTYPE(*unit) == OP_PL
		|| SE_UNIT_SUBTYPE(*unit) == OP_NL);

	ctxmemory_t *ctxmem = (ctxmemory_t*)ctx->memory;
	assert(ctxmem != 0L);

	se_object_t lhs = se_stack_pop(&ctxmem->efs), obj_x;

	if (se_ctx_check_number(&lhs, &obj_x) != 0)
	{
		return 1;
	}

	se_number_t *x;
	if (se_ctx_savetmp(ctx, obj_x.data, EO_NUM, (void**)&x) != 0)
	{
		return 1;
	}

	if (SE_UNIT_SUBTYPE(*unit) == OP_NL)
	{
		if (x->type == EN_FLT)
		{
			x->f = -x->f;
		} else
		{
			x->inf = x->i < 0 && -x->i < 0;
			x->i = -x->i;
		}
	}

	se_stack_push(&ctxmem->efs, wrap2obj(x, EO_NUM));

	return 0;
}

static int se_ctx_action_basecalc(se_context_t *ctx, unit_t *unit)
{	// 基础五则运算
	assert(ctx != 0L);
	assert(unit != 0L);
	assert(SE_UNIT_TYPE(*unit) == T_OPERATOR);
	assert(SE_UNIT_SUBTYPE(*unit) == OP_ADD
		|| SE_UNIT_SUBTYPE(*unit) == OP_SUB
		|| SE_UNIT_SUBTYPE(*unit) == OP_MOD
		|| SE_UNIT_SUBTYPE(*unit) == OP_MUL
		|| SE_UNIT_SUBTYPE(*unit) == OP_DIV);

	ctxmemory_t *ctxmem = (ctxmemory_t*)ctx->memory;
	assert(ctxmem != 0L);

	se_object_t rhs = se_stack_pop(&ctxmem->efs), obj_y;
	se_object_t lhs = se_stack_pop(&ctxmem->efs), obj_x;

	if (se_ctx_check_number(&lhs, &obj_x) != 0)
	{
		return 1;
	}

	if (se_ctx_check_number(&rhs, &obj_y) != 0)
	{
		return 1;
	}

	se_number_t *x = (se_number_t*)obj_x.data;
	se_number_t *y = (se_number_t*)obj_y.data;

	const int op = SE_UNIT_SUBTYPE(*unit);
	int useflt = x->type == EN_FLT || y->type == EN_FLT;

	if (useflt && op == OP_MOD)
	{
		se_throw(TypeError, ModuloWithFloat, 0, 0);
		return 1;
	}

	int32_t ix = x->i;
	int32_t iy = y->i;
	double  fx = x->type == EN_FLT ? x->f : ix * 1.0;
	double  fy = y->type == EN_FLT ? y->f : iy * 1.0;

	int32_t ri;
	double  rf;

	int inf = 0;

	switch (op)
	{
		case OP_ADD:
		{
			if (useflt)
			{
				rf = fx + fy;
			} else
			{
				ri = ix + iy;
				inf = (ix & iy & ~ri) >> 31;
			}
		}
		break;
		case OP_SUB:
		{
			if (useflt)
			{
				rf = fx - fy;
			} else
			{
				ri = ix - iy;
				inf = (ix & ~iy & ~ri) >> 31;
			}
		}
		break;
		case OP_MOD:
		{
			if (iy == 0)
			{
				se_throw(RuntimeError, IntDivOrModByZero, 0, 0);
				return 1;
			}
			ri = ix % iy;
		}
		break;
		case OP_MUL:
		{
			if (useflt)
			{
				rf = fx * fy;
			} else
			{
				ri = ix * iy;
				if (iy != 0)
				{
					inf = ri / iy != ix;
				}
			}
		}
		break;
		case OP_DIV:
		{
			if (!useflt && iy == 0)
			{
				se_throw(RuntimeError, IntDivOrModByZero, 0, 0);
				return 1;
			}
			if (useflt)
			{
				rf = fx / fy;
			} else
			{
				ri = ix / iy;
			}
		}
		break;
	}

	se_number_t result, *ret;
	if (useflt)
	{
		result.f = rf;
		result.type = EN_FLT;
		result.inf = isinf(rf);
		result.nan = isnan(rf);
	} else
	{
		result.i = ri;
		result.type = EN_DEC;
		result.inf = inf;
		result.nan = 0;
	}

	if (se_ctx_savetmp(ctx, &result, EO_NUM, (void**)&ret) != 0)
	{
		return 1;
	}

	se_stack_push(&ctxmem->efs, wrap2obj(ret, EO_NUM));

	return 0;
}

static int se_ctx_action_compare(se_context_t *ctx, unit_t *unit)
{	// 二元逻辑运算
	assert(ctx != 0L);
	assert(unit != 0L);
	assert(SE_UNIT_TYPE(*unit) == T_OPERATOR);
	assert(SE_UNIT_SUBTYPE(*unit) == OP_GTR
		|| SE_UNIT_SUBTYPE(*unit) == OP_GEQ
		|| SE_UNIT_SUBTYPE(*unit) == OP_LSS
		|| SE_UNIT_SUBTYPE(*unit) == OP_LEQ
		|| SE_UNIT_SUBTYPE(*unit) == OP_EQU
		|| SE_UNIT_SUBTYPE(*unit) == OP_NEQ
		|| SE_UNIT_SUBTYPE(*unit) == OP_LAND
		|| SE_UNIT_SUBTYPE(*unit) == OP_LOR);

	ctxmemory_t *ctxmem = (ctxmemory_t*)ctx->memory;
	assert(ctxmem != 0L);

	se_object_t rhs = se_stack_pop(&ctxmem->efs);
	se_object_t lhs = se_stack_pop(&ctxmem->efs);

	se_object_t obj_x, obj_y;

	if (se_ctx_check_number(&lhs, &obj_x) != 0)
	{
		return 1;
	}

	if (se_ctx_check_number(&rhs, &obj_y) != 0)
	{
		return 1;
	}

	se_number_t *x = (se_number_t*)obj_x.data, *y = (se_number_t*)obj_y.data;
	se_number_t result, *ret;
	result.type = EN_DEC;
	result.inf  = 0;
	result.nan  = 0;

	switch (SE_UNIT_SUBTYPE(*unit))
	{
#define CMP(L, OP, R) \
(	((L)->type == EN_FLT ? (L)->f : (L)->i) OP  \
	((R)->type == EN_FLT ? (R)->f : (R)->i)	)
		case OP_GTR:  result.i = CMP(x,  >, y); break;
		case OP_GEQ:  result.i = CMP(x, >=, y); break;
		case OP_LSS:  result.i = CMP(x,  <, y); break;
		case OP_LEQ:  result.i = CMP(x, <=, y); break;
		case OP_EQU:  result.i = CMP(x, ==, y); break;
		case OP_NEQ:  result.i = CMP(x, !=, y); break;
		case OP_LAND: result.i = CMP(x, &&, y); break;
		case OP_LOR:  result.i = CMP(x, ||, y); break;
#undef CMP
	}

	if (se_ctx_savetmp(ctx, &result, EO_NUM, (void**)&ret) != 0)
	{
		return 1;
	}

	se_stack_push(&ctxmem->efs, wrap2obj(ret, EO_NUM));

	return 0;
}

static int se_ctx_action_logical_not(se_context_t *ctx, unit_t *unit)
{	// 逻辑非
	assert(ctx != 0L);
	assert(unit != 0L);
	assert(SE_UNIT_TYPE(*unit) == T_OPERATOR);
	assert(SE_UNIT_SUBTYPE(*unit) == OP_LNOT);

	ctxmemory_t *ctxmem = (ctxmemory_t*)ctx->memory;
	assert(ctxmem != 0L);

	se_object_t lhs = se_stack_pop(&ctxmem->efs);
	se_object_t obj_x;

	if (se_ctx_check_number(&lhs, &obj_x) != 0)
	{
		return 1;
	}

	se_number_t *x = (se_number_t*)obj_x.data, *ret;
	if (se_ctx_savetmp(ctx, x, EO_NUM, (void**)&ret) != 0)
	{
		return 1;
	}

	ret->i = x->type == EN_FLT ? !x->f : !x->i;
	ret->type = EN_DEC;
	ret->inf  = 0;
	ret->nan  = 0;

	se_stack_push(&ctxmem->efs, wrap2obj(ret, EO_NUM));

	return 0;
}

static int se_ctx_action_bitwise_not(se_context_t *ctx, unit_t *unit)
{	// 按位取反
	assert(ctx != 0L);
	assert(unit != 0L);
	assert(SE_UNIT_TYPE(*unit) == T_OPERATOR);
	assert(SE_UNIT_SUBTYPE(*unit) == OP_NOT);

	ctxmemory_t *ctxmem = (ctxmemory_t*)ctx->memory;
	assert(ctxmem != 0L);

	se_object_t lhs = se_stack_pop(&ctxmem->efs);
	se_object_t obj_x;

	if (se_ctx_check_number(&lhs, &obj_x) != 0)
	{
		return 1;
	}

	se_number_t *x = (se_number_t*)obj_x.data, *ret;
	if (x->type == EN_FLT)
	{
		se_throw(TypeError, BitwiseOpWithFloat, 0, 0);
		return 1;
	}

	if (se_ctx_savetmp(ctx, x, EO_NUM, (void**)&ret) != 0)
	{
		return 1;
	}

	ret->i    = ~x->i;
	ret->type = x->type;
	ret->inf  = 0;
	ret->nan  = 0;

	se_stack_push(&ctxmem->efs, wrap2obj(ret, EO_NUM));

	return 0;
}

static int se_ctx_action_binary_bitop(se_context_t *ctx, unit_t *unit)
{	// 二元位运算
	assert(ctx != 0L);
	assert(unit != 0L);
	assert(SE_UNIT_TYPE(*unit) == T_OPERATOR);
	assert(SE_UNIT_SUBTYPE(*unit) == OP_LSH
		|| SE_UNIT_SUBTYPE(*unit) == OP_RSH
		|| SE_UNIT_SUBTYPE(*unit) == OP_AND
		|| SE_UNIT_SUBTYPE(*unit) == OP_XOR
		|| SE_UNIT_SUBTYPE(*unit) == OP_OR);

	ctxmemory_t *ctxmem = (ctxmemory_t*)ctx->memory;
	assert(ctxmem != 0L);

	se_object_t rhs = se_stack_pop(&ctxmem->efs), obj_y;
	se_object_t lhs = se_stack_pop(&ctxmem->efs), obj_x;

	if (se_ctx_check_number(&lhs, &obj_x) != 0)
	{
		return 1;
	}

	if (se_ctx_check_number(&rhs, &obj_y) != 0)
	{
		return 1;
	}

	se_number_t *x = (se_number_t*)obj_x.data;
	se_number_t *y = (se_number_t*)obj_y.data;

	if (x->type == EN_FLT)
	{
		se_throw(TypeError, BitwiseOpWithFloat, 0, 0);
		return 1;
	}

	if (y->type == EN_FLT)
	{
		se_throw(TypeError, BitwiseOpWithFloat, 0, 0);
		return 1;
	}

	int32_t t;
	switch (SE_UNIT_SUBTYPE(*unit))
	{
		case OP_LSH: t = x->i << y->i; break;
		case OP_RSH: t = x->i >> y->i; break;
		case OP_AND: t = x->i  & y->i; break;
		case OP_XOR: t = x->i  ^ y->i; break;
		case OP_OR:  t = x->i  | y->i; break;
	}

	se_number_t result, *ret;
	result.i    = t;
	result.type = x->type;
	result.inf  = 0;
	result.nan  = 0;

	if (se_ctx_savetmp(ctx, &result, EO_NUM, (void**)&ret) != 0)
	{
		return 1;
	}

	se_stack_push(&ctxmem->efs, wrap2obj(ret, EO_NUM));

	return 0;
}

static int se_ctx_action_calc_and_ass(se_context_t *ctx, unit_t *unit)
{	// 运算后赋值
	assert(ctx != 0L);
	assert(unit != 0L);
	assert(SE_UNIT_TYPE(*unit) == T_OPERATOR);
	assert(SE_UNIT_SUBTYPE(*unit) == OP_ADD_ASS
		|| SE_UNIT_SUBTYPE(*unit) == OP_SUB_ASS
		|| SE_UNIT_SUBTYPE(*unit) == OP_MOD_ASS
		|| SE_UNIT_SUBTYPE(*unit) == OP_MUL_ASS
		|| SE_UNIT_SUBTYPE(*unit) == OP_DIV_ASS
		|| SE_UNIT_SUBTYPE(*unit) == OP_LSH_ASS
		|| SE_UNIT_SUBTYPE(*unit) == OP_RSH_ASS
		|| SE_UNIT_SUBTYPE(*unit) == OP_AND_ASS
		|| SE_UNIT_SUBTYPE(*unit) == OP_XOR_ASS
		|| SE_UNIT_SUBTYPE(*unit) == OP_OR_ASS);

	ctxmemory_t *ctxmem = (ctxmemory_t*)ctx->memory;
	assert(ctxmem != 0L);

	se_object_t lhs = ctxmem->efs.stack[ctxmem->efs.size - 2];

	unit_t dummy;

	int subtype;
	switch (SE_UNIT_SUBTYPE(*unit))
	{
		case OP_ADD_ASS: subtype = OP_ADD; break;
		case OP_SUB_ASS: subtype = OP_SUB; break;
		case OP_MOD_ASS: subtype = OP_MOD; break;
		case OP_MUL_ASS: subtype = OP_MUL; break;
		case OP_DIV_ASS: subtype = OP_DIV; break;
		case OP_LSH_ASS: subtype = OP_LSH; break;
		case OP_RSH_ASS: subtype = OP_RSH; break;
		case OP_AND_ASS: subtype = OP_AND; break;
		case OP_XOR_ASS: subtype = OP_XOR; break;
		case OP_OR_ASS : subtype = OP_OR;  break;
	}

	dummy = tok2unit((token_t){ .type = T_OPERATOR, .sub_type = subtype });
	if (subtype <= OP_SUB)
	{
		if  (se_ctx_action_basecalc(ctx, &dummy) != 0)
		{
			return 1;
		}
	} else if (se_ctx_action_binary_bitop(ctx, &dummy) != 0)
	{
		return 1;
	}

	se_object_t rhs = se_stack_pop(&ctxmem->efs);
	se_stack_push(&ctxmem->efs, lhs);
	se_stack_push(&ctxmem->efs, rhs);

	dummy = tok2unit((token_t){ .type = T_OPERATOR, .sub_type = OP_ASS });
	if (se_ctx_action_assign(ctx, &dummy) != 0)
	{
		return 1;
	}

	return 0;
}