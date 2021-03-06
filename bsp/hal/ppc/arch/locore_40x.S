/*-
 * Copyright (c) 2009, Kohsuke Ohtani
 * Copyright (c) 2011, Sheng-Yu Chiu
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

/*
 * locore_40x.S - low level platform support for AMCC PowerPC 44x series
 */

/*
 * Memo: SPRG usage
 *  SPRG0 - kernel stack pointer
 *  SPRG1 - saved stack pointer
 *  SPRG2 - interrupt nest counter
 *  SPRG3 - scratch pad
 *  SPRG4 - trap id
 *
 *  SPRG6 - saved srr1
 *  SPRG7 - saved srr0
 */

#include <conf/config.h>
#include <machine/asm.h>
#include <machine/syspage.h>
#include <machine/memory.h>
#include <sys/errno.h>
#include <context.h>
#include <trap.h>
#include <cpu.h>

#if !defined(CONFIG_MMU)
#error "It's meaningless to disable MMU on AMCC PowerPC 40x architecture."
#endif

	.section ".text","ax"

#define ABS_JUMP(target) \
	mtsprg3	r12; \
	mfsrr1	r12; \
	mtsprg6	r12; /* sprg6: saved srr1 */ \
	mfsrr0	r12; \
	mtsprg7	r12; /* sprg7: saved srr0 */ \
	mfmsr	r12; \
	ori	r12, r12, (MSR_IR|MSR_DR); \
	mtsrr1	r12; /* return msr */	\
	lis	r12, (target)@ha; \
	ori	r12, r12, (target)@l; \
	mtsrr0	r12; /* return addr */ \
	mfsprg3	r12; \
	sync; \
	rfi

/*
 * Macro to save all registers.
 */
#define SAVE_ALL \
	subi	r1, r1, CTXREGS; \
	stw	r0, REG_R0(r1); \
	stw	r2, REG_R2(r1); \
	stw	r3, REG_R3(r1); \
	stw	r4, REG_R4(r1); \
	stw	r5, REG_R5(r1); \
	stw	r6, REG_R6(r1); \
	stw	r7, REG_R7(r1); \
	stw	r8, REG_R8(r1); \
	stw	r9, REG_R9(r1); \
	stw	r10, REG_R10(r1); \
	stw	r11, REG_R11(r1); \
	stw	r12, REG_R12(r1); \
	stw	r13, REG_R13(r1); \
	stw	r14, REG_R14(r1); \
	stw	r15, REG_R15(r1); \
	stw	r16, REG_R16(r1); \
	stw	r17, REG_R17(r1); \
	stw	r18, REG_R18(r1); \
	stw	r19, REG_R19(r1); \
	stw	r20, REG_R20(r1); \
	stw	r21, REG_R21(r1); \
	stw	r22, REG_R22(r1); \
	stw	r23, REG_R23(r1); \
	stw	r24, REG_R24(r1); \
	stw	r25, REG_R25(r1); \
	stw	r26, REG_R26(r1); \
	stw	r27, REG_R27(r1); \
	stw	r28, REG_R28(r1); \
	stw	r29, REG_R29(r1); \
	stw	r30, REG_R30(r1); \
	stw	r31, REG_R31(r1); \
	mfsprg1	r10; 			/* Get saved sp */ \
	stw	r10, REG_R1(r1); \
	mfspr	r10, SPR_SPRG7;		/* sprg7: saved srr0 */\
	stw	r10, REG_SRR0(r1); \
	mfspr	r10, SPR_SPRG6;		/* sprg6: saved srr1 */\
	stw	r10, REG_SRR1(r1); \
	mfspr	r10, SPR_LR; \
	stw	r10, REG_LR(r1); \
	mfspr	r10, SPR_CTR; \
	stw	r10, REG_CTR(r1); \
	mfspr	r10, SPR_XER; \
	stw	r10, REG_XER(r1); \
	mfcr	r10; \
	stw	r10, REG_CR(r1); \
	mfsprg4	r10; \
 	stw	r10, CTX_TRAPNO(r1)

/*
 * Macro to restore all registers.
 */
