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
 * main.c - pci server
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
#include <ipc/exec.h>
#include <ipc/ipc.h>
#include <ipc/pci.h>
#include <server.h>
#include <uio.h>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "pcireg.h"
#include "plat.h"

#ifdef DBG
static int debugflags = DBGBIT(INFO);
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

	struct list link;
};

struct pci_scan_node {
	struct list link;
	struct pci_func *pf;
};

struct pci_bus {
	struct pci_func *parent_bridge;
	uint32_t busno;
};

struct pci_user {
	task_t	task;
	struct list link;

	/* connect all pci_scan_node together */
	struct list pci_scan_result;
	/* iterator used to get pci function iteratively */
	list_t pci_scan_itr;
};

struct pci_serv_priv {
	struct list pci_func_list;
	struct list pci_user_list;
	paddr_t pci_map_base;
	struct uio_handle *uio_handle;
	struct uio_mem *config_space;
};
static struct pci_serv_priv *__serv;

/* forward declartion */
static uint32_t pci_conf_read(struct pci_func *f, uint32_t off);
static void pci_conf_write(struct pci_func *f, uint32_t off, uint32_t v);
static int pci_func_configure(struct pci_func *f);

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
	uio_write32(__serv->config_space, PCI_CFG_ADDR, host_to_pci(v));
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

	DPRINTF(VERBOSE, "pci: %02x:%02x.%d: %04x:%04x: class: %x.%x (%s) irq: %d\n",
		f->bus->busno, f->dev, f->func,
		PCI_VENDOR(f->dev_id), PCI_PRODUCT(f->dev_id),
		PCI_CLASS(f->dev_class), PCI_SUBCLASS(f->dev_class), class,
		f->irq_line);
}

