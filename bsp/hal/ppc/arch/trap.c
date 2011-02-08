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

/*
 * trap.c - called from the trap handler when a processor trap occurs.
 */

#include <kernel.h>
#include <task.h>
#include <hal.h>
#include <exception.h>
#include <cpu.h>
#include <trap.h>
#include <cpufunc.h>
#include <context.h>
#include <locore.h>

#if defined(CONFIG_MMU)
#include <mmu.h>
#include <vm.h>
#endif

#include <sys/signal.h>

#ifdef DEBUG
/*
 * Trap name.
 */
static const char *const trap_name[] = {
	"",				/*  0 */
	"System reset",			/*  1 */
	"Machine check",		/*  2 */
	"DSI",				/*  3 */
	"ISI",				/*  4 */
	"External interrupt",		/*  5 */
	"Alignment",			/*  6 */
	"Program",			/*  7 */
	"Floating point unavailable",	/*  8 */
	"Decrementer",			/*  9 */
	"Reserved",			/*  a */
	"Reserved",			/*  b */
	"System call",			/*  c */
	"Trace",			/*  d */
	"Floating point assist",	/*  e */
	"",				/*  f */
	"",				/* 10 */
	"Data TLB miss",		/* 11 */
};
#define MAXTRAP (sizeof(trap_name) / sizeof(void *) - 1)
#endif	/* DEBUG */

/*
 * Exception mapping table.
 * PPC exception is translated to the architecture
 * independent exception code.
 */
static const int exception_map[] = {
	SIGILL,
	SIGILL,
	SIGSEGV,	/* machine check */
	SIGSEGV,	/* address error (store) */
	SIGBUS,		/* instruction bus error */
	SIGINT,		/* external interrupt */
	SIGBUS,		/* alingment */
	SIGTRAP,	/* breakpoint trap */
	SIGFPE,		/* fpu unavail */
	SIGALRM,	/* decrementer */
	SIGILL,		/* reserved */
	SIGILL,		/* reserved */
	SIGCHLD,	/* syscall */
	SIGTRAP,	/* debug trap */
	SIGFPE,		/* fp assist */
};

#ifdef CONFIG_MMU
void handle_tlb_miss(struct cpu_regs *regs)
{
	uint32_t esr, dear;
	pgd_t pgd = mmu_get_current_pgd();

	DPRINTF(("tlb miss (%s)\n", curtask->name));

	esr = mfspr(SPR_ESR);
	dear = mfspr(SPR_DEAR);
	DPRINTF(("fault instruction address = %08x\n", regs->srr0));
	DPRINTF(("fault data access address = %08x\n", dear));
	DPRINTF(("MSR = %08x, ESR = %08x\n",
	       regs->srr1, esr));

	/* 
	 * Search on current page global directory
	 */
	if (pte_present(pgd, dear)) {

		/* get page table entry from given fault address */
		pte_t pte = vtopte(pgd, dear);

		if (page_present(pte, dear)) {

			DPRINTF(("Access Granted, Replace a TLB entry\n"));
			mmu_replace_tlb_entry(dear,
			                      (paddr_t)ptetopg(pte, dear),
					      pte);
			return;
		}
	}
	panic("Access denied\n");
}
#endif

/*
 * Trap handler
 * Invoke the exception handler if it is needed.
 */
void
trap_handler(struct cpu_regs *regs)
{
	uint32_t trap_no = regs->trap_no;

#ifdef CONFIG_MMU
	switch (trap_no) {
	case TRAP_DATA_TLB_MISS:
		handle_tlb_miss(regs);
		return;
	};
#endif
#if 1
#ifdef DEBUG
	printf("============================\n");
	printf("Trap %x: %s\n", trap_no, trap_name[trap_no]);
	printf("============================\n");

	trap_dump(regs);

	{
		int sp = regs->gr[1];
		int i = sp - 0x100;
		printf("dump stack:\n");
		for (; i < sp + 0x200 ; i += 4*4) {
			printf("0x%08x: %08x %08x %08x %08x\n",
			      i, *(uint32_t *)(i), *(uint32_t *)(i+4),
			      *(uint32_t *)(i+8), *(uint32_t *)(i+12));
		}
	}
	for (;;) ;
#endif
#endif
	if ((regs->srr1 & MSR_PR) != MSR_PR)
		panic("Kernel exception");

	exception_mark(exception_map[trap_no]);
	exception_deliver();
}

#ifdef CONFIG_DEBUG_CONTEXT_SWITCH
void
trap_unit_test(struct cpu_regs *r)
{
	static int count = 5;

	printf("trap_no: %d, count: %d\n", r->trap_no, count);
	trap_dump(r);
/*
	while (count > 0) {
		count--;
		__asm__("sc");
	}

	printf("\n\nreturn from trap:, count: %d\n", count);
	trap_dump(r);
*/
}
#endif

#ifdef DEBUG
void delay(int loop) {
	int i = 0;
	for (; i < loop; i ++);
}

void
trap_dump(struct cpu_regs *r)
{

	printf("Trap frame %x\n", r);
	printf(" r0  %08x r1  %08x r2  %08x r3  %08x r4  %08x r5  %08x\n",
	       r->gr[0], r->gr[1], r->gr[2], r->gr[3], r->gr[4], r->gr[5]);
	printf(" r6  %08x r7  %08x r8  %08x r9  %08x r10 %08x r11 %08x\n",
	       r->gr[6], r->gr[7], r->gr[8], r->gr[9], r->gr[10], r->gr[11]);
	printf(" r12 %08x r13 %08x r14 %08x r15 %08x r16 %08x r17 %08x\n",
	       r->gr[12], r->gr[13], r->gr[14], r->gr[15], r->gr[16], r->gr[17]);
	printf(" r18 %08x r19 %08x r20 %08x r21 %08x r22 %08x r23 %08x\n",
	       r->gr[18], r->gr[19], r->gr[20], r->gr[21], r->gr[22], r->gr[23]);
	printf(" r24 %08x r25 %08x r26 %08x r27 %08x r28 %08x r29 %08x\n",
	       r->gr[24], r->gr[25], r->gr[26], r->gr[27], r->gr[28], r->gr[29]);
	printf(" r30 %08x r31 %08x lr  %08x cr  %08x xer %08x ctr %08x\n",
	       r->gr[30], r->gr[31], r->lr, r->cr, r->xer, r->ctr);
	printf(" srr0 %08x srr1 %08x\n", r->srr0, r->srr1);

	printf(" >> interrupt is %s\n",
	       (r->srr1 & MSR_EE) ? "enabled" : "disabled");

	printf(" >> task=%s\n", curtask->name);
}
#endif /* !DEBUG */
