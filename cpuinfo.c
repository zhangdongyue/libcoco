#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include"common.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t cacheline_size;

#if ((__i386__ || __amd64__) && (__GNUC__ || __INTEL_COMPILER ))

static inline void cpuid(uint32_t i, uint32_t *buf);

#if ( __i386__ )

static inline void cpuid(uint32_t i,uint32_t *buf)
{
	__asm__ (
		" mov %%ebx, %%esi; "

		" mov %%eax, (%1);  "
		" cpuid;            "

		" mov %%ebx, 4(%1); "
		" mov %%edx, 8(%1); "
		" mov %%ecx, 12(%1);"

		" mov %%esi, %%ebx; "
		: : "a" (i), "D" (buf) : "ecx", "edx", "esi", "memory" );
}

#else /* __amd64__ */

static inline void cpuid(uint32_t i, uint32_t *buf)
{
	uint32_t eax, ebx, ecx, edx;

	__asm__ (
		"cpuid"
		:"=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx) : "a" (i));

	buf[0] = eax; 
	buf[1] = ebx;
	buf[2] = ecx;
	buf[3] = edx;
}

#endif

void cpuinfo(void) 
{
	unsigned char *vendor;
	uint32_t vbuf[5], cpu[4], model;

	vbuf[0] = 0;
	vbuf[1] = 0;
	vbuf[2] = 0;
	vbuf[3] = 0;
	vbuf[4] = 0;

	cpuid(0, vbuf);
	vendor = (unsigned char*) &vbuf[1];

	if (vbuf[0] == 0) {
		return ;
	}

	cpuid(1, cpu);

	if (strcmp(vendor, "GenuineIntel") == 0) {
		switch ((cpu[0] & 0xff00) >> 8) {
			/* Pentium */
			case 0x05:
				cacheline_size = 32;
				break;

			/* Pentium Pro, II, III */
			case 0x06:
				cacheline_size = 32;

				model = ((cpu[0] & 0xf0000) >> 8) | (cpu[0] & 0xf0);

				if (model >= 0xd0) {
					/* Intel Core, Core 2, Atom */
					cacheline_size = 64;
				}
				break;

			/* Pentium 4, cache line is 64 bytes, 
			 * it prefetches up to two cache lines
			 * during memory read*/
			case 0x0F:
				cacheline_size = 128;
				break;
		}
	} else if (strcmp(vendor, "AuthenticAMD") == 0) {
		cacheline_size = 64;
	}
}

#else
void cpuinfo(void) {
	printf("unused.\n");
}

#endif

#if 0
/* test */
int main()
{
	cpuinfo();
	printf("cache line size is %d.\n",cacheline_size);
	return 0;
	
}
#endif

#ifdef __cplusplus
}
#endif
