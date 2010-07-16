/*-
 * Copyright (c) 2009, Kohsuke Ohtani
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _PPC_CPU_H
#define _PPC_CPU_H

#include <sys/cdefs.h>

#define STKFRAME_LEN	8		/* length for stack frame */

/*
 * Flags in MSR
 */
#define	MSR_LE		0x00000001	/* little-endian mode enable */
#define	MSR_RI		0x00000002	/* recoverable exception */
#define	MSR_DR		0x00000010	/* data address translation */
#define	MSR_IR		0x00000020	/* instruction address translation */
#define	MSR_IP		0x00000040	/* exception prefix */
#define	MSR_FE1		0x00000100	/* floating-point exception mode 1 */
#define	MSR_BE		0x00000200	/* branch trace enable */
#define	MSR_SE		0x00000400	/* single-step trace enable */
#define	MSR_FE0		0x00000800	/* floating-point exception mode 0 */
#define	MSR_ME		0x00001000	/* machine check enable */
#define	MSR_FP		0x00002000	/* floating-point available */
#define	MSR_PR		0x00004000	/* privilege level (1:USR) */
#define	MSR_EE		0x00008000	/* external interrupt enable */
#define	MSR_ILE		0x00010000	/* exception little-endian mode (1:LE) */
#define	MSR_POW		0x00040000	/* power management enable */


/* default msr for starting user program */
#if 0
#ifdef CONFIG_MMU
#define MSR_DFLT	(uint32_t)(MSR_EE | MSR_PR | MSR_ME | MSR_IR | MSR_DR)
#else
#define MSR_DFLT	(uint32_t)(MSR_EE | MSR_PR | MSR_ME)
#endif
#endif
#define MSR_DFLT	(uint32_t)(MSR_EE | MSR_PR)

/*
 * Special Purpose Register declarations.
 */
#define	SPR_XER			  1	/* fixed point exception register */
#define	SPR_LR			  8	/* link register */
#define	SPR_CTR			  9	/* count register */
#define	SPR_DSISR		 18	/* DSI exception register */
#define	SPR_DAR			 19	/* data access register */
#define	SPR_DEC			 22	/* decrementer register */
#define	SPR_SRR0		 26	/* save/restore register 0 */
#define	SPR_SRR1		 27	/* save/restore register 1 */
#define	SPR_SPRG0		272	/* SPR general 0 */
#define	SPR_SPRG1		273	/* SPR general 1 */
#define	SPR_SPRG2		274	/* SPR general 2 */
#define	SPR_SPRG3		275	/* SPR general 3 */
#define	SPR_PVR			287	/* processor version register */

#if defined(CONFIG_PPC_BOOKE)
#define SPR_ZPR			944
#define SPR_PID			945
#endif

#if defined(CONFIG_PPC_BOOKE)
/* Tag portion */
#define TLB_EPN_MASK	0xFFFFFC00	/* Effective Page Number */
#define TLB_PAGESZ_MASK	0x00000380
#define TLB_PAGESZ(x)	(((x) & 0x7) << 7)
#define   PAGESZ_1K	0
#define   PAGESZ_4K	1
#define   PAGESZ_16K	2
#define   PAGESZ_64K	3
#define   PAGESZ_256K	4
#define   PAGESZ_1M	5
#define   PAGESZ_4M	6
#define   PAGESZ_16M	7
#define TLB_VALID	0x00000040	/* Entry is valid */

/* Data portion */
#define TLB_RPN_MASK	0xFFFFFC00	/* Real Page Number */
#define TLB_PERM_MASK	0x00000300
#define TLB_EX		0x00000200	/* Instruction execution allowed */
#define TLB_WR		0x00000100	/* Writes permitted */
#define TLB_ZSEL_MASK	0x000000F0
#define TLB_ZSEL(x)	(((x) & 0xF) << 4)
#define TLB_ATTR_MASK	0x0000000F
#define TLB_W		0x00000008	/* Caching is write-through */
#define TLB_I		0x00000004	/* Caching is inhibited */
#define TLB_M		0x00000002	/* Memory is coherent */
#define TLB_G		0x00000001	/* Memory is guarded from prefetch */
#endif

#ifndef __ASSEMBLY__

__BEGIN_DECLS
void	 outb(int, u_char);
u_char	 inb(int);
__END_DECLS

#endif /* !__ASSEMBLY__ */
#endif /* !_PPC_CPU_H */
