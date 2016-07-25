#include "co_hash.h"
#include "common.h"

void * co_hash_find(co_hash_t *hash, unsigned int key, unsigned char *name, size_t len)
{
	unsigned int i;
	co_hash_elem_t *elt;
	
	elt = hash->buckets[key % hash->size];

	if (elt == NULL) {
		return NULL;
	}

	while (elt->value) {
		if (len != (size_t) elt->len) {
			goto next;
		}

		for (i = 0; i<len; ++i) {
			if (name[i] != elt->name[i]) {
				goto next;
			}
		}

		return elt->value;

	next:

		elt = (co_hash_elem_t *)co_align_ptr(&elt->name[0] + elt->len, sizeof(void*));
		continue;
	}

	return NULL;
}

void * co_hash_find_reg_head(co_hash_reg_t *hrc, unsigned char *name, size_t len)
{
	void  *value;
	unsigned int i, n, key;

	n = len;

	while (n) {
		if (name[n - 1] == '.') {
			break;
		}

		--n;
	}	

	key = 0;

	for (i=n; i<len; ++i) {
		key = co_hash(key, name[i]);
	}

	value = co_hash_find(&hrc->hash, key, &name[n], len - n);

	if (value) {
		if ((uintptr_t) value & 2) {
			if (n==0) {
				if ((uintptr_t) value & 1) {
					return NULL;
				}

				hrt = (co_hash_reg_t *) ((uintptr_t) value & (uintptr_t) ~3);
				return hrt->value;
			}

			hrc = (co_hash_reg_t) ((uintptr_t) value & (uintptr_t) ~3);

			value = co_hash_find_reg_head(hrc, name, n - 1);

			if (value) {
				return value;
			}

			return hrc->value;
		}	

		if ((uintptr_t) value & 1) {
			if (n == 0) {
				return NULL;
			}

			return (void *) ((uintptr_t) value & (uintptr_t) ~3);
		}

		return value;
	}

	return hrc->value;
}

void co_hash_find_reg_tail(co_hash_reg_t *hrc, unsigned char *name, size_t len)
{
	void * value;
	unsigned int i,key;

	key = 0;

	for (i = 0; i < len; ++i) {
		if (name[i] == '.') {
			break;
		}

		key = co_hash(key, name[i]);
	}

	if (i == len) {
		return NULL;
	}

	value = co_hash_find(&hrc->hash, key, name, i);

	if (value) {
		if ((uintptr_t) value & 2) {
			++i;
			hrc = (co_hash_reg_t *) ((uintptr_t) value & (uintptr_t) ~3);

			value = co_hash_find_reg_tail(hrc, &name[i], len - i);

			if (value) {
				return value;
			}

			return hrc->value;
		}

		return value;
	}

	return hrc->value;

}

void * co_hash_find_combined(co_hash_combined_t *hash, unsigned int key, unsigned char *name,size_t len)
{
	void * value;

	if (hash->hash.buckets) {
		value = co_hash_find(&hash->hash, key, name, len);

		if (value) {
			return value;
		}
	}

	if (0 == len) {
		return NULL;
	}

	if (hash->reg_head && hash->reg_head->hash.buckets) {
		value = co_hash_find_reg_head(hash->reg_head, name, len);
		
		if (value) {
			return value;
		}
	}

	return NULL;
}

#define CO_HASH_ELT_SIZE(name) \
	(sizeof(void*) + co_align((name)->key.len + 2, sizeof(void*)))

