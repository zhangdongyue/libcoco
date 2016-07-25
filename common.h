#ifndef __COMMON_H__
#define __COMMON_H__

#include<sys/types.h>
#include<inttypes.h>
#include"co_mempool.h"

#define CO_ERROR -1
#define CO_OK 0
uint32_t cacheline_size = 64;

/* string */

#define tolower(c) (unsigned char) ((c >= 'A' && c <= 'Z') ? (c | 0x20) : c)

typedef struct {
	size_t  len;
	unsigned char  *data;
}str_t;

void strlower(unsigned char  *dst, unsigned char *src, size_t n);
unsigned char * cpystrn(unsigned char *dst, unsigned char *src, size_t n);

/* array */

typedef struct {
	void  *elems;
	unsigned int nelems;
	size_t  size;
	unsigned int  nalloc;
	co_mempool_t  *pool;
}array_t;

static inline int array_init(array_t * array, co_mempool_t * pool, unsigned int n, 
							 size_t size)
{
	array->nelems = 0;
	array->size = size;
	array->nalloc = n;
	array->pool = pool;

	array->elems = co_palloc(pool, n * size);
	if (array->elems == NULL) {
		return CO_ERROR;
	}

	return CO_OK;
}

array_t array_create(co_mempool_t *p, unsigned int n, size_t size);
void array_destroy(array_t *a);
void * array_push(array_t *a);
void * array_push_n(array_t * a, unsigned int n);

#endif


