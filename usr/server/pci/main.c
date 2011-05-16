/*-
 * Copyright (c) MIT 6.828 JOS
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
 * pci.c - pci server
 */

#define DBG
#define MODULE_NAME  "pci"

#include <sys/param.h>
#include <sys/cdefs.h>
#include <sys/list.h>
#include <sys/endian.h>
#include <sys/dbg.h>
#include <assert.h>
#include <errno.h>
#include <ipc/pci.h>
#include <ipc/exec.h>
#include <ipc/ipc.h>
#include <server.h>
#include <uio.h>
#include <pci.h>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "pcireg.h"
#include "plat.h"

/* #define SHOW_PCI_VERBOSE_INFO	1 */

#ifdef DBG
static int debugflags = DBGBIT(INFO) | DBGBIT(TRACE);
#endif
#ifdef DBG
#define ASSERT(e)	dassert(e)
#else
#define ASSERT(e)
#endif

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

enum pci_config_space {
	PCI_CFG_ADDR	= 0x0,
	PCI_CFG_DATA	= 0x4,
};

struct pci_func {
	struct pci_bus *bus;	/* Primary bus for bridges */
	task_t owner;

	uint32_t dev;
	uint32_t func;

	uint32_t dev_id;
	uint32_t dev_class;

	uint32_t reg_base[6];
	uint32_t reg_size[6];
	uint8_t irq_line;
	uint8_t irq_pin;
	struct uio_mem *mem_region;
	int	num_mem_region;

	struct list link;
};

struct pci_bus {
	struct pci_func *parent_bridge;
	uint32_t busno;
};

struct pci_serv_priv {
	struct list pci_func_list;
	paddr_t pci_map_base;
	struct uio_handle *uio_handle;
	struct uio_mem *config_space;
};
static struct pci_serv_priv *serv;

