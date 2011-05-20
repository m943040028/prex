
#ifndef _PCI_DEFS_H_
#define _PCI_DEFS_H_

enum match_flag {
	PCI_MATCH_VENDOR	= 0x1,
	PCI_MATCH_DEVICE	= 0x2,
	PCI_MATCH_CLASS		= 0x4,
	PCI_MATCH_ALL		= 0x8,
};

typedef struct pci_probe {
	uint16_t	pci_vendor;
	uint16_t	pci_device;
	uint32_t	pci_class;
	enum match_flag	type;
} pci_probe_t;

typedef struct pci_func {
	uint32_t	busno;
	uint32_t	dev_id;
	uint32_t	func_id;
} pci_func_t;

#endif
