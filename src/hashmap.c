#ifndef SE_CONTEXT_BUILD
#error hashmap.c is only available in context.c
#endif

// SDBMHash function
static inline uint32_t hash(const char *s)
{
    uint32_t _hash = 0;
	while (*s)
	{
    	_hash = (*s++) + (_hash << 6) + (_hash << 16) - _hash;
	}
    return (_hash & 0x7fffffff);
}

// 插入哈希表，键存在时覆盖
static s2inode_t* hashmap_insert(hashmap_t *map, const char *s, uint16_t id)
{
	assert(map != 0L);
	assert(map->table != 0L);
	assert(s != 0L);
	assert(*s != '\0');

	uint32_t _hash = hash(s) % g_hashmap_size[map->size_id];
	s2inode_t *node = map->table + _hash;

	if (node->str == 0L)
	{	// 表头为空，直接写入
		node->str  = s;
		node->id   = id;
		node->next = 0L;
		++map->used;
		return node;
	}

	while (node->next != 0L)
	{
		if (strcmp(node->next->str, s) == 0)
		{	// 键存在，覆写值
			node->next->id = id;
			return node->next;
		}
		node = node->next;
	}

	node->next = (s2inode_t*)se_alloc(sizeof(s2inode_t));
	assert(node->next != 0L);
	node = node->next;

	node->str  = s;
	node->id   = id;
	node->next = 0L;

	return node;
}

static void hashmap_remove(hashmap_t *map, const char *s)
{
	assert(map != 0L);
	assert(map->table != 0L);

	if (s == 0L) return;
	if (*s == '\0') return;

	uint32_t _hash = hash(s) % g_hashmap_size[map->size_id];
	s2inode_t *node = map->table + _hash;

	if (strcmp(node->str, s) == 0)
	{	// 表头为空，直接写入
		if (node->next != 0L)
		{
			s2inode_t *tmp = node->next;
			node->str  = tmp->str;
			node->id   = tmp->id;
			node->next = node->next->next;
			se_free(tmp);
		} else
		{
			node->str = 0L;
			node->id  = 0;
			--map->used;
		}
		return;
	}

	while (node->next != 0L)
	{
		if (strcmp(node->next->str, s) == 0)
		{
			s2inode_t *tmp = node->next;
			node->next = node->next->next;
			se_free(tmp);
			return;
		}
		node = node->next;
	}
}

static s2inode_t* hashmap_find_by_key(hashmap_t *map, const char *s)
{
	assert(map != 0L);
	assert(map->table != 0L);

	if (s == 0L) return 0L;
	if (*s == '\0') return 0L;

	uint32_t _hash = hash(s) % g_hashmap_size[map->size_id];
	s2inode_t *node = map->table + _hash;

	if (node->str == 0L) return 0L;

	while (node != 0L)
	{
		if (strcmp(node->str, s) == 0)
		{
			return node;
		}
		node = node->next;
	}

	return 0L;
}

static s2inode_t* hashmap_find_by_value(hashmap_t *map, uint16_t id)
{
	assert(map != 0L);
	assert(map->table != 0L);

	int i = -1, bound = g_hashmap_size[map->size_id];

	while (++i < bound)
	{
		s2inode_t *node = map->table + i;
		if (node->str != 0L)
		{
			while (node != 0L)
			{
				if (node->id == id)
				{
					return node;
				}
				node = node->next;
			}
		}
	}

	return 0L;
}