#ifndef _BYTE_SWAP_H_
#define _BYTE_SWAP_H_

/*
 * linux/byteorder/swab.h
 * Byte-swapping, independently from CPU endianness
 *      swabXX[ps]?(foo)
 *
 * Francois-Rene Rideau <fare@tunes.org> 19971205
 *    separated swab functions from cpu_to_XX,
 *    to clean up support for bizarre-endian architectures.
 *
 * See asm-i386/byteorder.h and suches for examples of how to provide
 * architecture-dependent optimized versions
 *
 * merged to PrexOS,
 * bsp/drv/include/bswap.h
 */
 
/* casts are necessary for constants, because we never know how for sure
 * how U/UL/ULL map to uint16_t, uint32_t, uint64_t. At least not in a portable way.
 */
#include <sys/types.h>

#define ___swab16(x) \
	({ \
	 uint16_t __x = (x); \
	 ((uint16_t)( \
		 (((uint16_t)(__x) & (uint16_t)0x00ffU) << 8) | \
		 (((uint16_t)(__x) & (uint16_t)0xff00U) >> 8) )); \
	 })

#define ___swab32(x) \
	({ \
	 uint32_t __x = (x); \
	 ((uint32_t)( \
		 (((uint32_t)(__x) & (uint32_t)0x000000ffUL) << 24) | \
		 (((uint32_t)(__x) & (uint32_t)0x0000ff00UL) <<  8) | \
		 (((uint32_t)(__x) & (uint32_t)0x00ff0000UL) >>  8) | \
		 (((uint32_t)(__x) & (uint32_t)0xff000000UL) >> 24) )); \
	 })

#define ___swab64(x) \
	({ \
	 uint64_t __x = (x); \
	 ((uint64_t)( \
		 (uint64_t)(((uint64_t)(__x) & (uint64_t)0x00000000000000ffULL) << 56) | \
		 (uint64_t)(((uint64_t)(__x) & (uint64_t)0x000000000000ff00ULL) << 40) | \
		 (uint64_t)(((uint64_t)(__x) & (uint64_t)0x0000000000ff0000ULL) << 24) | \
		 (uint64_t)(((uint64_t)(__x) & (uint64_t)0x00000000ff000000ULL) <<  8) | \
		 (uint64_t)(((uint64_t)(__x) & (uint64_t)0x000000ff00000000ULL) >>  8) | \
		 (uint64_t)(((uint64_t)(__x) & (uint64_t)0x0000ff0000000000ULL) >> 24) | \
		 (uint64_t)(((uint64_t)(__x) & (uint64_t)0x00ff000000000000ULL) >> 40) | \
		 (uint64_t)(((uint64_t)(__x) & (uint64_t)0xff00000000000000ULL) >> 56) )); \
	})

#endif /* _BYTE_SWAP_H_ */
