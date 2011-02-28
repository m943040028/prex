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
#include <driver.h>
#include <pci.h>

#define SHOW_PCI_VERBOSE_INFO	1
#define DEBUG_PCI		1

#include "pcireg.h"
#include "pci.h"

static struct list pci_resource_list = LIST_INIT(pci_resource_list);
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

static uint32_t
pci_conf_read(struct pci_func *f, uint32_t off)
{
	pci_conf1_set_addr(f->bus->busno, f->dev, f->func, off);                                                                                                                                                                             
	return pci_to_host(bus_read_32(PCI_CFG_DATA));
}

static void
pci_conf_write(struct pci_func *f, uint32_t off, uint32_t v)
{
	pci_conf1_set_addr(f->bus->busno, f->dev, f->func, off);
	bus_write_32(PCI_CFG_DATA, host_to_pci(v));
}

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

	printf("PCI: %02x:%02x.%d: %04x:%04x: class: %x.%x (%s) irq: %d\n",
		f->bus->busno, f->dev, f->func,
		PCI_VENDOR(f->dev_id), PCI_PRODUCT(f->dev_id),
		PCI_CLASS(f->dev_class), PCI_SUBCLASS(f->dev_class), class,
		f->irq_line);
}

void
pci_func_enable(struct pci_func *f)
{
	pci_conf_write(f, PCI_COMMAND_STATUS_REG,
			  PCI_COMMAND_IO_ENABLE |
			  PCI_COMMAND_MEM_ENABLE |
			  PCI_COMMAND_MASTER_ENABLE);

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
#ifdef SHOW_PCI_VERBOSE_INFO
			printf("  mem region %d: %d bytes at 0x%x\n",
			       regnum, size, base);
#endif
		} else {
			size = PCI_MAPREG_IO_SIZE(rv);
			base = PCI_MAPREG_IO_ADDR(oldv);
#ifdef SHOW_PCI_VERBOSE_INFO
			printf("  io region %d: %d bytes at 0x%x\n",
			       regnum, size, base);
#endif
		}

		pci_conf_write(f, bar, oldv);
		f->reg_base[regnum] = base;
		f->reg_size[regnum] = size;

		if (size && !base)
			printf("PCI device %02x:%02x.%d (%04x:%04x) "
			       "may be misconfigured: "
			       "region %d: base 0x%x, size %d\n",
			       f->bus->busno, f->dev, f->func,
			       PCI_VENDOR(f->dev_id), PCI_PRODUCT(f->dev_id),
			       regnum, base, size);
	}

	printf("PCI function %02x:%02x.%d (%04x:%04x) enabled\n",
		f->bus->busno, f->dev, f->func,
		PCI_VENDOR(f->dev_id), PCI_PRODUCT(f->dev_id));
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
		if (PCI_HDRTYPE_TYPE(bhlc) > 1)	/* Unsupported or no device */
			continue;

		totaldev++;

		struct pci_func f = df;
		for (f.func = 0; f.func < (PCI_HDRTYPE_MULTIFN(bhlc) ? 8 : 1);
				f.func++) {
			struct pci_func af = f;

			af.dev_id = pci_conf_read(&f, PCI_ID_REG);
			if (PCI_VENDOR(af.dev_id) == 0xffff)
				continue;

			uint32_t intr = pci_conf_read(&af, PCI_INTERRUPT_REG);
			af.irq_line = PCI_INTERRUPT_LINE(intr);
			af.dev_class = pci_conf_read(&af, PCI_CLASS_REG);
#ifdef SHOW_PCI_VERBOSE_INFO
			pci_print_func(&af);
#endif
			/*pci_attach(&af);*/
			pci_func_enable(&af);
		}
	}

	return totaldev;
}

static int
pci_init(struct driver *self)
{
	static struct pci_bus root_bus;
	memset(&root_bus, 0, sizeof(root_bus));

	return pci_scan_bus(&root_bus);
}

struct driver pci_driver = {
	/* name */	"pci",
	/* devsops */	NULL,
	/* devsz */	0,
	/* flags */	0,
	/* probe */	NULL,
	/* init */	pci_init,
	/* shutdown */	NULL,
};

static paddr_t
pci_allocate_address(size_t size) {
	
	pci_map_base += size;
	return pci_map_base;
}

static void
pci_insert_resource_map(struct pci_resource_map *map) {
	list_init(&map->link);
	list_insert(&pci_resource_list, &map->link);
}

struct pci_resource_map *
pci_find_resource(char *name)
{
	list_t n, head = &pci_resource_list;
	struct pci_resource_map *target;
	for (n = list_first(head); n != head; n = list_next(n)) { 
		target = list_entry(n, struct pci_resource_map, link);

		if (strncmp(name, target->name, sizeof(target->name)) == 0)
			return target;
	}

	return 0;
}

int
pci_allocate_mmio_range(char *name, size_t size)
{
	struct pci_resource_map map;
	map.size = size;
	strncpy(map.name, name, sizeof(map.name));
	map.type = PCI_RES_MEM;
	map.start = pci_allocate_address(size);
	if (map.start == 0)
		return ENOMEM;

	pci_insert_resource_map(&map);

	return 0;
}

int
pci_remove_resource(char *name)
{
	struct pci_resource_map *target;
	target = pci_find_resource(name);
	if (!target) return ENODEV;

	list_remove(&target->link);

	return 0;
}
