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

/*
 * pci.c - pci subsystem
 */

#include <sys/param.h>
#include <sys/cdefs.h>
#include <sys/list.h>
#include <sys/endian.h>
#include <driver.h>
#include <platform/pci.h>
#include <pci.h>
#include "pcireg.h"

/* #define SHOW_PCI_VERBOSE_INFO	1 */

#if BYTE_ORDER == BIG_ENDIAN
uint32_t host_to_pci(uint32_t x)
{

	u_char *s = (u_char *)&x;
	return (uint32_t)(s[0] | s[1] << 8 | s[2] << 16 | s[3] << 24);
}
uint32_t pci_to_host(uint32_t x)
{

	u_char *s = (u_char *)&x;
	return (uint32_t)(s[0] | s[1] << 8 | s[2] << 16 | s[3] << 24);
}

#else
#define host_to_pci(v)	(v)
#define pci_to_host(v)	(v)
#endif

/* forward declartion */
uint32_t pci_conf_read(struct pci_func *f, uint32_t off);
void pci_conf_write(struct pci_func *f, uint32_t off, uint32_t v);

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

	struct list link;
};

struct pci_bus {
	struct pci_func *parent_bridge;
	uint32_t busno;
};

static struct list pci_func_list = LIST_INIT(pci_func_list);
static paddr_t pci_map_base = CONFIG_PCI_MMIO_ALLOC_BASE;

static void
pci_conf1_set_addr(uint32_t bus,
		   uint32_t dev,
		   uint32_t func,
		   uint32_t offset)
{
	ASSERT(bus < 256);
	ASSERT(dev < 32);
	ASSERT(func < 8);
	ASSERT(offset < 256);
	ASSERT((offset & 0x3) == 0);

	uint32_t v = (1 << 31) |	/* config-space */
		(bus << 16) | (dev << 11) | (func << 8) | (offset);
	bus_write_32(PCI_CFG_ADDR, host_to_pci(v));
}

#ifdef SHOW_PCI_VERBOSE_INFO
static const char *pci_class[] = 
{
	[0x0] = "Unknown",
	[0x1] = "Storage controller",
	[0x2] = "Network controller",
	[0x3] = "Display controller",
	[0x4] = "Multimedia device",
	[0x5] = "Memory controller",
	[0x6] = "Bridge device",
};

static void 
pci_print_func(struct pci_func *f)
{
	const char *class = pci_class[0];
	if (PCI_CLASS(f->dev_class) < sizeof(pci_class) / sizeof(pci_class[0]))
		class = pci_class[PCI_CLASS(f->dev_class)];

	printf("pci: %02x:%02x.%d: %04x:%04x: class: %x.%x (%s) irq: %d\n",
		f->bus->busno, f->dev, f->func,
		PCI_VENDOR(f->dev_id), PCI_PRODUCT(f->dev_id),
		PCI_CLASS(f->dev_class), PCI_SUBCLASS(f->dev_class), class,
		f->irq_line);
}
#endif

static uint8_t
pci_allocate_irqline(void)
{
	int irq;
	for (irq = 0; irq < 0xff; irq++) {
		if (platform_pci_probe_irq(irq))
			return irq;
	}
	return irq;
}

static paddr_t
pci_allocate_memory(size_t size) 
{

	/*
	 * Calculate address we will assign the device.
	 * This can be made more clever, specifically:
	 *  1) The lowest 1MB should be reserved for 
	 *     devices with 1M memory type.
	 *     : Needs to be handled.
	 *  2) The low 32bit space should be reserved for
	 *     devices with 32bit type.
	 *     : With the usual handful of devices it is unlikely that the
	 *       low 4GB space will become full.
	 *  3) A bitmap can be used to avoid fragmentation.
	 *     : Again, unlikely to be necessary.
	 */
	paddr_t aligned_addr;

	/* For now, simply align to required size. */
	aligned_addr = (pci_map_base+size-1) & ~(size-1);

	pci_map_base = aligned_addr + size;
	if (pci_map_base > 
	    CONFIG_PCI_MMIO_ALLOC_BASE + CONFIG_PCI_MMIO_ALLOC_SIZE)
		return 0; /* address exhausted */

	return aligned_addr;
}

static int
pci_scan_bus(struct pci_bus *bus)
{
	int totaldev = 0;
	struct pci_func df;
	memset(&df, 0, sizeof(df));
	df.bus = bus;

	for (df.dev = 0; df.dev < 32; df.dev++) {
		uint32_t bhlc = pci_conf_read(&df, PCI_BHLC_REG);
		struct pci_func f;
		if (PCI_HDRTYPE_TYPE(bhlc) > 1)	/* Unsupported or no device */
			continue;

		/* found a device */
		totaldev++;
		f = df;

		for (f.func = 0; f.func < (PCI_HDRTYPE_MULTIFN(bhlc) ? 8 : 1);
				f.func++) {
			struct pci_func *af;
			uint32_t dev_id;
			uint32_t intr;

			dev_id = pci_conf_read(&f, PCI_ID_REG);
			if (PCI_VENDOR(dev_id) == 0xffff)
				continue;	

			/* found a function */
			af = kmem_alloc(sizeof(*af));
			*af = f;
			list_init(&af->link);
			list_insert(&pci_func_list, &af->link);
			af->dev_id = dev_id;

			intr = pci_conf_read(af, PCI_INTERRUPT_REG);
			af->irq_line = PCI_INTERRUPT_LINE(intr);
			af->dev_class = pci_conf_read(af, PCI_CLASS_REG);
#ifdef SHOW_PCI_VERBOSE_INFO
			pci_print_func(af);
#endif
		}
	}

	return totaldev;
}

