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
#ifdef CONFIG_MMU
#define MSR_DFLT	(uint32_t)(MSR_EE | MSR_PR | MSR_ME | MSR_IR | MSR_DR)
#else
#define MSR_DFLT	(uint32_t)(MSR_EE | MSR_PR | MSR_ME)
#endif

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
#define SPR_DECR		0x016	/* Decrementer Regiter */
#define SPR_PID			0x030
#define SPR_DECAR		0x036	/* Decrementer Auto-Reload Regiter */
#define SPR_DEAR		0x03D	/* Data Error Address Register */
#define SPR_ESR			0x03E	/* Exception Syndrome Register */
#define	SPR_IVPR		0x03F	/* Interrupt Vector Prefix Register */
#define SPR_TSR			0x150	/* Timer Status Register */
#define SPR_TCR			0x154	/* Timer Control Register */
#define SPR_IVOR0		0x190	/* Interrupt Vector Offset Register 0 */
#define SPR_IVOR1		0x191	/* Interrupt Vector Offset Register 1 */
#define SPR_IVOR2		0x192	/* Interrupt Vector Offset Register 2 */
#define SPR_IVOR3		0x193	/* Interrupt Vector Offset Register 3 */
#define SPR_IVOR4		0x194	/* Interrupt Vector Offset Register 4 */
#define SPR_IVOR5		0x195	/* Interrupt Vector Offset Register 5 */
#define SPR_IVOR6		0x196	/* Interrupt Vector Offset Register 6 */
#define SPR_IVOR7		0x197	/* Interrupt Vector Offset Register 7 */
#define SPR_IVOR8		0x198	/* Interrupt Vector Offset Register 8 */
#define SPR_IVOR9		0x199	/* Interrupt Vector Offset Register 9 */
#define SPR_IVOR10		0x19a	/* Interrupt Vector Offset Register 10 */
#define SPR_IVOR11		0x19b	/* Interrupt Vector Offset Register 11 */
#define SPR_IVOR12		0x19c	/* Interrupt Vector Offset Register 12 */
#define SPR_IVOR13		0x19d	/* Interrupt Vector Offset Register 13 */
#define SPR_IVOR14		0x19e	/* Interrupt Vector Offset Register 14 */
#define SPR_IVOR15		0x19f	/* Interrupt Vector Offset Register 15 */
#define	SPR_EVPR		0x3D6	/* Exception Vector Prefix Register */
#define SPR_MMUCR		0x3B2	/* MMU Control Register */
#endif /* CONFIG_PPC_BOOKE */

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

/* Bit definitions related to the ESR. */
#define ESR_MCI		0x80000000	/* Machine Check - Instruction */
#define ESR_IMCP	0x80000000	/* Instr. Machine Check - Protection */
#define ESR_IMCN	0x40000000	/* Instr. Machine Check - Non-config */
#define ESR_IMCB	0x20000000	/* Instr. Machine Check - Bus error */
#define ESR_IMCT	0x10000000	/* Instr. Machine Check - Timeout */
#define ESR_PIL		0x08000000	/* Program Exception - Illegal */
#define ESR_PPR		0x04000000	/* Program Exception - Privileged */
#define ESR_PTR		0x02000000	/* Program Exception - Trap */
#define ESR_FP		0x01000000	/* Floating Point Operation */
#define ESR_DST		0x00800000	/* Storage Exception - Data miss */
#define ESR_DIZ		0x00400000	/* Storage Exception - Zone fault */
#define ESR_ST		0x00800000	/* Store Operation */
#define ESR_DLK		0x00200000	/* Data Cache Locking */
#define ESR_ILK		0x00100000	/* Instr. Cache Locking */
#define ESR_PUO		0x00040000	/* Unimplemented Operation exception */
#define ESR_BO		0x00020000	/* Byte Ordering */

/* Bit definitions related to the TCR. */
#define TCR_WP(x)	(((x)&0x3)<<30)	/* WDT Period */
#define TCR_WP_MASK	TCR_WP(3)
#define WP_2_17		0		/* 2^17 clocks */
#define WP_2_21		1		/* 2^21 clocks */
#define WP_2_25		2		/* 2^25 clocks */
#define WP_2_29		3		/* 2^29 clocks */
#define TCR_WRC(x)	(((x)&0x3)<<28)	/* WDT Reset Control */
#define TCR_WRC_MASK	TCR_WRC(3)
#define WRC_NONE	0		/* No reset will occur */
#define WRC_CORE	1		/* Core reset will occur */
#define WRC_CHIP	2		/* Chip reset will occur */
#define WRC_SYSTEM	3		/* System reset will occur */
#define TCR_WIE		0x08000000	/* WDT Interrupt Enable */
#define TCR_PIE		0x04000000	/* PIT Interrupt Enable */
#define TCR_DIE		TCR_PIE		/* DEC Interrupt Enable */
#define TCR_FP(x)	(((x)&0x3)<<24)	/* FIT Period */
#define TCR_FP_MASK	TCR_FP(3)
#define FP_2_9		0		/* 2^9 clocks */
#define FP_2_13		1		/* 2^13 clocks */
#define FP_2_17		2		/* 2^17 clocks */
#define FP_2_21		3		/* 2^21 clocks */
#define TCR_FIE		0x00800000	/* FIT Interrupt Enable */
#define TCR_ARE		0x00400000	/* Auto Reload Enable */

/* Bit definitions for the TSR. */
#define TSR_ENW		0x80000000	/* Enable Next Watchdog */
#define TSR_WIS		0x40000000	/* WDT Interrupt Status */
#define TSR_WRS(x)	(((x)&0x3)<<28)	/* WDT Reset Status */
#define WRS_NONE	0		/* No WDT reset occurred */
#define WRS_CORE	1		/* WDT forced core reset */
#define WRS_CHIP	2		/* WDT forced chip reset */
#define WRS_SYSTEM	3		/* WDT forced system reset */
#define TSR_PIS		0x08000000	/* PIT Interrupt Status */
#define TSR_DIS		TSR_PIS		/* DEC Interrupt Status */
#define TSR_FIS		0x04000000	/* FIT Interrupt Status */

#endif /* CONFIG_PPC_BOOKE */

#ifndef __ASSEMBLY__

__BEGIN_DECLS
void	 outb(int, u_char);
u_char	 inb(int);
#if defined(CONFIG_MMU)
void	 write_tlb_entry(uint8_t, uint32_t, uint32_t);
#endif
__END_DECLS

#endif /* !__ASSEMBLY__ */
#endif /* !_PPC_CPU_H */