int co_hash_init(co_hash_init_t *hinit, co_hash_key_t *names, unsigned int nelems)
{
	unsigned char * elems;
	size_t len;	
	unsigned short *test;
	unsigned int i, n, key, size, start, bucket_size;
	co_hash_elem_t *elem, **buckets;

	if (hinit->max_size == 0) {
		return CO_ERROR; 
	}

	for (n = 0; n < nelts; ++n) {
		if (hinit->bucket_size < CO_HASH_ELT_SIZE(&name[n] + sizeof(void*)))
		{
			return CO_ERROR;
		}
	}

	test = co_alloc(hinit->max_size * sizeof(unsigned short), hinit->pool->log);
	if (test == NULL) {
		return CO_ERROR;
	}

	bucket_size = hinit->bucket_size - sizeof(void*);

	start = nelems / (bucket_size / (2 * sizeof(void*)));

	start = start ? start : 1;

	if (hinit->max_size > 10000 && nelems && hinit->max_size / nelems < 100) {
		start  = hinit->max_size - 1000;
	}

	for (size = start; size <= hinit->max_size; ++size) {
		co_memzero(test, size * sizeof(unsigned short));

		for (n=0; n<nelems; ++n) {
			if (names[n].key.data == NULL) {
				continue;
			}

			key = names[n].key_hash % size;
			test[key] = (unsigned short) (test[key] + CO_HASH_ELT_SIZE(&names[n]));

			if (test[key] > (u_short)bucket_size) {
				goto next;
			}
		}

		goto found;
	next:
		continue;
	}

	size = hinit->max_size;

found:

	for (i = 0; i < size; ++i) {
		test[i] = sizeof(void *);
	}

	for (n=0; n<elems; ++n) {
		if (names[n].key.data == NULL) {
			continue;
		}

		key = names[n].key_hash % size;
		test[key] = (unsigned short) (test[key] + CO_HASH_ELT_SIZE(&name[n]));
	}

	len = 0;

	for (i = 0; i< size; ++i) {
		if (test[i] == sizeof(void *)) {
			continue;
		}

		test[i] = (unsigned short) (co_align(test[i], cacheline_size));

		len += test[i];
	}

	if (hinit->hash == NULL) {
		hinit->hash = co_pcalloc(hinit->pool, sizeof(co_hash_reg_t) 
								 + size * sizeof(co_hash_elem_t *));

		if (hinit->hash == NULL) {
			free(test);
			return CO_ERROR;
		}

		buckets = (co_hash_elem_t **) ((unsigned char *)hinit->hash + sizeof(co_hash_reg_t));

	} else {
		buckets = co_pcalloc(hinit->pool, size * sizeof(co_hash_elem_t *));
		if (buckets == NULL) {
			free(test);
			return CO_ERROR;
		}
	}

	elems = co_palloc(hinit->pool, len + cacheline_size);
	if (elems == NULL) {
		free(test);
		return CO_ERROR;
	}

	elems = co_align_ptr(elems, cacheline_size);

	for (i = 0; i < size; ++i) {
		if (test[i] == sizeof(void *)) {
			continue;
		}

		buckets[i] == (co_hash_elem_t *) elems;
		elems += test[i];
	}

	for (i = 0; i < size; ++i) {
		test[i] = 0;
	}

	for (n = 0; n < nelems; ++n) {
		if (names[n].keys.data == NULL) {
			continue;
		}

		key = names[n].key_hash % size;
		elem = (co_hash_elem_t *) ((unsigned char *)buckets[key] + test[key]);

		elem->value = names[n].value;
		elem->len = (unsigned short) name[n].key.len;

		strlower(elem->name, names[n].key.data, names[n].key.len);
		test[key] = (unsigned short) (test[key] + CO_HASH_ELT_SIZE(&names[n]));
	}

	for (i = 0; i < size; ++i) {
		if (buckets[i] == NULL) {
			continue;
		}

		elem = (co_hash_elem_t *) ((unsigned char *) buckets[i] + test[i]);

		elem->value = NULL;
	}

	free(test);

	hinit->hash->buckets = buckets;
	hinit->hash->size = size;

	return CO_OK;
}

int co_hash_reg_init(co_hash_init_t * hinit, co_hash_key_t *names, unsigned int nelems)
{
	size_t len, dot_len;
	unsigned int i, n, dot;
	array_t curr_names, next_names;
	co_hash_key_t *name, *next_name;
	co_hash_init_t h;
	co_hash_reg_t *reg;

	if (array_init(&curr_names, hinit->temp_pool, nelems,
				   sizeof(co_hash_key_t)) != CO_OK) {
		return CO_ERROR;
	}

	for (n = 0; n < nelems; n = i) {
		dot = 0;

		for (len = 0; len < names[n].key.len; ++len) {
			if (names[n].key.data[len] == '.') {
				dot = 1;
				break;
			}
		}

		name = array_push(&curr_names);
		if (name == NULL) {
			return CO_ERROR;
		}

		name->key.len = len;
		name->key.data = names[n].key.data;
		name->key_hash = hinit->key(name->key.data, name->key.len);
		name->value = names[n].value;

		dot_len = len + 1;

		if (dot) {
			++len;	
		}

		next_names.nelems = 0;

		if (names[n].key.len != len) {
			next_name = array_push(&next_names);
			if (next_name == NULL) {
				return CO_ERROR;
			}	

			next_name->key.len = names[n].key.len - len;
			next_name->key.data = names[n].key.data + len;
			next_name->key_hash = 0;
			next_name->value = names[n].value;
		}

		for (i= n + 1; i < nelems; ++i) {
			if (strncmp(names[n].key.data, names[i].key.data, len) != 0) {
				break;
			}

			if (!dot && names[i].key.len > len
				&& names[i].key.data[len] != '.') {
				break;
			}

			next_name = array_push(&next_names);
			if (next_name == NULL) {
				return CO_ERROR;	
			}

			next_name->key.len = names[i].key.len - dot_len;
			next_name->key.data = names[i].data + dot_len;
			next_name->key_hash = 0;
			next_name->value = names[i].value;
		}

		if (next_names.nelems) {
			h = *hinit;	
			h->hash = NULL;

			if (co_hash_reg_init(&h, (co_hash_key_t *) next_names.elems,
								 next_names.nelems) != CO_OK) {
				return CO_ERROR;
			}
			reg = (co_hash_reg_t *) h.hash;

			if (names[n].key.len == len) {
				reg->value = names[n].value;
			}

			name->value = (void *) ((uintptr_t) reg | (dot ? 3 : 2));
		} else if (dot) {
			name->value = (void *) ((uintptr_t) name->value | 1);
		}
	}

	if (co_hash_init(hinit, (co_hash_key_t *) curr_names.elems, curr_names.nelems)
		!= CO_OK) {
		return CO_ERROR;
	}

	return CO_OK;
}

