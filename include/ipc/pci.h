/*
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

#ifndef _IPC_PCI_H
#define _IPC_PCI_H

#include <ipc/ipc.h>
#include <pci_defs.h>

struct pci_probe_msg {
	struct msg_header hdr;
	pci_probe_t	probe;
};

struct pci_probe_reply {
	struct msg_header hdr;
	pci_func_id_t	func;
	uint32_t	reg_base[6];
	uint32_t	reg_size[6];
	uint16_t	irqline;
	uint8_t		eof;
};

struct pci_acquire_msg {
	struct msg_header hdr;
	pci_func_id_t	func;
};

struct pci_enable_msg {
	struct msg_header hdr;
	pci_func_id_t	func;
	uint8_t		flag;
};

struct pci_release_msg {
	struct msg_header hdr;
	pci_func_id_t	func;
};

/*
 * Messages for pci object
 */
#define PCI_CONNECT		0x00000500
#define PCI_PROBE_DEVICE	0x00000501
#define PCI_PROBE_GET_RESULT	0x00000502
#define PCI_FUNC_ACQUIRE	0x00000503
#define PCI_FUNC_ENABLE		0x00000504
#define PCI_FUNC_RELEASE	0x00000505
#define PCI_DISCONNECT		0x00000506

#endif /* !_IPC_PCI_H */
