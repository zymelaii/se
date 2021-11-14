#include <se/type.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

se_object_t wrap2obj(void *data, int type)
{
	se_object_t ret = { 0 };
	ret.type = type;
	ret.data = data;
	ret.id   = -1; // unassigned

	switch (type)
	{
		case EO_OBJ:
		{
			while (1)
			{
				se_object_t *oo = (se_object_t*)data;
				if (oo->type == EO_NIL || data == 0L)
				{
					ret.data   = 0L;
					ret.type   = EO_NIL;
					ret.is_nil = 1;
					break;
				} else if (oo->type == EO_OBJ)
				{
					data   = oo->data;
					ret.id = oo->id;
				} else
				{
					ret.data = oo->data;
					ret.type = oo->type;
					break;
				}
			}
			ret.is_ref = 1;
			break;
		}
		case EO_NUM:
		case EO_FUNC:
		case EO_ARRAY:
		{
			break;
		}
		case EO_NIL:
		default:
		{
			ret.data   = 0L;
			ret.type   = EO_NIL;
			ret.is_nil = 1;
			break;
		}
	}

	return ret;
}

const char* obj2str(se_object_t obj, char *buffer, int len)
{
	buffer[len] = '\0';
	char buf[64] = { 0 }, *p = buf;

	if (obj.is_ref)
	{
		se_function_t *fn = (se_function_t*)obj.data;
		se_array_t    *ar = (se_array_t*)obj.data;
		switch (obj.type)
		{
			case EO_NIL  : strcpy (buf, "Object<void>");                   break;
			case EO_NUM  : strcpy (buf, "Object<Number>");                 break;
			case EO_FUNC : sprintf(buf, "Object<Function<%d>>", fn->argc); break;
			case EO_ARRAY: sprintf(buf, "Object<Array<%d>>",    ar->size); break;
			default:       strcpy (buf, "Object");
		}
		return strncpy(buffer, buf, len);
	}

	switch (obj.type)
	{
		case EO_NUM:
		{
			se_number_t *num = (se_number_t*)obj.data;
			if (num->nan) return strncpy(buffer, "NaN", len);
			if (num->inf) return strncpy(buffer, "Inf", len);
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
			se_function_t *func = (se_function_t*)obj.data;
			sprintf(buf, "[Function<%d>: %s]",
				func->argc,
				(func->symbol == 0L ? "(hidden)" : func->symbol));
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
				if (ar->data[i].type == EO_ARRAY)
				{
					sprintf(buf, "Array<%d>", ((se_array_t*)ar->data[i].data)->size);
				} else
				{
					obj2str(ar->data[i], buf, 64);
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
}