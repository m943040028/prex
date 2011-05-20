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

#include <sys/prex.h>
#include <sys/param.h>
#include <sys/cdefs.h>
#include <sys/list.h>
#include <sys/endian.h>
#include <ipc/pci.h>
#include <ipc/ipc.h>
#include <pci.h>
#include <uio.h>

#include <stdlib.h>

struct pci_scan_result {
	int             num_func;
	struct list	func_list;
	struct list	itr;
};

struct pci_handle {
	object_t	pci_object;
	struct pci_scan_result *result;
	struct uio_handle *uio;
};

struct pci_func_handle {
	struct pci_handle *pci_handle;
	struct list	link;
	pci_func_t      func;
	int		(*irq_handler)(void *args);
	void		*args;
	struct uio_mem	bar[6];
	uint32_t	reg_base[6];
	uint32_t	reg_size[6];
	int		irqline;
};

static void
pci_free_scan_result(struct pci_scan_result *result)
{
	struct pci_func_handle *handle;
	list_t n = list_first(&result->func_list);
	while (!list_empty(n)) {
		handle = list_entry(n, struct pci_func_handle, link);
		n = list_next(n);
		free(handle);
	}
	free(result);
}

struct pci_handle *
pci_attach(void)
{
	struct pci_handle *handle;
	struct msg msg;
	int ret;

	handle = malloc(sizeof(struct pci_handle));
	if (!handle)
		return NULL;
	memset(handle, 0, sizeof(handle));

	ret = object_lookup("!pci", &handle->pci_object)
	if (ret) {
		dprintf("cannot get pci object\n");
		return NULL;
	}

	msg.hdr.code = PCI_CONNECT;
	ret = msg_send(&handle->pci_object, &msg);
	if (ret) {
		dprintf("cannot connect to pci server\n");
		return NULL;
	}

	return handle;
}

int
pci_scan_device(struct pci_handle *handle, pci_probe_t probe)
{
	struct pci_probe_msg pm;
	struct pci_probe_reply pr;
	struct pci_scan_result *result;
	struct pci_func_handle *func_handle;
	int nr_probed = 0;
	int ret;
	pm.hdr.code = PCI_PROBE_DEVICE;
	pm.probe = probe;
	list_init(&result->func_list);
	list_init(&result->itr);

	/* if we have scan_result already, drop it */
	if (handle->result) {
		pci_free_scan_result(&handle->result);
	}

	result = malloc(sizeof(struct pci_scan_result));
	if (result)
		return ENOMEM;

	while (1) {
		ret = msg_send(&handle->pci_object, &pm);
		if (ret)
			return ret;

		ret = msg_recv(&handle->pci_object, &pr);
		if (pr.eof)
			break;

		/* we found a function */
		nr_probed++;
		func_handle = malloc(sizeof(struct pci_func_handle));
		if (!func_handle) {
			pci_free_scan_result(result);
			handle->result = NULL;
			return ENOMEM;
		}
		list_init(&func_handle->link);
		func_handle->func = pr.func;
		func_handle->pci_handle = handle;
		memcpy(func_handle->reg_base, pr.reg_base, sizeof(pr.reg_base));
		memcpy(func_handle->reg_size, pr.reg_size, sizeof(pr.reg_size));
		func_handle->irqline = pr.irqline;
		list_insert(&result->func_list, &func_handle->link);
	}

	handle->result = result;
	result->itr = list_first(&result->func_list);

	return 0;
}

pci_func_handle *
pci_get_func(struct pci_handle *handle)
{
	struct pci_scan_result *result = handle->result;
	if (!result)
		return result;
	if (result->itr == NULL)
		return NULL;
	handle = list_entry(result->itr, struct pci_func_handle, link);
	result->itr = list_next(&result->itr);
	return handle;
}

int
pci_func_enable(struct pci_func_handle *handle, uint8_t flag)
{
	struct pci_handle *pci_hanlde = handle->pci_handle;
	struct pci_enable_msg pe;
	int ret;

	pe.hdr.code = PCI_FUNC_ENABLE;
	pe.func = handle->func;
	pe.flag = flag;
	return msg_send(&pci_handle->pci_object, &pe);	
}

int
pci_func_acquire(struct pci_func_handle *handle)
{
	struct pci_handle *pci_hanlde = handle->pci_handle;
	struct pci_acquire_msg msg;
	int ret;

	msg.hdr.code = PCI_FUNC_ACQUIRE;
	msg.func = handle->func;
	msg.task = task_self();
	return msg_send(&pci_handle->pci_object, &msg);
}

int
pci_func_release(struct pci_func_handle *handle)
{
	struct pci_handle *pci_hanlde = handle->pci_handle;
	struct pci_release_msg msg;
	int ret;

	msg.hdr.code = PCI_FUNC_RELEASE;
	msg.func = handle->func;
	msg.task = task_self();
	return msg_send(&pci_handle->pci_object, &msg);
}


