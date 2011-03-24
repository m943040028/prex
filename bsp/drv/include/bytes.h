#ifndef _BYTES_H_
#define _BYTES_H_

#include <bswap.h>

#if BYTE_ORDER == BIG_ENDIAN
#define	cpu_to_le16(x)	___swab16(x)
#define	cpu_to_le32(x)	___swab32(x)
#define	cpu_to_le64(x)	___swab64(x)
#define	le16_to_cpu(x)	___swab16(x)
#define	le32_to_cpu(x)	___swab32(x)
#define	le64_to_cpu(x)	___swab64(x)
#else
#define	cpu_to_le16(x)	(x)
#define	cpu_to_le32(x)	(x)
#define	cpu_to_le64(x)	(x)
#endif

#if BYTE_ORDER == BIG_ENDIAN
#define	cpu_to_be16(x)	(x)
#define	cpu_to_be32(x)	(x)
#define	cpu_to_be64(x)	(x)
#else
#define	cpu_to_be16(x)	___swab16(x)
#define	cpu_to_be32(x)	___swab32(x)
#define	cpu_to_be64(x)	___swab64(x)
#define	be16_to_cpu(x)	___swab16(x)
#define	be32_to_cpu(x)	___swab32(x)
#define	be64_to_cpu(x)	___swab64(x)
#endif

#endif /* _BYTES_H_ */