static uint8_t
pci_allocate_irqline(void)
{
	uint8_t irq;
	int ret;
	ret = platform_pci_probe_irq(&irq);
	if (ret) /* out of irq allocation space */
		return 0;
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
	aligned_addr = (__serv->pci_map_base+size-1) & ~(size-1);

	__serv->pci_map_base = aligned_addr + size;
	if (__serv->pci_map_base > 
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
			list_insert(&__serv->pci_func_list, &af->link);
			af->dev_id = dev_id;

			intr = pci_conf_read(af, PCI_INTERRUPT_REG);
			af->irq_line = PCI_INTERRUPT_LINE(intr);
			af->dev_class = pci_conf_read(af, PCI_CLASS_REG);
			pci_print_func(af);
			pci_func_configure(af);
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

static uint32_t
pci_conf_read(struct pci_func *f, uint32_t off)
{
	pci_conf1_set_addr(f->bus->busno, f->dev, f->func, off);
	return pci_to_host(uio_read32(__serv->config_space, PCI_CFG_DATA));
}

static void
pci_conf_write(struct pci_func *f, uint32_t off, uint32_t v)
{
	pci_conf1_set_addr(f->bus->busno, f->dev, f->func, off);
	uio_write32(__serv->config_space, PCI_CFG_DATA, host_to_pci(v));
}

static int
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
			DPRINTF(VERBOSE, "pci: allocated mem region %d: %d bytes at 0x%x\n",
				regnum, size, base);
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

	return 0;
}

void
_pci_func_enable(struct pci_func *f, uint8_t flags)
{

	uint32_t v = 0;
	if (flags & PCI_MEM_ENABLE)
		v |= PCI_COMMAND_MEM_ENABLE;
	if (flags & PCI_IO_ENABLE)
		v |= PCI_COMMAND_IO_ENABLE;

	pci_conf_write(f, PCI_COMMAND_STATUS_REG,
			  v | PCI_COMMAND_MASTER_ENABLE);

	DPRINTF(VERBOSE, "pci: function %02x:%02x.%d (%04x:%04x) enabled\n",
		f->bus->busno, f->dev, f->func,
		PCI_VENDOR(f->dev_id), PCI_PRODUCT(f->dev_id));
}

int
server_init(void)
{
	LOG_FUNCTION_NAME_ENTRY();
	__serv = malloc(sizeof(struct pci_serv_priv));
	if (!__serv) {
		LOG_FUNCTION_NAME_EXIT(ENOMEM);
		return ENOMEM;
	}

	list_init(&__serv->pci_func_list);
	list_init(&__serv->pci_user_list);
	__serv->pci_map_base = CONFIG_PCI_MMIO_ALLOC_BASE;

	__serv->uio_handle = uio_init();
	if (!__serv->uio_handle) {
		LOG_FUNCTION_NAME_EXIT(EIO);
		return EIO;
	}

	/* map pci configuration space */
	__serv->config_space = uio_map_iomem(__serv->uio_handle, "config_space",
					     CONFIG_PCI_CONFIG_BASE, PAGE_SIZE);
	if (!__serv->config_space)
		return EIO;

	pci_init();

	LOG_FUNCTION_NAME_EXIT(0);
	return 0;
}

static struct pci_user *
pci_serv_find_user_by_task(task_t task)
{
	list_t n = list_first(&__serv->pci_user_list);
	LOG_FUNCTION_NAME_ENTRY();

	if (list_empty(&__serv->pci_user_list))
		return NULL;

	do {
		struct pci_user *user = list_entry(n, struct pci_user, link);
		if (user->task == task) {
			LOG_FUNCTION_NAME_EXIT_PTR(user);
			return user;
		}
		n = list_next(n);
	} while (n != &__serv->pci_user_list);
	LOG_FUNCTION_NAME_EXIT_PTR(NULL);
	return NULL;
}

static struct pci_func *
pci_serv_find_func(struct pci_user *user, pci_func_id_t *id)
{
	list_t n = list_first(&__serv->pci_user_list);
	LOG_FUNCTION_NAME_ENTRY();

	if (list_empty(&__serv->pci_user_list))
		return NULL;

	do {
		struct pci_scan_node *node = 
			list_entry(n, struct pci_scan_node, link);
		struct pci_func *pf = node->pf;
		
		if (id->busno == pf->bus->busno &&
		    id->dev_id == pf->dev_id &&
		    id->dev_class == pf->dev_class &&
		    id->func == pf->func &&
		    id->dev == pf->dev) {
			LOG_FUNCTION_NAME_EXIT_PTR(pf);
			return pf;
		}
		n = list_next(n);
	} while (n != &__serv->pci_user_list);
	LOG_FUNCTION_NAME_EXIT_PTR(NULL);
	return NULL;
}

static void
pci_serv_free_scan_result(struct pci_user *user)
{
	list_t p = list_first(&user->pci_scan_result);

	while (!list_empty(&user->pci_scan_result)) {
		struct pci_scan_node *node =
			list_entry(p, struct pci_scan_node, link);
		list_remove(&node->link);
		free(node);
		p = list_next(p);
	}
}

static void
pci_serv_probe_device(pci_probe_t *probe, struct pci_user *user)
{
	list_t n = list_first(&__serv->pci_func_list);

	LOG_FUNCTION_NAME_ENTRY();
	if (list_empty(&__serv->pci_func_list))
		return;
	if (!list_empty(&user->pci_scan_result))
		/* already has scan result? free it */
		pci_serv_free_scan_result(user);

	do {
		struct pci_func *pf = list_entry(n, struct pci_func, link);
		uint8_t matched = 0;
		if (probe->type & PCI_MATCH_VENDOR_PRODUCT)
			matched = (probe->pci_vendor == PCI_VENDOR(pf->dev_id))
				&&(probe->pci_product == PCI_PRODUCT(pf->dev_id));
		if (probe->type & PCI_MATCH_VENDOR_ONLY)
			matched = (probe->pci_vendor == PCI_VENDOR(pf->dev_id));
		if (probe->type & PCI_MATCH_PRODUCT_ONLY)
			matched = (probe->pci_product == PCI_PRODUCT(pf->dev_id));
		if (probe->type & PCI_MATCH_CLASS)
			matched = (probe->pci_class == pf->dev_class);
		if (probe->type & PCI_MATCH_PROBE)
			matched = 1;
		if (matched) {
			struct pci_scan_node *node;
			node = malloc(sizeof(*node));
			node->pf = pf;
			list_init(&node->link);
			list_insert(&user->pci_scan_result, &node->link);
			DPRINTF(VERBOSE, "probe matched\n");
		}
		n = list_next(n);
	} while (n != &__serv->pci_func_list);
	/* initial iterator */
	user->pci_scan_itr = list_first(&user->pci_scan_result);
	LOG_FUNCTION_NAME_EXIT_NORET();
}

static void
pci_serv_add_user(struct pci_user *user)
{
	list_insert(&__serv->pci_user_list, &user->link);
}

static void
pci_serv_remove_user(struct pci_user *user)
{
	list_remove(&user->link);
}

static int 
pci_connect(struct msg *msg, struct msg **reply, size_t *reply_size)
{
	task_t task = msg->hdr.task;
	struct pci_user *user;
	*reply = NULL;

	LOG_FUNCTION_NAME_ENTRY();
	if (pci_serv_find_user_by_task(task))
		return EEXIST;

	user = malloc(sizeof(struct pci_user));
	if (!user)
		return ENOMEM;
	list_init(&user->link);
	list_init(&user->pci_scan_result);
	user->task = task;
	pci_serv_add_user(user);

	LOG_FUNCTION_NAME_EXIT(0);
	return 0;
}

static int 
pci_disconnect(struct msg *msg, struct msg **reply, size_t *reply_size)
{
	task_t task = msg->hdr.task;
	struct pci_user *user;
	*reply = NULL;

	user = pci_serv_find_user_by_task(task);
	if (!user)
		return ENOENT;
	pci_serv_remove_user(user);
	free(user);
	return 0;
}

static int
pci_probe_deivce(struct msg *msg, struct msg **reply, size_t *reply_size)
{
	struct pci_probe_msg *m = (struct pci_probe_msg *)msg;
	task_t task = m->hdr.task;
	struct pci_user *user;
	*reply = NULL;

	user = pci_serv_find_user_by_task(task);
	if (!user)
		return ENOENT;
	
	pci_serv_probe_device(&m->probe, user);
	return 0;
}

static int
pci_probe_get_result(struct msg *msg, struct msg **reply, size_t *reply_size)
{
	task_t task = msg->hdr.task;
	struct pci_probe_reply *r;
	struct pci_func *pf;
	struct pci_scan_node *node;
	struct pci_user *user;
	LOG_FUNCTION_NAME_ENTRY();

	r = malloc(sizeof(*r));
	memset(r, 0, sizeof(*r));
	*reply = (struct msg *)r;
	*reply_size = sizeof(*r);

	user = pci_serv_find_user_by_task(task);
	if (!user)
		return ENOENT;

	if (user->pci_scan_itr == &user->pci_scan_result) {
		r->eof = 1;
		return 0;
	}

	node = list_entry(user->pci_scan_itr, struct pci_scan_node, link);
	pf = node->pf;
	r->id.busno = pf->bus->busno;
	r->id.dev_id = pf->dev_id;
	r->id.dev_class = pf->dev_class;
	r->id.dev = pf->dev;
	r->id.func = pf->func;
	memcpy(r->reg_base, pf->reg_base, sizeof(pf->reg_base));
	memcpy(r->reg_size, pf->reg_size, sizeof(pf->reg_size));
	r->irqline = pf->irq_line;

	user->pci_scan_itr = list_next(user->pci_scan_itr);

	LOG_FUNCTION_NAME_EXIT(0);
	return 0;
}

static int
pci_func_acquire(struct msg *msg, struct msg **reply, size_t *reply_size)
{
	struct pci_acquire_msg *m = (struct pci_acquire_msg *)msg;
	task_t task = m->hdr.task;
	struct pci_user *user;
	struct pci_func *pf;
	*reply = NULL;

	user = pci_serv_find_user_by_task(task);
	if (!user)
		return EEXIST;
	
	pf = pci_serv_find_func(user, &m->id);
	if (!pf)
		return EEXIST;
	if (pf->owner)
		return EBUSY;
	pf->owner = task;
	return 0;
}

static int
pci_func_enable(struct msg *msg, struct msg **reply, size_t *reply_size)
{
	struct pci_enable_msg *m = (struct pci_enable_msg *)msg;
	task_t task = m->hdr.task;
	uint8_t flag = m->flag;
	struct pci_user *user;
	struct pci_func *pf;
	*reply = NULL;

	user = pci_serv_find_user_by_task(task);
	if (!user)
		return EEXIST;
	
	pf = pci_serv_find_func(user, &m->id);
	if (!pf)
		return EEXIST;
	_pci_func_enable(pf, flag);
	return 0;
}

static int
pci_func_release(struct msg *msg, struct msg **reply, size_t *reply_size)
{
	struct pci_release_msg *m = (struct pci_release_msg *)msg;
	task_t task = m->hdr.task;
	struct pci_user *user;
	struct pci_func *pf;
	*reply = NULL;

	user = pci_serv_find_user_by_task(task);
	if (!user)
		return EEXIST;
	
	pf = pci_serv_find_func(user, &m->id);
	if (!pf)
		return EEXIST;
	pf->owner = 0;
	return 0;
}

/*
 * Message mapping
  */
struct msg_map {
	int	code;
	int	(*func)(struct msg *, struct msg **reply, size_t *reply_size);
};

static const struct msg_map pcimsg_map[] = {
	{ PCI_CONNECT,		pci_connect },
	{ PCI_PROBE_DEVICE,	pci_probe_deivce },
	{ PCI_PROBE_GET_RESULT,	pci_probe_get_result },
	{ PCI_FUNC_ACQUIRE,	pci_func_acquire },
	{ PCI_FUNC_ENABLE,	pci_func_enable },
	{ PCI_FUNC_RELEASE,	pci_func_release },
	{ PCI_DISCONNECT,	pci_disconnect },
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
		struct msg *reply;
		size_t reply_size;

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
					error = (*map->func)(&msg, &reply, &reply_size);
					break;
				}
				map++;
			}
		}
		/*
		 * Reply to the client.
		 */
		if (reply) {
			/* func has its own reply message */
			reply->hdr.status = error;
			reply->hdr.code = msg.hdr.code;
			reply->hdr.task = msg.hdr.task;
			msg_reply(obj, reply, reply_size);
			free(reply);
		} else {
			/* simply acts as RPC */
			msg.hdr.status = error;
			msg_reply(obj, &msg, sizeof(msg));
		}
	}
}

