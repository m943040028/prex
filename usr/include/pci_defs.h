#ifndef _PCI_DEFS_H_
#define _PCI_DEFS_H_

#include <sys/types.h>

/* pci_func_enable() flags */
#define	PCI_MEM_ENABLE	0x1
#define PCI_IO_ENABLE	0x2

enum match_flag {
	PCI_MATCH_VENDOR_PRODUCT = 0x1,
	PCI_MATCH_VENDOR_ONLY	= 0x2,
	PCI_MATCH_PRODUCT_ONLY	= 0x4,
	PCI_MATCH_CLASS		= 0x8,
	PCI_MATCH_PROBE		= 0x10,
};

typedef struct pci_probe_s {
	uint16_t	pci_vendor;
	uint16_t	pci_product;
	uint32_t	pci_class;
	enum match_flag	type;
} pci_probe_t;

typedef struct pci_func_id_s {
	uint32_t	dev_id;
	uint32_t	dev_class;
	uint32_t	busno;
	uint32_t	func;
	uint32_t	dev;
} pci_func_id_t;

#define pci_func_vendor(func)\
	((func).dev_id >> 16)
#define pci_func_product(func)\
	((func).dev_id & 0xffff)
#define pci_func_class(func)\
	((func).dev_class)
#define pci_func_busno(func) \
	((func).busno)
#define pci_func_dev(func)\
	((func).dev)
#define pci_func_func(func)\
	((func).func)
#endif
