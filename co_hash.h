#ifndef __CO_HASH_H__
#define __CO_HASH_H__
/*
 _____    ___
|  _  \ /
| | | |
| |_| |  
|_____/   ___
*/
#ifdef __cplusplus
extern 'C' {
#endif

#include "co_mempool.h"

typedef struct {
	void  * value;
	unsigned short len;
	unsigned char  name[1];
} co_hash_elem_t;

typedef struct {
	co_hash_elem_t  **buckets;
	unsigned int  size;
} co_hash_t;

typedef struct {
	co_hash_t  hash;
	void  *value;
} co_hash_reg_t;

typedef struct {
	str_t  key;
	unsigned int   key_hash;
	void  *value;
} co_hash_key_t;

typedef unsigned int (*co_hash_key_handle_pt) (unsigned char *pdata, size_t len);

typedef struct {
	co_hash_t  hash;
	co_hash_reg_t *reg_head;
	co_hash_reg_t *reg_tail;
} co_hash_combined_t;

typedef struct {
	co_hash_t  *hash;
	co_hash_key_handle_pt  key;

	unsigned int  max_size;
	unsigned int  bucket_size;

	char  *name;
	co_mempool_t  *pool;
	co_mempool_t  *temp_pool;
} co_hash_init_t;

#define CO_HASH_SMALL    1
#define CO_HASH_LARGE    2
#define CO_HASH_LARGE_ASIZE  16384
#define CO_HASH_LARGE_HSIZE  10007

#define CO_HASH_REG_KEY  1
#define CO_HASH_READONLY_KEY  2

typedef struct {
	unsigned int  hsize;
	co_mempool_t  *pool;
	co_mempool_t  *temp_pool;
	
	array_t  keys;
	array_t  *keys_hash;

	array_t  dns_reg_head;
	array_t  *dns_reg_head_hash;

	array_t  dns_reg_tail;
	array_t  *dns_reg_tail_hash;
} co_hash_keys_arrays_t;

typedef struct {
	unsigned int  hash;
	str_t  key;
	str_t  value;
	unsigned char  * lowcase_key;
} co_table_elem_t;

void *co_hash_find(co_hash_t *hash, unsigned int key, unsigned char *name, size_t len);
void *co_hash_find_reg_head(co_hash_reg_t *phrt, unsigned char *pname, size_t len);
void *co_hash_find_reg_tail(co_hash_reg_t *phrg, unsigned char *pname, size_t len);
void *co_hash_find_combined(co_hash_combined_t *phash, unsigned int key, unsigned char *pname, size_t len); 

#define co_hash(key, c) ((unsigned int)key * 31 + c)
unsigned int co_hash_key(unsigned char *pdata, size_t len);
unsigned int co_hash_key_lc(unsigned char *pdata, size_t len);
unsigned int co_hash_strlow(unsigned *pdst, unsigned char *psrc, size_t n);

unsigned int co_hash_keys_array_init(co_hash_keys_arrays_t *phat, unsigned int type);
unsigned int co_hash_add_key(co_hash_keys_arrays_t *phat, str_t *pkey, void * value, unsigned int flags);

#ifdef __cplusplus
}
#endif /*cplusplus*/

#endif /* __CO_HASH_H__ */
