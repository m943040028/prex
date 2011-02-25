/*-
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
 * ppc4xx_pci.c - platform dependent configuration for ppc4xx pci<->plb bridge
 */

#define PCIC0_CONFIG_SPACE_BASE		(0xeec00000)
#define PCIC0_INTERNAL_BASE		(0xef480000)

enum {
	PCIC0_CFGADDR	= PCIC0_CONFIG_SPACE_BASE + 0x0,
	PCIC0_CFGDATA	= PCIC0_CONFIG_SPACE_BASE + 0x4,
} pcic0_config_space_regs;

enum {
	/* PLB Memory Map (PMM) registers 
	   PLB addresses ->  PCI address */
	PCIL0_PMM0LA	= PCIC0_INTERNAL_BASE + 0x0,
	PCIL0_PMM0MA	= PCIC0_INTERNAL_BASE + 0x4,
	PCIL0_PMM0PCILA	= PCIC0_INTERNAL_BASE + 0x8,
	PCIL0_PMM0PCIHA	= PCIC0_INTERNAL_BASE + 0xc,
	PCIL0_PMM1LA	= PCIC0_INTERNAL_BASE + 0x10,
	PCIL0_PMM1MA	= PCIC0_INTERNAL_BASE + 0x14,
	PCIL0_PMM1PCILA	= PCIC0_INTERNAL_BASE + 0x18,
	PCIL0_PMM1PCIHA	= PCIC0_INTERNAL_BASE + 0x1c,
	PCIL0_PMM2LA	= PCIC0_INTERNAL_BASE + 0x20,
	PCIL0_PMM2MA	= PCIC0_INTERNAL_BASE + 0x24,
	PCIL0_PMM2PCILA	= PCIC0_INTERNAL_BASE + 0x28,
	PCIL0_PMM2PCIHA	= PCIC0_INTERNAL_BASE + 0x2c,

	/* PCI Target Map (PTM) registers
	   PCI addresses -> PLB address */
	PCIL0_PTM1MS	= PCIC0_INTERNAL_BASE + 0x30,
	PCIL0_PTM1LA	= PCIC0_INTERNAL_BASE + 0x34,
	PCIL0_PTM2MS	= PCIC0_INTERNAL_BASE + 0x38,
	PCIL0_PTM2LA	= PCIC0_INTERNAL_BASE + 0x3c,
	PCI_REG_SIZE	= PCIC0_INTERNAL_BASE + 0x40,
} pcic0_internal_regs;


int
platform_pci_init(void)
{
	/* TODO: complete this function */
	return 0;
}