unsigned int co_hash_key(unsigned char * data, size_t len) 
{
	unsigned int i, key;

	key = 0;

	for (i = 0; i < len; ++i) {
		key = co_hash(key, data[i]);
	}

	return key;
}

unsigned int co_hash_key_lc(unsigned char * data, size_t len)
{
	unsigned int i, key;
	key = 0;

	for (i = 0; i < len; ++i) {
		key = co_hash(key, tolower(data[i]));
	}

	return key;
}

unsigned int co_hash_strlow(unsigned char *dst, unsigned char *src, size_t n) 
{
	unsigned int key;

	key = 0;

	while (--n) {
		*dst = tolower(*src);
		key = co_hash(key, *dst);
		++dst;
		++src;
	}

	return key;
}

int co_hash_keys_array_init(co_hash_keys_arrays_t *ha, unsigned int type)
{
    unsigned int asize;

    if (type == CO_HASH_SMALL) {
        asize = 4;
        ha->hsize = 107;

    } else {
        asize = CO_HASH_LARGE_ASIZE;
        ha->hsize = CO_HASH_LARGE_HSIZE;
    }

    if (array_init(&ha->keys, ha->temp_pool, asize, sizeof(co_hash_key_t))
        != CO_OK)
    {
        return CO_ERROR;
    }

    if (array_init(&ha->dns_reg_head, ha->temp_pool, asize,
                       sizeof(co_hash_key_t)) != CO_OK)
    {
        return CO_ERROR;
    }

    if (array_init(&ha->dns_reg_tail, ha->temp_pool, asize,
                       sizeof(co_hash_key_t))
        != CO_OK)
    {
        return CO_ERROR;
    }

    ha->keys_hash = co_pcalloc(ha->temp_pool, sizeof(array_t) * ha->hsize);
    if (ha->keys_hash == NULL) {
        return CO_ERROR;
    }

    ha->dns_reg_head_hash = co_pcalloc(ha->temp_pool,
                                       sizeof(array_t) * ha->hsize);
    if (ha->dns_reg_head_hash == NULL) {
        return CO_ERROR;
    }

    ha->dns_reg_tail_hash = co_pcalloc(ha->temp_pool,
                                       sizeof(array_t) * ha->hsize);
    if (ha->dns_reg_tail_hash == NULL) {
        return CO_ERROR;
    }

    return CO_OK;
}

