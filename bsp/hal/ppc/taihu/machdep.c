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
 * machdep_taihu.c - machine-dependent routines for PPC40x EvB
 */

#include <machine/syspage.h>
#include <sys/power.h>
#include <sys/bootinfo.h>
#include <kernel.h>
#include <page.h>
#include <mmu.h>
#include <cpu.h>
#include <io.h>
#include <cpufunc.h>
#include <locore.h>

extern void *exception_vector;
extern void *exception_vector_end;

#ifdef CONFIG_MMU
/*
 * Virtual and physical address mapping
 *
 *      { virtual, physical, size, type }
 */
struct mmumap mmumap_table[] =
{
	/*
	 * Physical memory
	 */
	{ 0x80000000, 0x00000000, 0x8000000, VMT_RAM },

	/*
	 * I/O space
	 */
	{ 0xee000000, 0xee000000, 0x2000000, VMT_IO },

#ifdef CONFIG_PCI
	/*
	 * PCI MMIO space
	 */
	{ CONFIG_PCI_MMIO_ALLOC_BASE, CONFIG_PCI_MMIO_ALLOC_BASE,
	  CONFIG_PCI_MMIO_ALLOC_SIZE, VMT_IO },
#endif

	{ 0,0,0,0 }
};
#endif

/*
 * Idle
 */
void
machine_idle(void)
{

	cpu_idle();
}

/*
 * Cause machine reset.
 */
static void
machine_reset(void)
{
	/* NOTREACHED */
}

/*
 * Set system power
 */
void
machine_powerdown(int state)
{

	DPRINTF(("Power down machine\n"));

	splhigh();

	switch (state) {
	case PWR_SUSPEND:
	case PWR_OFF:
		for (;;)
			cpu_idle();
		/* NOTREACHED */
		break;
	case PWR_REBOOT:
		machine_reset();
		/* NOTREACHED */
		break;
	}
}

/*
 * Return pointer to the boot information.
 */
void
machine_bootinfo(struct bootinfo **bip)
{

	*bip = (struct bootinfo *)BOOTINFO;
}

void
machine_abort(void)
{

	for (;;) ;
}

/*
 * Machine-dependent startup code
 */
void
machine_startup(void)
{
	void *vector_offset = (void *)SYSPAGE;

	/*
	 * Reserve system pages.
	 */
	page_reserve(kvtop(SYSPAGE), SYSPAGESZ);

	/*
	 * Copy exception vectors.
	 */
	memcpy(vector_offset, &exception_vector, 0x3000);

#ifdef CONFIG_MMU
	/*
	 * Initialize MMU
	 */
	mmu_init(mmumap_table);
#endif
}
