/*================================================
 *Copyright (c) Dongyue.Zippy All Rights Reserved.
 *================================================*/

// Author: Dongyue.z
// Mailto: zhangdy1986@gmail.com
// Date: 2016-02-26 18:26:00
// Brief: Atomic Operation

#ifndef _COCO_ATOMIC_H_INCLUDED_
#define _COCO_ATOMIC_H_INCLUDED_

#if (COCO_HAVE_LIBATOMIC)

#define AO_REQUIRE_CAS
#include <atomic_ops.h>

#define COCO_HAVE_ATOMIC_OPS  1

typedef long                        coco_atomic_int_t;
typedef AO_t                        coco_atomic_uint_t;
typedef volatile coco_atomic_uint_t  coco_atomic_t;

#if (COCO_PTR_SIZE == 8)
#define COCO_ATOMIC_T_LEN            (sizeof("-9223372036854775808") - 1)
#else
#define COCO_ATOMIC_T_LEN            (sizeof("-2147483648") - 1)
#endif

#define coco_atomic_cmp_set(lock, old, new)                                    \
    AO_compare_and_swap(lock, old, new)
#define coco_atomic_fetch_add(value, add)                                      \
    AO_fetch_and_add(value, add)
#define coco_memory_barrier()        AO_nop()
#define coco_cpu_pause()


#elif (COCO_DARWIN_ATOMIC)

/*
 * use Darwin 8 atomic(3) and barrier(3) operations
 * optimized at run-time for UP and SMP
 */

#include <libkern/OSAtomic.h>

/* "bool" conflicts with perl's CORE/handy.h */
#if 0
#undef bool
#endif


#define COCO_HAVE_ATOMIC_OPS  1

#if (COCO_PTR_SIZE == 8)

typedef int64_t                     coco_atomic_int_t;
typedef uint64_t                    coco_atomic_uint_t;
#define COCO_ATOMIC_T_LEN            (sizeof("-9223372036854775808") - 1)

#define coco_atomic_cmp_set(lock, old, new)                                    \
    OSAtomicCompareAndSwap64Barrier(old, new, (int64_t *) lock)

#define coco_atomic_fetch_add(value, add)                                      \
    (OSAtomicAdd64(add, (int64_t *) value) - add)

#else

typedef int32_t                     coco_atomic_int_t;
typedef uint32_t                    coco_atomic_uint_t;
#define COCO_ATOMIC_T_LEN            (sizeof("-2147483648") - 1)

#define coco_atomic_cmp_set(lock, old, new)                                    \
    OSAtomicCompareAndSwap32Barrier(old, new, (int32_t *) lock)

#define coco_atomic_fetch_add(value, add)                                      \
    (OSAtomicAdd32(add, (int32_t *) value) - add)

#endif

#define coco_memory_barrier()        OSMemoryBarrier()

#define coco_cpu_pause()

typedef volatile coco_atomic_uint_t  coco_atomic_t;


#elif (COCO_HAVE_GCC_ATOMIC)

/* GCC 4.1 builtin atomic operations */

#define COCO_HAVE_ATOMIC_OPS  1

typedef long                        coco_atomic_int_t;
typedef unsigned long               coco_atomic_uint_t;

#if (COCO_PTR_SIZE == 8)
#define COCO_ATOMIC_T_LEN            (sizeof("-9223372036854775808") - 1)
#else
#define COCO_ATOMIC_T_LEN            (sizeof("-2147483648") - 1)
#endif

typedef volatile coco_atomic_uint_t  coco_atomic_t;


#define coco_atomic_cmp_set(lock, old, set)                                    \
    __sync_bool_compare_and_swap(lock, old, set)

#define coco_atomic_fetch_add(value, add)                                      \
    __sync_fetch_and_add(value, add)

#define coco_memory_barrier()        __sync_synchronize()

#if ( __i386__ || __i386 || __amd64__ || __amd64 )
#define coco_cpu_pause()             __asm__ ("pause")
#else
#define coco_cpu_pause()
#endif


#elif ( __i386__ || __i386 )

typedef int32_t                     coco_atomic_int_t;
typedef uint32_t                    coco_atomic_uint_t;
typedef volatile coco_atomic_uint_t  coco_atomic_t;
#define COCO_ATOMIC_T_LEN            (sizeof("-2147483648") - 1)


#if ( __SUNPRO_C )

#define COCO_HAVE_ATOMIC_OPS  1

coco_atomic_uint_t
coco_atomic_cmp_set(coco_atomic_t *lock, coco_atomic_uint_t old,
    coco_atomic_uint_t set);

coco_atomic_int_t
coco_atomic_fetch_add(coco_atomic_t *value, coco_atomic_int_t add);

/*
 * Sun Studio 12 exits with segmentation fault on '__asm ("pause")',
 * so coco_cpu_pause is declared in src/os/unix/coco_sunpro_x86.il
 */

