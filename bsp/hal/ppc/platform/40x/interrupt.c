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
 * interrupt.c - interrupt management routines for universal interrupt controller
 */

/* TODO: cascade multiple uics */

#include <kernel.h>
#include <hal.h>
#include <irq.h>
#include <io.h>
#include <cpufunc.h>
#include <context.h>
#include <trap.h>
#include <clock.h>
#include <locore.h>
#include <sys/ipl.h>

/* Number of IRQ lines */
#define NIRQS		(32*CONFIG_NUICS)

/* Common IO Routines */
#define UIC_DCR_BASE	0x0c0
#define uic_write_reg(uic, reg, data) \
	mtdcr((UIC_DCR_BASE+0x10*(uic)+(reg)), (data))
#define uic_read_reg(uic, reg) \
	mfdcr((UIC_DCR_BASE+0x10*(uic)+(reg)))

/* UIC Registers */
enum {
	DCR_UICSR  = 0x000,
	DCR_UICSRS = 0x001,
	DCR_UICER  = 0x002,
	DCR_UICCR  = 0x003,
	DCR_UICPR  = 0x004,
	DCR_UICTR  = 0x005,
	DCR_UICMSR = 0x006,
	DCR_UICVR  = 0x007,
	DCR_UICVCR = 0x008,
} uic_dcr_registers;

/*
 * Interrupt priority level
 *
 * Each interrupt has its logical priority level, with 0 being
 * the highest priority. While some ISR is running, all lower
 * priority interrupts are masked off.
 */
static volatile int irq_level;

/*
 * Interrupt mapping table
 */
static int	ipl_table[NIRQS];	/* Vector -> level */
static u_int	mask_table[NIPLS];	/* Level -> mask */

/*
 * Set mask for current ipl
 */
static void
update_mask(void)
{
	u_int mask = mask_table[irq_level];

	uic_write_reg(0, DCR_UICER, mask);
}

/*
 * Unmask interrupt in PIC for specified irq.
 * The interrupt mask table is also updated.
 * Assumed CPU interrupt is disabled in caller.
 */
void
interrupt_unmask(int vector, int level)
{
	u_int enable_bit = (u_int)(0x80000000 >> vector);
	int i, s;

	s = splhigh();
	ipl_table[vector] = level;
	/*
	 * Unmask target interrupt for all
	 * lower interrupt levels.
	 */
	for (i = 0; i < level; i++)
		mask_table[i] |= enable_bit;
	update_mask();
	splx(s);
}

/*
 * Mask interrupt in PIC for specified irq.
 * Interrupt must be disabled when this routine is called.
 */
void
interrupt_mask(int vector)
{
	u_int enable_bit = (u_int)(0x80000000 >> vector);
	int i, level, s;

	s = splhigh();
	level = ipl_table[vector];
	for (i = 0; i < level; i++)
		mask_table[i] &= ~enable_bit;
	ipl_table[vector] = IPL_NONE;
	update_mask();
	splx(s);
}

/*
 * Setup interrupt mode.
 * Select whether an interrupt trigger is edge or level.
 */
void
interrupt_setup(int vector, int mode)
{
	int  s;
	u_int bit;
	u_char val;

	s = splhigh();
	bit = (u_int)(0x80000000 >> vector);

	val = uic_read_reg(0, DCR_UICTR);
	if (mode == IMODE_EDGE)
		val |= bit;
	else
		val &= ~bit;
	uic_write_reg(0, DCR_UICTR, val);
	splx(s);
}

/*
 * Get interrupt source.
 */
static int
interrupt_lookup(void)
{
	int uicsr, irq = 0, bit_mask = 0x80000000;

	uicsr = uic_read_reg(0, DCR_UICSR);

	while (1) {
		if (uicsr & bit_mask)
			break;
		bit_mask >>= 1;
		irq++;
	}

	return irq;
}

/*
 * Common interrupt handler.
 *
 * This routine is called from the low level interrupt routine
 * written in assemble code. The interrupt flag is automatically
 * disabled by h/w in CPU when the interrupt is occurred. The
 * target interrupt will be masked in ICU while the irq handler
 * is called.
 */
void
interrupt_handler(struct cpu_regs *regs)
{
	int vector;
	int old_ipl, new_ipl;

	/* Handle decrementer interrupt */
	if (regs->trap_no == TRAP_PIT_INTERRUPT) {
		clock_isr(NULL);
		return;
	}

	/* Find pending interrupt */
	vector = interrupt_lookup();

	/* Adjust interrupt level */
	old_ipl = irq_level;
	new_ipl = ipl_table[vector];
	if (new_ipl > old_ipl)		/* Ignore spurious interrupt */
		irq_level = new_ipl;
	update_mask();

	/* Dispatch interrupt */
	splon();
	irq_handler(vector);
	sploff();

	/* Restore interrupt level */
	irq_level = old_ipl;
	update_mask();
}

/*
 * Initialize universal interrupt controllers.
 * All interrupts will be masked off in ICU.
 */
void
interrupt_init(void)
{
	int i;

	irq_level = IPL_NONE;

	for (i = 0; i < NIRQS; i++)
		ipl_table[i] = IPL_NONE;

	for (i = 0; i < NIPLS; i++)
		mask_table[i] = 0x0;

	for (i = 0; i < CONFIG_NUICS; i++)
		/* mask all interrupts */
		uic_write_reg(i, DCR_UICER, 0x0);
}
