#include <driver.h>
#include <pci.h>

#define	DEBUG_E1000	1

#include "e1000_hw.h"
#include "e1000.h"

#ifdef DEBUG_E1000
int debugflags = DBGBIT(INFO);
#endif

struct e1000_hw {
	device_t	dev;		/* device object */
	irq_t		irq;		/* irq handle */

	uint32_t	io_base;
	uint32_t	io_size;
	uint8_t		irqline; 
};

static list_t pci_device_list; /* list of pci devices probed */

static int
e1000_hw_reset(struct e1000_hw *hw)
{
        uint32_t ctrl;
        uint32_t icr;

	/* Clear interrupt mask to stop board from generating interrupts */
	ew32(IMC, 0xffffffff);

	/* Disable the Transmit and Receive units.  Then delay to allow
	 * any pending transactions to complete before we hit the MAC with
	 * the global reset.
	 */
	ew32(RCTL, 0);
	ew32(TCTL, E1000_TCTL_PSP);
	er32(STATUS);

        /* Delay to allow any outstanding PCI transactions to complete before
	 * resetting the device
	 */
	delay_usec(10000);

	/* reset phy */
	ew32(CTRL, (ctrl | E1000_CTRL_PHY_RST));
	delay_usec(5000);

	/* Issue a global reset to the MAC.  This will reset the chip's
	 * transmit, receive, DMA, and link units.  It will not effect
	 * the current PCI configuration.  The global reset bit is self-
	 * clearing, and should clear within a microsecond.
	 */
	ew32(CTRL, (ctrl | E1000_CTRL_RST));

	/* Clear interrupt mask to stop board from generating interrupts */
	ew32(IMC, 0xffffffff);

	/* Clear any pending interrupt events. */
	icr = er32(ICR);

	return 0;
}

int e1000_match_func(uint16_t vendor, uint16_t device, uint32_t class)
{

	if (vendor == 0x8086 && device == E1000_DEV_ID_82540EM)
		return 1;
	else
		return 0;
}

static int
e1000_init(struct driver *self)
{
	struct pci_func *f;
	device_t dev;
	struct e1000_hw *hw;

	f = to_pci_func(pci_device_list);
	if (pci_func_configure(f) != 0) {
		DPRINTF(INFO, "cannot configure PCI interface\n");
	}

	dev = device_create(self, "net0", D_NET|D_PROT);
	hw = device_private(dev);
	hw->dev = dev;

	hw->io_base = pci_func_get_reg_base(f, 0);
	hw->io_size = pci_func_get_reg_size(f, 0);
	hw->irqline = pci_func_get_irqline(f);
	DPRINTF(INFO, "mmio_base=%08x, size=%08x\n",
		hw->io_base, hw->io_size);
	DPRINTF(INFO, "irqline=%d\n", hw->irqline);
	pci_func_enable(f, PCI_MEM_ENABLE);

	if (e1000_hw_reset(hw) < 0) {
		DPRINTF(INFO, "cannot reset hardware\n");
		return EIO;
	}

	return 0;
}

static int
e1000_probe(struct driver *self)
{
	pci_device_list = pci_probe_device(e1000_match_func);
	if (!pci_device_list) {
		return ENODEV;
	}
	DPRINTF(INFO, "probed eepro1000 NIC\n");

	return 0;
}

struct driver e1000_driver = {
	/* name */      "e1000",
	/* devsops */   NULL,
	/* devsz */     sizeof(struct e1000_hw),
	/* flags */     0,
	/* probe */     e1000_probe,
	/* init */      e1000_init,
	/* shutdown */  NULL,
};
