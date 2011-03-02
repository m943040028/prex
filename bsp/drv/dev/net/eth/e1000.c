#include <driver.h>
#include <pci.h>

int e1000_match_func(uint16_t vendor, uint16_t device, uint32_t class)
{

	if (vendor == 0x8086 && device == 0x100e)
		return 1;
	else
		return 0;
}

static int
e1000_init(struct driver *self)
{
	return 0;
}

static int
e1000_probe(struct driver *self)
{
	list_t device_list;
	struct pci_func *f;

	device_list = pci_probe_device(e1000_match_func);
	if (!device_list) {
		return ENODEV;
	}
	f = to_pci_func(device_list);
	pci_func_configure(f);
	pci_func_enable(f, PCI_MEM_ENABLE);
	
	return 0;
}

struct driver e1000_driver = {
	/* name */      "e1000",
	/* devsops */   NULL,
	/* devsz */     0,
	/* flags */     0,
	/* probe */     e1000_probe,
	/* init */      e1000_init,
	/* shutdown */  NULL,
};