#define RESTORE_ALL \
	lwz	r10, REG_SRR0(r1); \
	mtspr	SPR_SRR0, r10; \
	lwz	r10, REG_SRR1(r1); \
	mtspr	SPR_SRR1, r10; \
	lwz	r10, REG_LR(r1); \
	mtspr	SPR_LR, r10; \
	lwz	r10, REG_XER(r1); \
	mtspr	SPR_XER, r10; \
	lwz	r10, REG_CTR(r1); \
	mtspr	SPR_CTR, r10; \
	lwz	r10, REG_CR(r1); \
	mtcr	r10; \
	lwz	r10, REG_R1(r1); \
	mtsprg1	r10; 			/* Restore saved sp */ \
	lwz	r0, REG_R0(r1); \
	lwz	r2, REG_R2(r1); \
	lwz	r3, REG_R3(r1); \
	lwz	r4, REG_R4(r1); \
	lwz	r5, REG_R5(r1); \
	lwz	r6, REG_R6(r1); \
	lwz	r7, REG_R7(r1); \
	lwz	r8, REG_R8(r1); \
	lwz	r9, REG_R9(r1); \
	lwz	r10, REG_R10(r1); \
	lwz	r11, REG_R11(r1); \
	lwz	r12, REG_R12(r1); \
	lwz	r13, REG_R13(r1); \
	lwz	r14, REG_R14(r1); \
	lwz	r15, REG_R15(r1); \
	lwz	r16, REG_R16(r1); \
	lwz	r17, REG_R17(r1); \
	lwz	r18, REG_R18(r1); \
	lwz	r19, REG_R19(r1); \
	lwz	r20, REG_R20(r1); \
	lwz	r21, REG_R21(r1); \
	lwz	r22, REG_R22(r1); \
	lwz	r23, REG_R23(r1); \
	lwz	r24, REG_R24(r1); \
	lwz	r25, REG_R25(r1); \
	lwz	r26, REG_R26(r1); \
	lwz	r27, REG_R27(r1); \
	lwz	r28, REG_R28(r1); \
	lwz	r29, REG_R29(r1); \
	lwz	r30, REG_R30(r1); \
	lwz	r31, REG_R31(r1);

/*
 * Macro to build an exception entry.
 * We assume interrupts are disabled.
 */
#define EXCEPTION_ENTRY(name, id, xfer) \
	.align 5; \
exception_##name: \
	mtsprg1 r1;		/* sprg1: current sp */ \
	mtsprg3	r10; 		/* sprg3: saved r10 */ \
	mfcr	r10; 		/* r10:	saved cr */ \
	mfsrr1	r1; 		/* r1: msr before trap */\
	mtcr	r1; \
	bt	17, 1f;		/* Exception from user mode? */ \
	mfsprg1	r1; 		/* Kernel mode => restore orignal sp */ \
	b	2f; \
1:	mfsprg0	r1; 		/* User mode => Load kernel stack */ \
2:	mtcr	r10; 		/* Restore cr */ \
	li	r10, id; 	/* sprg4: trap_id */\
	mtsprg4	r10; \
	mfsprg3 r10;		/* Restore r10 */ \
	ABS_JUMP(xfer);

#define INTR_ENTRY(n, name, id) \
		. = (n); \
		EXCEPTION_ENTRY(name, id, interrupt_common)

#define TRAP_ENTRY(n, name, id) \
		. = (n); \
		EXCEPTION_ENTRY(name, id, trap_common)

/*
 * Macro to build a system call entry.
 */
#define SYSC_ENTRY(n, name, id) \
	. = (n); \
exception_##name: \
	mtsprg1 r1;		/* sprg1: current sp */ \
	mfsprg0	r1; 		/* Load kernel stack */ \
	mtsprg3	r10; \
	li	r10, id; \
	mtsprg4 r10; \
	mfsprg3 r10; \
	ABS_JUMP(syscall_entry);

#define RESET_ENTRY(n, name) \
	. = (n); \
exception_##name: \
	ABS_JUMP(kernel_start)

/*
 * Exception vectors
 */
.globl	exception_vector
exception_vector:
RESET_ENTRY(0x100,system_reset)
TRAP_ENTRY(0x200, machine_check  ,TRAP_MACHINE_CHECK)
TRAP_ENTRY(0x300, dsi            ,TRAP_DSI)
TRAP_ENTRY(0x400, isi            ,TRAP_ISI)
INTR_ENTRY(0x500, external_intr  ,TRAP_EXT_INTERRUPT)
TRAP_ENTRY(0x600, alignment      ,TRAP_ALIGNMENT)
TRAP_ENTRY(0x700, program        ,TRAP_PROGRAM)
TRAP_ENTRY(0x800, fp_unavailable ,TRAP_FP_UNAVAILABLE)
SYSC_ENTRY(0xc00, syscall        ,TRAP_SYSTEM_CALL)
TRAP_ENTRY(0xd00, trace          ,TRAP_TRACE)
TRAP_ENTRY(0xe00, fp_assist      ,TRAP_FP_ASSIST)
INTR_ENTRY(0x1000,pit_interrupt  ,TRAP_PIT_INTERRUPT)
TRAP_ENTRY(0x1100,data_tlb_miss  ,TRAP_DATA_TLB_MISS)
TRAP_ENTRY(0x1200,inst_tlb_miss  ,TRAP_INST_TLB_MISS)

