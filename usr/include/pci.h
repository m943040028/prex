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

#ifndef _PCI_H_
#define _PCI_H_

#include <sys/types.h>

struct pci_func;

typedef int (pci_match_func)(uint16_t,/* vendor */
			     uint16_t,/* device */
			     uint32_t /* class  */);

/* pci_func_enalbe() flags */
#define	PCI_MEM_ENABLE	0x1
#define PCI_IO_ENABLE	0x2

__BEGIN_DECLS
int	  pci_init(void);

list_t	  pci_probe_device(pci_match_func);
int	  pci_func_configure(struct pci_func *);
void	  pci_func_enable(struct pci_func *, uint8_t);
uint32_t  pci_func_get_reg_base(struct pci_func *, int);
uint32_t  pci_func_get_reg_size(struct pci_func *, int);
uint8_t	  pci_func_get_irqline(struct pci_func *);

void	  pci_conf_write(struct pci_func *, uint32_t, uint32_t);
uint32_t  pci_conf_read(struct pci_func *, uint32_t);

uint32_t  pci_bus_read32(paddr_t); 
void	  pci_bus_write32(paddr_t, uint32_t);
uint8_t	  pci_bus_read8(paddr_t); 
void	  pci_bus_write8(paddr_t, uint8_t);
struct pci_func *to_pci_func(list_t);
__END_DECLS

#endif /* _PCI_H_ */
