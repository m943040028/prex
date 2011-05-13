#ifndef _PCI_PLAT_H_
#define _PCI_PLAT_H_

__BEGIN_DECLS
int	platform_pci_init(void);
int	platform_pci_probe_irq(uint8_t irqline);
__END_DECLS

#endif