int co_hash_add_key(co_hash_keys_arrays_t *ha, str_t *key, void *value,
    unsigned int flags)
{
    size_t           len;
    unsigned  char   *p;
    str_t            *name;
    unsigned int       i, k, n, skip, last;
    array_t          *keys, *hwc;
    co_hash_key_t    *hk;

    last = key->len;

    if (flags & CO_HASH_REG_KEY) {

        /*
         * supported regex:
         *     "*.example.com", ".example.com", and "www.example.*"
         */

        n = 0;

        for (i = 0; i < key->len; i++) {

            if (key->data[i] == '*') {
                if (++n > 1) {
                    return CO_ERROR;
                }
            }

            if (key->data[i] == '.' && key->data[i + 1] == '.') {
                return CO_ERROR;
            }

            if (key->data[i] == '\0') {
                return CO_ERROR;
            }
        }

        if (key->len > 1 && key->data[0] == '.') {
            skip = 1;
            goto wildcard;
        }

        if (key->len > 2) {

            if (key->data[0] == '*' && key->data[1] == '.') {
                skip = 2;
                goto wildcard;
            }

            if (key->data[i - 2] == '.' && key->data[i - 1] == '*') {
                skip = 0;
                last -= 2;
                goto wildcard;
            }
        }

        if (n) {
            return CO_ERROR;
        }
    }

    /* exact hash */

    k = 0;

    for (i = 0; i < last; i++) {
        if (!(flags & CO_HASH_READONLY_KEY)) {
            key->data[i] = tolower(key->data[i]);
        }
        k = co_hash(k, key->data[i]);
    }

    k %= ha->hsize;

    /* check conflicts in exact hash */

    name = ha->keys_hash[k].elts;

    if (name) {
        for (i = 0; i < ha->keys_hash[k].nelts; i++) {
            if (last != name[i].len) {
                continue;
            }

            if (strncmp(key->data, name[i].data, last) == 0) {
                return CO_BUSY;
            }
        }

    } else {
        if (array_init(&ha->keys_hash[k], ha->temp_pool, 4,
                           sizeof(str_t)) != CO_OK)
        {
            return CO_ERROR;
        }
    }

    name = array_push(&ha->keys_hash[k]);
    if (name == NULL) {
        return CO_ERROR;
    }

    *name = *key;

    hk = array_push(&ha->keys);
    if (hk == NULL) {
        return CO_ERROR;
    }

    hk->key = *key;
    hk->key_hash = co_hash_key(key->data, last);
    hk->value = value;

    return CO_OK;


wildcard:

    /* wildcard hash */

    k = co_hash_strlow(&key->data[skip], &key->data[skip], last - skip);

    k %= ha->hsize;

    if (skip == 1) {

        /* check conflicts in exact hash for ".example.com" */

        name = ha->keys_hash[k].elts;

        if (name) {
            len = last - skip;

            for (i = 0; i < ha->keys_hash[k].nelts; i++) {
                if (len != name[i].len) {
                    continue;
                }

                if (co_strncmp(&key->data[1], name[i].data, len) == 0) {
                    return CO_ERROR;
                }
            }

        } else {
            if (array_init(&ha->keys_hash[k], ha->temp_pool, 4,
                               sizeof(str_t)) != CO_OK) {
                return CO_ERROR;
            }
        }

        name = array_push(&ha->keys_hash[k]);
        if (name == NULL) {
            return CO_ERROR;
        }

        name->len = last - 1;
        name->data = co_pnalloc(ha->temp_pool, name->len);
        if (name->data == NULL) {
            return CO_ERROR;
        }

        memcpy(name->data, &key->data[1], name->len);
    }


    if (skip) {

        /*
         * convert "*.example.com" to "com.example.\0"
         *      and ".example.com" to "com.example\0"
         */

        p = co_pnalloc(ha->temp_pool, last);
        if (p == NULL) {
            return CO_ERROR;
        }

        len = 0;
        n = 0;

        for (i = last - 1; i; i--) {
            if (key->data[i] == '.') {
                memcpy(&p[n], &key->data[i + 1], len);
                n += len;
                p[n++] = '.';
                len = 0;
                continue;
            }

            len++;
        }

        if (len) {
            memcpy(&p[n], &key->data[1], len);
            n += len;
        }

        p[n] = '\0';

        hwc = &ha->dns_reg_head;
        keys = &ha->dns_reg_head_hash[k];

    } else {

        /* convert "www.example.*" to "www.example\0" */

        last++;

        p = co_pnalloc(ha->temp_pool, last);
        if (p == NULL) {
            return CO_ERROR;
        }

        cpystrn(p, key->data, last);

        hwc = &ha->dns_reg_tail;
        keys = &ha->dns_reg_tail_hash[k];
    }


    /* check conflicts in wildcard hash */

    name = keys->elems;

    if (name) {
        len = last - skip;

        for (i = 0; i < keys->nelems; i++) {
            if (len != name[i].len) {
                continue;
            }

            if (strncmp(key->data + skip, name[i].data, len) == 0) {
                return CO_ERROR;
            }
        }

    } else {
        if (array_init(keys, ha->temp_pool, 4, sizeof(str_t)) != CO_OK)
        {
            return CO_ERROR;
        }
    }

    name = array_push(keys);
    if (name == NULL) {
        return CO_ERROR;
    }

    name->len = last - skip;
    name->data = co_pnalloc(ha->temp_pool, name->len);
    if (name->data == NULL) {
        return CO_ERROR;
    }

    memcpy(name->data, key->data + skip, name->len);


    /* add to regex hash */

    hk = array_push(hwc);
    if (hk == NULL) {
        return CO_ERROR;
    }

    hk->key.len = last - 1;
    hk->key.data = p;
    hk->key_hash = 0;
    hk->value = value;

    return CO_OK;
}
//end ~