/* Exported interface */
int
pci_init(void)
{
	static struct pci_bus root_bus;
	memset(&root_bus, 0, sizeof(root_bus));

	if (platform_pci_init() < 0)
		panic("pci: unable to initial PCI\n");

	return pci_scan_bus(&root_bus);
}

uint32_t
pci_conf_read(struct pci_func *f, uint32_t off)
{
	pci_conf1_set_addr(f->bus->busno, f->dev, f->func, off);
	return pci_to_host(bus_read_32(PCI_CFG_DATA));
}

void
pci_conf_write(struct pci_func *f, uint32_t off, uint32_t v)
{
	pci_conf1_set_addr(f->bus->busno, f->dev, f->func, off);
	bus_write_32(PCI_CFG_DATA, host_to_pci(v));
}

int
pci_func_configure(struct pci_func *f)
{
	uint32_t bar_width;
	uint32_t bar;
	for (bar = PCI_MAPREG_START; bar < PCI_MAPREG_END;
	     bar += bar_width)
	{
		uint32_t oldv = pci_conf_read(f, bar);

		bar_width = 4;
		pci_conf_write(f, bar, 0xffffffff);
		uint32_t rv = pci_conf_read(f, bar);

		if (rv == 0)
			continue;

		int regnum = PCI_MAPREG_NUM(bar);
		uint32_t base, size;
		if (PCI_MAPREG_TYPE(rv) == PCI_MAPREG_TYPE_MEM) {
			if (PCI_MAPREG_MEM_TYPE(rv) == PCI_MAPREG_MEM_TYPE_64BIT)
				bar_width = 8;

			size = PCI_MAPREG_MEM_SIZE(rv);
			base = PCI_MAPREG_MEM_ADDR(oldv);
			if (!base) {
				/* device is not properly configured,
				   allocate mmio address for it */
				base = pci_allocate_memory(size);
				if (!base)
					return ENOMEM;
				oldv = base;
			}
#ifdef SHOW_PCI_VERBOSE_INFO
			printf("pci: allocated mem region %d: %d bytes at 0x%x\n",
				regnum, size, base);
#endif
		} else {
#ifdef CONFIG_ARCH_HAS_IO_SPACE
			/* TODO handle IO region */
#endif
		}

		pci_conf_write(f, bar, oldv);
		f->reg_base[regnum] = base;
		f->reg_size[regnum] = size;
	}
	f->irq_line = pci_allocate_irqline();
	pci_conf_write(f, PCI_INTERRUPT_REG, PCI_INTERRUPT_LINE(f->irq_line));

	printf("pci: function %02x:%02x.%d (%04x:%04x) configured\n",
		f->bus->busno, f->dev, f->func,
		PCI_VENDOR(f->dev_id), PCI_PRODUCT(f->dev_id));
	return 0;
}

void
pci_func_enable(struct pci_func *f, uint8_t flags)
{

	uint32_t v;
	if (flags & PCI_MEM_ENABLE)
		v |= PCI_COMMAND_MEM_ENABLE;
	if (flags & PCI_IO_ENABLE)
		v |= PCI_COMMAND_IO_ENABLE;

	pci_conf_write(f, PCI_COMMAND_STATUS_REG,
			  v | PCI_COMMAND_MASTER_ENABLE);

	printf("pci: function %02x:%02x.%d (%04x:%04x) enabled\n",
		f->bus->busno, f->dev, f->func,
		PCI_VENDOR(f->dev_id), PCI_PRODUCT(f->dev_id));
}

list_t
pci_probe_device(pci_match_func match_func)
{
	list_t l = NULL;
	list_t	n, head = &pci_func_list;
	for (n = list_first(&pci_func_list); n != head;
			    n = list_next(n)) {
		struct pci_func *f;
		f = list_entry(n, struct pci_func, link);
		if (match_func(PCI_VENDOR(f->dev_id),
			       PCI_PRODUCT(f->dev_id),
			       f->dev_class)) {
			if (!l)
				l = &f->link;
			else
				list_insert(l, &f->link);
		}
	}
	return l;
}

struct pci_func *to_pci_func(list_t list) {
	return list_entry(list, struct pci_func, link);
}

uint32_t __inline
pci_func_get_reg_base(struct pci_func *f, int regnum)
{
	return f->reg_base[regnum];
}

uint32_t __inline
pci_func_get_reg_size(struct pci_func *f, int regnum)
{
	return f->reg_size[regnum];
}

uint8_t __inline
pci_func_get_irqline(struct pci_func *f)
{
	return f->irq_line;
}
