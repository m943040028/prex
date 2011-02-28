/*-
 * Copyright (c) MIT 6.828 JOS
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

#ifndef _DEV_PCI_PCI_H_
#define	_DEV_PCI_PCI_H_

#include <sys/endian.h>

#if BYTE_ORDER == BIG_ENDIAN
#define swapb_32(x) \
({ \
	uint32_t __x = (x); \
 	((uint32_t)( \
	(((uint32_t)(__x) & (uint32_t)0x000000ffUL) << 24) | \
	(((uint32_t)(__x) & (uint32_t)0x0000ff00UL) <<  8) | \
	(((uint32_t)(__x) & (uint32_t)0x00ff0000UL) >>  8) | \
	(((uint32_t)(__x) & (uint32_t)0xff000000UL) >> 24) )); \
})
#define	host_to_pci(v)	swapb_32(v)
#define pci_to_host(v)	swapb_32(v)
#else
#define host_to_pci(v)	(v)
#define pci_to_host(v)	(v)
#endif

enum pci_config_space {
	PCI_CFG_ADDR	= CONFIG_PCI_CONFIG_BASE + 0x0,
	PCI_CFG_DATA	= CONFIG_PCI_CONFIG_BASE + 0x4,
};

struct pci_func {
	struct pci_bus *bus;	/* Primary bus for bridges */

	uint32_t dev;
	uint32_t func;

	uint32_t dev_id;
	uint32_t dev_class;

	uint32_t reg_base[6];
	uint32_t reg_size[6];
	uint8_t irq_line;
};

struct pci_bus {
	struct pci_func *parent_bridge;
	uint32_t busno;
};

struct pci_resource_map {
	char            name[20];
	pci_resource_t  type;
	paddr_t         start;
	size_t          size;
	struct list     link;
};


#endif /* _DEV_PCI_PCI_H_ */

