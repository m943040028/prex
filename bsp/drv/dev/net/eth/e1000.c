#include <driver.h>
#include <pci.h>
#include <net.h>

#define	DEBUG_E1000	1

#include "e1000_hw.h"
#include "e1000.h"

#ifdef DEBUG_E1000
int debugflags = DBGBIT(INFO);
#endif

static int e1000_probe(struct driver *self);
static int e1000_init(struct driver *self);
static int e1000_net_init(struct net_driver *self);

struct e1000_hw {
	uint32_t	io_base;
	uint32_t	io_size;
	uint8_t		irqline; 
};

struct e1000_adaptor {
	irq_t		irq;		/* irq handle */

	int		num_tx_queues;
	int		num_rx_queues;
	struct e1000_tx_ring *tx_ring;
	struct e1000_rx_ring *rx_ring;

	struct e1000_hw	hw;
};

static list_t pci_device_list; /* list of pci devices probed */

struct driver e1000_driver = {
	/* name */	"e1000",
	/* devsops */	NULL,
	/* devsz */	sizeof(struct e1000_adaptor),
	/* flags */	0,
	/* probe */	e1000_probe,
	/* init */	e1000_init,
	/* shutdown */	NULL,
};

static struct net_driver_operations e1000_ops = {
	/* init */	e1000_net_init,
};

static struct net_driver e1000_net_driver = {
	/* driver */	&e1000_driver,
	/* ops */	&e1000_ops,
};

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
	er32(STATUS); /* wait for complete */

        /* Delay to allow any outstanding PCI transactions to complete before
	 * resetting the device
	 */
	delay_usec(10000);
	ctrl = er32(CTRL);

	/* reset phy */
	ew32(CTRL, (ctrl | E1000_CTRL_PHY_RST));
	delay_usec(5000);

	/* Issue a global reset to the MAC.  This will reset the chip's
	 * transmit, receive, DMA, and link units.  It will not effect
	 * the current PCI configuration.  The global reset bit is self-
	 * clearing, and should clear within a microsecond.
	 */
	ew32(CTRL, (ctrl | E1000_CTRL_RST));

	/* After MAC reset, force reload of EEPROM to restore power-on settings to
	 * device.  Later controllers reload the EEPROM automatically, so just wait
	 * for reload to complete.
	 */
	delay_usec(20000);

	/* Clear interrupt mask to stop board from generating interrupts */
	ew32(IMC, 0xffffffff);

	/* Clear any pending interrupt events. */
	icr = er32(ICR);

	return 0;
}

static void e1000_clean_tx_ring(struct e1000_adaptor *adapter,
				struct e1000_tx_ring *tx_ring)
{
}

static void e1000_clean_rx_ring(struct e1000_adaptor *adapter,
				struct e1000_rx_ring *rx_ring)
{
}

static void e1000_clean_all_tx_rings(struct e1000_adaptor *adaptor)
{
	int i;

	for (i = 0; i < adaptor->num_tx_queues; i++)
		e1000_clean_tx_ring(adaptor, &adaptor->tx_ring[i]);
}

static void e1000_clean_all_rx_rings(struct e1000_adaptor *adaptor)
{
	int i;

	for (i = 0; i < adaptor->num_rx_queues; i++)
		e1000_clean_rx_ring(adaptor, &adaptor->rx_ring[i]);
}

int e1000_match_func(uint16_t vendor, uint16_t device, uint32_t class)
{

	if (vendor == 0x8086 && device == E1000_DEV_ID_82540EM)
		return 1;
	else
		return 0;
}

static int
e1000_net_init(struct net_driver *self)
{
	struct pci_func *f;
	struct e1000_adaptor *adaptor;
	struct e1000_hw *hw;

	f = to_pci_func(pci_device_list);
	if (pci_func_configure(f) != 0) {
		DPRINTF(INFO, "cannot configure PCI interface\n");
	}

	adaptor = net_driver_private(self);
	hw = &adaptor->hw;

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

	e1000_clean_all_tx_rings(adaptor);
	e1000_clean_all_rx_rings(adaptor);

	return 0;
}

static int
e1000_init(struct driver *init)
{
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
	net_driver_attach(&e1000_net_driver);

	return 0;
}