.globl	exception_vector_end
exception_vector_end:

/*
 * Kernel start point
 */
ENTRY(kernel_start)
	/*
	 * Setup CPU registers.
	 */
	li	r3, MSR_IP		/* Establish default MSR value */
	mtmsr	r3

	li	r3, 0			/* Init interrupt nest count */
	mtspr	SPR_SPRG2, r3

	li	r3, 0			/* Reset timebase */
	mttbl	r3
	mttbu	r3
	mttbl	r3

	/*
	 * Init boot stack
	 */
	lis	r1, BOOTSTKTOP@ha
	addi	r1, r1, BOOTSTKTOP@l
	subi	r1, r1, 16

	mtspr	SPR_SPRG0, r1		/* Keep kernel stack */

	/*
	 * Clear kernel BSS
	 */
	lis	r3, __bss@ha
	addi	r3, r3, __bss@l
	lis	r4, __end@ha
	addi	r4, r4, __end@l
	li	r0, 0
1:
	stwu	r0, 4(r3)
	cmplw	cr0, r3, r4
	blt	1b

	tlbia			/* Invalidate all TLB entries */
	isync

	li	r3, 0
	mtspr	SPR_PID, r3	/* PID=0 to disable protection */
	isync

	/* Lock first 16MB of memory into UTLB to enable flat mapping */
	lis	r4, KERNBASE@ha
	ori     r4, r4, (TLB_VALID|TLB_PAGESZ(PAGESZ_16M))

	ori	r3, r3, (TLB_WR|TLB_EX)	/* Set the write and execute bits */

	li	r2, 63 		/* r2 = utlb entry */
	tlbwe	r3, r2, 1	/* write tlblo(Data entry) */
	tlbwe	r4, r2, 0	/* write tlbhi(Tag entry) */
	isync

	/* Allow client access on zone 0 */
	lis	r2, 0x40000000@ha
	mtspr	SPR_ZPR, r2

	/* Enable intrustion and data translation */
	mfmsr	r3
	ori	r3, r3, (MSR_IR|MSR_DR)
	mtspr	SPR_SRR1, r3		/* return msr */
	lis	r3, main@ha
	ori	r3, r3, main@l		/* return addr */
	mtspr	SPR_SRR0, r3
	sync
	rfi
	/* never reached */


/*
 * Common entry for interrupts.
 * r10 - trap id
 */
ENTRY(interrupt_common)
	SAVE_ALL

	mfsprg2	r28			/* r28: current IRQ nesting level */
	addi	r3, r28, 1		/* Increment IRQ nesting level */
	mtsprg2	r3

	mfmsr	r29			/* r29: current msr value */
	isync
	mr	r30, r1			/* r30: trap frame */
	subi	r1, r1, STKFRAME_LEN	/* Adjust stack frame for C routine */

	cmpwi	cr0, r28, 0		/* Outermost interrupt? */
	bne	1f
	bl	sched_lock		/* If outermost, lock scheduler */
1:
	mr	r3, r30
	bl	interrupt_handler	/* Call main interrupt handler */

	mtsprg2	r28			/* Restore IRQ nesting level */
	cmpwi	cr0, r28, 0		/* Outermost interrupt? */
	bne	interrupt_ret
	bl	sched_unlock		/* Try to preempt */

	mtcr	r29			/* Exception from user mode? */
	bf	17, interrupt_ret	/* Exit if it's from kernel mode */

	mfmsr	r27			/* Enable IRQ */
	ori	r4, r27, MSR_EE@l
	mtmsr	r4
	bl	exception_deliver	/* Check exception */
	mtmsr	r27			/* Disable IRQ */
interrupt_ret:
	mr	r1, r30			/* Restore stack */
	RESTORE_ALL
	mfsprg1	r1			/* restore original sp */
	rfi

/*
 * System call entry
 */
	.global syscall_ret
