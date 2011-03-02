#include <driver.h>
#include <pci.h>

#define DEBUG_E1000

#ifdef DEBUG_E1000
enum {
	DBG_INFO,
};
#define DBGBIT(x)       (1<<DBG_##x)
static int debugflags = DBGBIT(INFO);

#define DPRINTF(what, fmt, ...) do { \
	if (debugflags & DBGBIT(what)) \
		printf("e1000: " fmt, ## __VA_ARGS__); \
	} while (0)
#else
#define DPRINTF(what, fmt, ...) do {} while (0)                                                                                                                                                                                               
#endif

struct e1000_dev {
	uint32_t	io_base;
	uint32_t	io_size;
	uint8_t		irqline; 
};
static struct e1000_dev dev;

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
	DPRINTF(INFO, "probed eepro1000 NIC\n");
	pci_func_configure(f);
	
	dev.io_base = pci_func_get_reg_base(f, 0);
	dev.io_size = pci_func_get_reg_size(f, 0);
	dev.irqline = pci_func_get_irqline(f);
	DPRINTF(INFO, "mmio_base=%08x, size=%08x\n",
		dev.io_base, dev.io_size);
	DPRINTF(INFO, "irqline=%d\n", dev.irqline);
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
