#include <se/type.h>
#include <se/alloc.h>
#include <se/exception.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// ´ò°üÎªse_object_t
se_object_t wrap2obj(void *data, int type)
{
	se_object_t ret = { 0 };
	ret.data = data;
	ret.type = type;

	if (type == EO_OBJ)
	{
		assert(data != 0L);
		se_object_t *ref = data;
		assert(ref->type != EO_OBJ);
		assert(ref->id != 0);
		++ref->refs;
	} else if (type == EO_NIL)
	{
		ret.data = 0L;
		ret.is_nil = 1;
	}

	return ret;
}

se_object_t objclone(se_object_t obj)
{
	se_object_t ret = {
		.data   = 0L,
		.id     = 0,
		.type   = obj.type,
		.refs   = 0,
		.is_nil = obj.is_nil,
	};

	if (!ret.is_nil)
	{
		int size = 0;
		switch (ret.type)
		{
			case EO_NUM  : size = sizeof(se_number_t);   break;
			case EO_FUNC : size = sizeof(se_function_t); break;
			case EO_ARRAY: size = sizeof(se_array_t);    break;
		}
		void *p = se_alloc(size);
		memcpy(p, obj.data, size);
		ret.data = p;
	}

	return ret;
}

const char* obj2str(se_object_t obj, char *buffer, int len)
{
	buffer[len] = '\0';
	char buf[64] = { 0 }, *p = buf;

	switch (obj.type)
	{
		case EO_OBJ:
		{
			se_object_t *oo = (se_object_t*)obj.data;
			char reftype = '*'; // Ö»¶ÁÒýÓÃ

			if (oo->type == EO_OBJ)
			{
				oo = (se_object_t*)oo->data;
				reftype = '&'; // ¶ÁÐ´ÒýÓÃ
			}

			assert(obj.data != 0L);
			assert(oo->id   >  0 );
			assert(oo->refs >  0 );

			switch (oo->type)
			{
				case EO_NIL:
				{
					sprintf(buf, "Object<void%c%d>", reftype, oo->refs);
				}
				break;
				case EO_NUM:
				{
					sprintf(buf, "Object<Number%c%d>", reftype, oo->refs);
				}
				break;
				case EO_FUNC:
				{
					se_function_t *fn = (se_function_t*)oo->data;
					assert(fn->argc >= -1);
					if (fn->argc == -1)
					{
						sprintf(buf, "Object<Function<...>%c%d>", reftype, oo->refs);
					} else
					{
						sprintf(buf, "Object<Function<%d>%c%d>", fn->argc, reftype, oo->refs);
					}
				}
				break;
				case EO_ARRAY:
				{
					se_array_t *ar = (se_array_t*)oo->data;
					sprintf(buf, "Object<Array<%d>%c%d>", ar->size, reftype, oo->refs);
				}
				break;
				default: assert(0);
			}
			return strncpy(buffer, buf, len);
		}
		break;
		case EO_NUM:
		{
			se_number_t *num = (se_number_t*)obj.data;
			if (num->inf) return strncpy(buffer, "Inf", len);
			if (num->nan) return strncpy(buffer, "NaN", len);
			if (num->type == EN_FLT)
			{
				sprintf(p, "%g", num->f);
				return strncpy(buffer, buf, len);
			}
			int radix;
			switch (num->type)
			{
				case EN_BIN: radix = 2;  *p++ = '0'; *p++ = 'b'; break;
				case EN_OCT: radix = 8;  *p++ = '0'; break;
				case EN_DEC: radix = 10; break;
				case EN_HEX: radix = 16; *p++ = '0'; *p++ = 'x'; break;
			}
			ltoa(num->i, p, radix);
			return strncpy(buffer, buf, len);
		}
		break;
		case EO_FUNC:
		{
			se_function_t *fn = (se_function_t*)obj.data;
			const char *symbol = fn->symbol == 0L ? "(hidden)" : fn->symbol;
			assert(fn->argc >= -1);
			if (fn->argc == -1)
			{
				sprintf(buf, "[Function<...>: %s]", symbol);
			} else
			{
				sprintf(buf, "[Function<%d>: %s]", fn->argc, symbol);
			}
			return strncpy(buffer, buf, len);
		}
		break;
		case EO_ARRAY:
		{
			se_array_t *ar = (se_array_t*)obj.data;
			snprintf(buffer, len, "Array<%d>", ar->size);

			if (ar->size == 0) return buffer;

			int length = strlen(buffer), totalsize = length;

			if (totalsize + 6 > len) return buffer;

			p = buffer + length;
			p[0] = ' ';
			p[1] = '{';
			totalsize += 2, p += 2;

			int i = 0;
			for (; i < ar->size; ++i)
			{
				se_object_t *obj = &ar->data[i];

				if (obj->type == EO_ARRAY)
				{
					sprintf(buf, "Array<%d>", ((se_array_t*)obj->data)->size);
				} else
				{
					obj2str(*obj, buf, 64);
				}

				length = strlen(buf);
				if (totalsize + length + 3 >= len)
				{
					strncpy(p, " ...,", len - totalsize);
					p += 5;
					break;
				}

				*p++ = ' ';
				strcpy(p, buf);
				p[length] = ',';

				p += length + 1;
				totalsize += length;
			}

			if (*(p - 1) == ',')
			{
				*(p - 1) = ' ';
				*p++ = '}';
			}

			*p = '\0';

			return buffer;
		}
		break;
		case EO_NIL:
		default:
		{
			return strncpy(buffer, "nil", len);
		}
		break;
	}

	assert(0);
	return buffer;
}