ENTRY(syscall_entry)
	SAVE_ALL

	mr	r30, r1			/* r30: trap frame */
	subi	r1, r1, STKFRAME_LEN	/* Adjust stack frame for C routine */

	mfmsr	r27			/* Enable IRQ */
	ori	r27, r27, MSR_EE@l
	mtmsr	r27

	mr	r26, r0			/* r26: saved syscall number */
	mr	r7, r0
	bl	syscall_handler		/* System call dispatcher */

	cmpwi	cr0, r26, 0		/* exception_return? */
	beq	1f			/* Skip storing error if so */
	stw	r3, REG_R3(r30)		/* Set return value */
1:
	bl	exception_deliver	/* Check exception */
	mr	r1, r30			/* Restore stack */
syscall_ret:
	mfmsr	r27			/* Disable IRQ */
	andi.	r27, r27, ~MSR_EE@l
	mtmsr	r27
	RESTORE_ALL
	mfsprg1	r1			/* Restore original sp */
	rfi

/*
 * Common entry for exceptions.
 * r10 - trap id
 */
ENTRY(trap_common)
	SAVE_ALL

	mfsprg2	r3			/* increment nest counter */
	addi	r3, r3, 1
	mtsprg2	r3

	mr	r3, r1
	subi	r1, r1, STKFRAME_LEN
	bl	trap_handler
	addi	r1, r1, STKFRAME_LEN

	mfsprg2	r3			/* decrement nest counter */
	addi	r3, r3, -1
	mtsprg2	r3
trap_ret:
	RESTORE_ALL
	mfsprg1	r1			/* restore original sp */
	rfi

/*
 * Switch register context.
 * r3 = previous kern_regs, r4 = next kern_regs
 * Interrupts must be disabled by caller.
 *
 * syntax - void cpu_switch(kern_regs *prev, kern_regs *next)
 *
 * Note: GCC uses r0-r12 as scratch registers
 */
ENTRY(cpu_switch)
	isync
	stw	r13, 0x00(r3)
	stw	r14, 0x04(r3)
	stw	r15, 0x08(r3)
	stw	r16, 0x0c(r3)
	stw	r17, 0x10(r3)
	stw	r18, 0x14(r3)
	stw	r19, 0x18(r3)
	stw	r20, 0x1c(r3)
	stw	r21, 0x20(r3)
	stw	r22, 0x24(r3)
	stw	r23, 0x28(r3)
	stw	r24, 0x2c(r3)
	stw	r25, 0x30(r3)
	stw	r26, 0x34(r3)
	stw	r27, 0x38(r3)
	stw	r28, 0x3c(r3)
	stw	r29, 0x40(r3)
	stw	r30, 0x44(r3)
	stw	r31, 0x48(r3)
	stw	r2,  0x4c(r3)
	stw	r1,  0x50(r3)
	mflr	r5
	stw	r5,  0x54(r3)
	mfcr	r5
	stw	r5,  0x58(r3)
	mfsprg0	r5			/* Save kernel stack */
	stw	r5,  0x5c(r3)

	lwz	r13, 0x00(r4)
	lwz	r14, 0x04(r4)
	lwz	r15, 0x08(r4)
	lwz	r16, 0x0c(r4)
	lwz	r17, 0x10(r4)
	lwz	r18, 0x14(r4)
	lwz	r19, 0x18(r4)
	lwz	r20, 0x1c(r4)
	lwz	r21, 0x20(r4)
	lwz	r22, 0x24(r4)
	lwz	r23, 0x28(r4)
	lwz	r24, 0x2c(r4)
	lwz	r25, 0x30(r4)
	lwz	r26, 0x34(r4)
	lwz	r27, 0x38(r4)
	lwz	r28, 0x3c(r4)
	lwz	r29, 0x40(r4)
	lwz	r30, 0x44(r4)
	lwz	r31, 0x48(r4)
	lwz	r2,  0x4c(r4)
	lwz	r1,  0x50(r4)
	lwz	r5,  0x54(r4)
	mtlr	r5
	lwz	r5,  0x58(r4)
	mtcr	r5
	lwz	r5,  0x5c(r4)
	mtsprg0	r5			/* Restore kernel stack */
	isync
	blr

/*
 * void sploff(void);
 */
ENTRY(sploff)
	mfmsr	r3
	andi.	r4, r3, ~MSR_EE@l
	mtmsr	r4
	blr

/*
 * void splon(void);
 */
ENTRY(splon)
	mfmsr	r3
	ori	r4, r3, MSR_EE@l
	mtmsr	r4
	blr
