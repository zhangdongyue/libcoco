#include "common.h"

/* string */
void strlower(unsigned char  *dst, unsigned char *src, size_t n)
{
	while (n) {
		*dst = tolower(*src);
		dst++;
		src++;
		n--;
	}
}

unsigned char * cpystrn(unsigned char *dst, unsigned char *src, size_t n)
{
    if (n == 0) {
        return dst;
    }

    while (--n) {
        *dst = *src;

        if (*dst == '\0') {
            return dst;
        }

        dst++;
        src++;
    }

    *dst = '\0';

    return dst;
}

/* array */
array_t array_create(co_mempool_t *p, unsigned int n, size_t size)
{
	array_t * a;
	a = co_palloc(p, sizeof(array_t));
	if (NULL == a) {
		return NULL;
	}

	if (CO_OK != array_init(a, p, n, size)) {
		return NULL;
	}

	return a;
}

void array_destroy(array_t *a) 
{
	co_mempool_t *p;

	p = a->pool;

	if ((unsigned char *)a->elems + a->size * a->nalloc == p->d.last) {
		p->d.last -= a->size * a->nalloc;
	}

	if ((unsigned char *)a + sizeof(array_t) == p->d.last) {
		p->d.last = (unsigned char *) a;
	} 
}

void * array_push(array_t *a)
{
	void *elem, *new;
	size_t size;
	co_mempool_t *p;

	if (a->nelems == a->nalloc) {
		size = a->size * a->nalloc;
		p = a->pool;

		if ((unsigned char *) a->elems + size == p->d.last 
			&& p->d.last + a->size <= p->d.end) {
			p->d.last += a->size;
			a->nalloc++;
		} else {
			
			new = co_palloc(p, 2 * size);
			if (NULL == new) {
				return NULL;
			}
			memcpy(new, a->elems, size);
			a->elems = new;
			a->nalloc *= 2;
		}
	}

	elem = (unsigned char *) a->elems + a->size * a->nelems;
	a->nelems++;

	return elem;
}

void * array_push_n(array_t * a, unsigned int n)
{
	void *elem, *new;
	size_t size;
	unsigned int nalloc;
	co_mempool_t *p;

	size = n * a->size;

	if (a->nelems + n > a->nalloc) {
		p = a->pool;

		if ((u_char *) a->elems + a->size * a->nalloc == p->d.last
			&& p->d.last + size <= p->d.end)
		{
			p->d.last += size;
			a->nalloc += n;	
		} else {
			nalloc = 2 * ((n >= a->nalloc) ? n : a->nalloc);
			new = co_palloc(p, nalloc * a->size);
			if (NULL == new) {
				return NULL;
			}

			memcpy (new, a->elems, a->nelems * a->size);
			a->elems = new;
			a->nalloc = nalloc;
		}
	}

	elem = (unsigned char *) a->elems + a->size * a->nelems;
	a->nelems += n;

	return elem;
}
