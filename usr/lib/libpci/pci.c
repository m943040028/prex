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
 * pci.c - User-space pci library
 */

#define DBG
#define MODULE_NAME  "libpci"

#include <sys/prex.h>
#include <sys/param.h>
#include <sys/cdefs.h>
#include <sys/list.h>
#include <sys/endian.h>
#include <sys/dbg.h>
#include <assert.h>
#include <ipc/pci.h>
#include <ipc/ipc.h>
#include <pci.h>
#include <uio.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef DBG
static int debugflags = DBGBIT(INFO);
#endif


struct pci_handle {
	object_t	pci_object;
	struct list	func_list;
	struct uio_handle *uio;
};

struct pci_func_handle {
	struct pci_handle *pci_handle;
	struct list	link;
	pci_func_id_t	func;
	int		(*irq_handler)(void *args);
	void		*args;
	struct uio_mem	*bar[6];
	uint32_t	reg_base[6];
	uint32_t	reg_size[6];
	int		irqline;
};

static void
pci_free_func_hanlde(struct pci_handle *handle)
{
	struct pci_func_handle *func_handle;
	list_t n = list_first(&handle->func_list);
	while (!list_empty(&handle->func_list)) {
		func_handle = list_entry(n, struct pci_func_handle, link);
		list_remove(&func_handle->link);
		free(func_handle);
		n = list_next(n);
	}
}

struct pci_handle *
pci_attach(void)
{
	struct pci_handle *handle;
	struct msg msg;
	int ret;
	LOG_FUNCTION_NAME_ENTRY();

	handle = malloc(sizeof(struct pci_handle));
	if (!handle)
		return NULL;
	memset(handle, 0, sizeof(handle));
	list_init(&handle->func_list);

	ret = object_lookup("!pci", &handle->pci_object);
	if (ret) {
		DPRINTF(INFO, "cannot get pci object\n");
		return NULL;
	}

	msg.hdr.code = PCI_CONNECT;
	ret = msg_send(handle->pci_object, &msg, sizeof(msg));
	if (ret) {
		DPRINTF(INFO, "cannot connect to pci server\n");
		goto err_free_handle;
	}
	if (msg.hdr.status)
		goto err_free_handle;

	LOG_FUNCTION_NAME_EXIT_PTR(handle);
	return handle;
err_free_handle:
	free(handle);
	LOG_FUNCTION_NAME_EXIT_PTR(NULL);
	return NULL;
}

void
pci_detach(struct pci_handle *handle)
{
	struct msg msg;

	msg.hdr.code = PCI_DISCONNECT;
	msg_send(handle->pci_object, &msg, sizeof(msg));
	pci_free_func_hanlde(handle);
	free(handle);
}

int
pci_scan_device(struct pci_handle *handle, pci_probe_t probe)
{
	struct pci_probe_msg pm;
	struct msg msg;
	int ret;

	LOG_FUNCTION_NAME_ENTRY();
	if (!list_empty(&handle->func_list))
		pci_free_func_hanlde(handle);
	list_init(&handle->func_list);

	pm.hdr.code = PCI_PROBE_DEVICE;
	pm.probe = probe;
	ret = msg_send(handle->pci_object, &pm, sizeof(pm));
	if (ret) {
		LOG_FUNCTION_NAME_EXIT(ret);
		return ret;
	}
	LOG_FUNCTION_NAME_EXIT(msg.hdr.status);
	return msg.hdr.status;
}

int
pci_get_func(struct pci_handle *handle, struct pci_func_handle **func)
{
	struct pci_func_handle *func_handle;
	struct pci_probe_reply pr;
	int ret;
	LOG_FUNCTION_NAME_ENTRY();
	pr.hdr.code = PCI_PROBE_GET_RESULT;
	ret = msg_send(handle->pci_object, &pr, sizeof(pr));
	if (ret) {
		LOG_FUNCTION_NAME_EXIT(ret);
		return ret;
	}
	if (pr.hdr.status) {
		LOG_FUNCTION_NAME_EXIT(pr.hdr.status);
		return pr.hdr.status;
	}

	if (pr.eof) {
		LOG_FUNCTION_NAME_EXIT(EOF);
		return EOF;
	}

	/* we found a function */
	func_handle = malloc(sizeof(struct pci_func_handle));
	if (!func_handle) {
		LOG_FUNCTION_NAME_EXIT(ENOMEM);
		return ENOMEM;
	}

	list_init(&func_handle->link);
	func_handle->func = pr.func;
	func_handle->pci_handle = handle;
	memcpy(func_handle->reg_base, pr.reg_base, sizeof(pr.reg_base));
	memcpy(func_handle->reg_size, pr.reg_size, sizeof(pr.reg_size));
	func_handle->irqline = pr.irqline;
	list_insert(&handle->func_list, &func_handle->link);
	*func = func_handle;

	LOG_FUNCTION_NAME_EXIT(0);
	return 0;
}

int
pci_func_enable(struct pci_func_handle *handle, uint8_t flag)
{
	struct pci_handle *pci_handle = handle->pci_handle;
	struct pci_enable_msg msg;

	msg.hdr.code = PCI_FUNC_ENABLE;
	msg.func = handle->func;
	msg.flag = flag;
	return msg_send(pci_handle->pci_object, &msg, sizeof(msg));
}

int
pci_func_acquire(struct pci_func_handle *handle)
{
	struct pci_handle *pci_handle = handle->pci_handle;
	struct pci_acquire_msg msg;

	msg.hdr.code = PCI_FUNC_ACQUIRE;
	msg.func = handle->func;
	return msg_send(pci_handle->pci_object, &msg, sizeof(msg));
}

int
pci_func_release(struct pci_func_handle *handle)
{
	struct pci_handle *pci_handle = handle->pci_handle;
	struct pci_release_msg msg;

	msg.hdr.code = PCI_FUNC_RELEASE;
	msg.func = handle->func;
	return msg_send(pci_handle->pci_object, &msg, sizeof(msg));
}

void
pci_func_get_mem_region(struct pci_func_handle *handle, int bar,
			struct pci_mem_region *region)
{
	ASSERT(region != NULL);
	region->base = handle->reg_base[bar];
	region->size = handle->reg_size[bar];
}

void
pci_func_get_irqline(struct pci_func_handle *handle, uint8_t *irqline)
{
	ASSERT(irqline != NULL);
	*irqline = handle->irqline;
}

void
pci_func_get_func_id(struct pci_func_handle *handle, pci_func_id_t *func)
{
	ASSERT(func != NULL);
	*func = handle->func;
}