/* forward declartion */
uint32_t pci_conf_read(struct pci_func *f, uint32_t off);
void pci_conf_write(struct pci_func *f, uint32_t off, uint32_t v);

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
	uio_write32(serv->config_space, PCI_CFG_ADDR, host_to_pci(v));
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

	dprintf("pci: %02x:%02x.%d: %04x:%04x: class: %x.%x (%s) irq: %d\n",
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
	aligned_addr = (serv->pci_map_base+size-1) & ~(size-1);

	serv->pci_map_base = aligned_addr + size;
	if (serv->pci_map_base > 
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
			af = malloc(sizeof(*af));
			*af = f;
			list_init(&af->link);
			list_insert(&serv->pci_func_list, &af->link);
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
	return pci_to_host(uio_read32(serv->config_space, PCI_CFG_DATA));
}

void
pci_conf_write(struct pci_func *f, uint32_t off, uint32_t v)
{
	pci_conf1_set_addr(f->bus->busno, f->dev, f->func, off);
	uio_write32(serv->config_space, PCI_CFG_DATA, host_to_pci(v));
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
			dprintf("pci: allocated mem region %d: %d bytes at 0x%x\n",
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
	/* FIXME */
	f->irq_pin = PCI_INTERRUPT_PIN_C;
	pci_conf_write(f, PCI_INTERRUPT_REG,
		       PCI_INTERRUPT_LINE(f->irq_line) |
		       PCI_INTERRUPT_PIN(f->irq_pin));

	dprintf("pci: function %02x:%02x.%d (%04x:%04x) configured\n",
		f->bus->busno, f->dev, f->func,
		PCI_VENDOR(f->dev_id), PCI_PRODUCT(f->dev_id));
	return 0;
}

void
pci_func_enable(struct pci_func *f, uint8_t flags)
{

	uint32_t v = 0;
	if (flags & PCI_MEM_ENABLE)
		v |= PCI_COMMAND_MEM_ENABLE;
	if (flags & PCI_IO_ENABLE)
		v |= PCI_COMMAND_IO_ENABLE;

	pci_conf_write(f, PCI_COMMAND_STATUS_REG,
			  v | PCI_COMMAND_MASTER_ENABLE);

	dprintf("pci: function %02x:%02x.%d (%04x:%04x) enabled\n",
		f->bus->busno, f->dev, f->func,
		PCI_VENDOR(f->dev_id), PCI_PRODUCT(f->dev_id));
}

list_t
pci_probe_device(pci_match_func match_func)
{
	list_t l = NULL;
	list_t	n, head = &serv->pci_func_list;
	for (n = list_first(&serv->pci_func_list); n != head;
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

int
server_init(void)
{
	LOG_FUNCTION_NAME_ENTRY();
	serv = malloc(sizeof(struct pci_serv_priv));
	if (!serv) {
		LOG_FUNCTION_NAME_EXIT(ENOMEM);
		return ENOMEM;
	}

	list_init(&serv->pci_func_list);
	serv->pci_map_base = CONFIG_PCI_MMIO_ALLOC_BASE;

	serv->uio_handle = uio_init();
	if (!serv->uio_handle) {
		LOG_FUNCTION_NAME_EXIT(EIO);
		return EIO;
	}

	/* map pci configuration space */
	if (uio_map_iomem(serv->uio_handle, "config_space", 
			  CONFIG_PCI_CONFIG_BASE, PAGE_SIZE) < 0)
	{
		LOG_FUNCTION_NAME_EXIT(EIO);
		return EIO;
	}
	
	if (pci_init()) {
		DPRINTF(WARN, "cannot initialize pci subsystem\n");
		LOG_FUNCTION_NAME_EXIT(EIO);
		return EIO;
	}

	LOG_FUNCTION_NAME_EXIT(0);
	return 0;
}

/*
 * Message mapping
  */
struct msg_map {
	int	code;
	int	(*func)(struct msg *);
};

static const struct msg_map pcimsg_map[] = {
	
};


/*
 * Main routine for pci server.
 */
int
main(int argc, char *argv[])
{
	static struct msg msg;
	const struct msg_map *map;
	object_t obj;
	struct bind_msg bm;
	object_t execobj, procobj;
	int error;

	sys_log("Starting pci server\n");

	/* Boost thread priority. */
	thread_setpri(thread_self(), PRI_PCI);

	/*
	 * Wait until all required system servers
	 * become available.
	 */
	wait_server("!proc", &procobj);
	wait_server("!exec", &execobj);

	/*
	 * Request to bind a new capabilities for us.
	 */
	bm.hdr.code = EXEC_BINDCAP;
	strlcpy(bm.path, "/boot/pci", sizeof(bm.path));
	msg_send(execobj, &bm, sizeof(bm));

	/*
	 * Register to process server
	 */
	register_process();

	/*
	 * Initialiate pci server;
	 */
	if (server_init())
		sys_panic("server_init failed");

	/*
	 * Create an object to expose our service.
	 */
	error = object_create("!pci", &obj);
	if (error)
		sys_panic("fail to create object");

	/*
	 * Message loop
	 */
	for (;;) {
		/*
		 * Wait for an incoming request.
		 */
		error = msg_receive(obj, &msg, sizeof(msg));
		if (error)
			continue;

		DPRINTF(VERBOSE, "msg code=%x task=%x\n",
			msg.hdr.code, (uint32_t)msg.hdr.task);


		/* Check client's capability. */
		if (task_chkcap(msg.hdr.task, CAP_USERIO) != 0) {
			map = NULL;
			error = EPERM;
		} else {
			error = EINVAL;
			map = &pcimsg_map[0];
			while (map->code != 0) {
				if (map->code == msg.hdr.code) {
					error = (*map->func)(&msg);
					break;
				}
				map++;
			}
		}
		/*
		 * Reply to the client.
		 */
		msg.hdr.status = error;
		msg_reply(obj, &msg, sizeof(msg));
#ifdef DBG
		if (map != NULL && error != 0)
			DPRINTF(VERBOSE, "msg code=%x error=%d\n",
				 map->code, error);
#endif
	}
}