void
coco_cpu_pause(void);

/* the code in src/os/unix/coco_sunpro_x86.il */

#define coco_memory_barrier()        __asm (".volatile"); __asm (".nonvolatile")


#else /* ( __GNUC__ || __INTEL_COMPILER ) */

#define COCO_HAVE_ATOMIC_OPS  1

#include "coco_gcc_atomic_x86.h"

#endif


#elif ( __amd64__ || __amd64 )

typedef int64_t                     coco_atomic_int_t;
typedef uint64_t                    coco_atomic_uint_t;
typedef volatile coco_atomic_uint_t  coco_atomic_t;
#define COCO_ATOMIC_T_LEN            (sizeof("-9223372036854775808") - 1)


#if ( __SUNPRO_C )

#define COCO_HAVE_ATOMIC_OPS  1

coco_atomic_uint_t
coco_atomic_cmp_set(coco_atomic_t *lock, coco_atomic_uint_t old,
    coco_atomic_uint_t set);

coco_atomic_int_t
coco_atomic_fetch_add(coco_atomic_t *value, coco_atomic_int_t add);

/*
 * Sun Studio 12 exits with segmentation fault on '__asm ("pause")',
 * so coco_cpu_pause is declared in src/os/unix/coco_sunpro_amd64.il
 */

void
coco_cpu_pause(void);

/* the code in src/os/unix/coco_sunpro_amd64.il */

#define coco_memory_barrier()        __asm (".volatile"); __asm (".nonvolatile")


#else /* ( __GNUC__ || __INTEL_COMPILER ) */

#define COCO_HAVE_ATOMIC_OPS  1

#include "coco_gcc_atomic_amd64.h"

#endif


#elif ( __sparc__ || __sparc || __sparcv9 )

#if (COCO_PTR_SIZE == 8)

typedef int64_t                     coco_atomic_int_t;
typedef uint64_t                    coco_atomic_uint_t;
#define COCO_ATOMIC_T_LEN            (sizeof("-9223372036854775808") - 1)

#else

typedef int32_t                     coco_atomic_int_t;
typedef uint32_t                    coco_atomic_uint_t;
#define COCO_ATOMIC_T_LEN            (sizeof("-2147483648") - 1)

#endif

typedef volatile coco_atomic_uint_t  coco_atomic_t;


#if ( __SUNPRO_C )

#define COCO_HAVE_ATOMIC_OPS  1

#include "coco_sunpro_atomic_sparc64.h"


#else /* ( __GNUC__ || __INTEL_COMPILER ) */

#define COCO_HAVE_ATOMIC_OPS  1

#include "coco_gcc_atomic_sparc64.h"

#endif


#elif ( __powerpc__ || __POWERPC__ )

#define COCO_HAVE_ATOMIC_OPS  1

#if (COCO_PTR_SIZE == 8)

typedef int64_t                     coco_atomic_int_t;
typedef uint64_t                    coco_atomic_uint_t;
#define COCO_ATOMIC_T_LEN            (sizeof("-9223372036854775808") - 1)

#else

typedef int32_t                     coco_atomic_int_t;
typedef uint32_t                    coco_atomic_uint_t;
#define COCO_ATOMIC_T_LEN            (sizeof("-2147483648") - 1)

#endif

typedef volatile coco_atomic_uint_t  coco_atomic_t;


#include "coco_gcc_atomic_ppc.h"

#endif


#if !(COCO_HAVE_ATOMIC_OPS)

#define COCO_HAVE_ATOMIC_OPS  0

typedef int32_t                     coco_atomic_int_t;
typedef uint32_t                    coco_atomic_uint_t;
typedef volatile coco_atomic_uint_t  coco_atomic_t;
#define COCO_ATOMIC_T_LEN            (sizeof("-2147483648") - 1)


static coco_inline coco_atomic_uint_t
coco_atomic_cmp_set(coco_atomic_t *lock, coco_atomic_uint_t old,
     coco_atomic_uint_t set)
{
     if (*lock == old) {
         *lock = set;
         return 1;
     }

     return 0;
}


static coco_inline coco_atomic_int_t
coco_atomic_fetch_add(coco_atomic_t *value, coco_atomic_int_t add)
{
     coco_atomic_int_t  old;

     old = *value;
     *value += add;

     return old;
}

#define coco_memory_barrier()
#define coco_cpu_pause()

#endif


void coco_spinlock(coco_atomic_t *lock, coco_atomic_int_t value, coco_uint_t spin);

#define coco_trylock(lock)  (*(lock) == 0 && coco_atomic_cmp_set(lock, 0, 1))
#define coco_unlock(lock)    *(lock) = 0


#endif /* _COCO_ATOMIC_H_INCLUDED_ */
