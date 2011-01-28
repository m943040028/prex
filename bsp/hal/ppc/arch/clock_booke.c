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
 * clock.c - clock driver for Power PC decrementer.
 */

#include <sys/ipl.h>
#include <kernel.h>
#include <timer.h>
#include <irq.h>
#include <cpu.h>
#include <cpufunc.h>
#include <mmu.h>

#define DECR_COUNT	100

/*
 * Timer handler.
 */
int
clock_isr(void *arg)
{
	int s;

	/* Ack */
	mtspr(SPR_TSR, TSR_DIS);

	/* Reset decrementer */
	mtspr(SPR_DECR, DECR_COUNT);

	/* Update victim tlb index */
	mmu_tlb_index_update();

	s = splhigh();
	timer_handler();
	splx(s);

	return INT_DONE;
}

/*
 * Initialize clock H/W.
 */
void
clock_init(void)
{

	DPRINTF(("clock_init\n"));
	/* Clear any pending timer interrupts */
	mtspr(SPR_TSR, TSR_ENW | TSR_WIS | TSR_DIS | TSR_FIS);

	/* Enable decrementer interrupt */
	mtspr(SPR_TCR, TCR_DIE);

	/* Set decrementer */
	mtspr(SPR_DECR, DECR_COUNT);
}
