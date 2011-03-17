#include <driver.h>
#include <pci.h>
#include <net.h>

#define	DEBUG_E1000	1

#include "e1000_hw.h"
#include "e1000.h"

#ifdef DEBUG_E1000
int debugflags = DBGBIT(INFO);
#endif

/* driver layer operations */
static int e1000_probe(struct driver *);
static int e1000_init(struct driver *);

/* net_driver layer operations */
static int e1000_net_init(struct net_driver *);
static int e1000_net_start(struct net_driver *);
static int e1000_net_stop(struct net_driver *);
static int e1000_init_buf(struct net_driver *, init_buf_req_t);
static int e1000_alloc_queue(struct net_driver *, queue_req_t);
static int e1000_release_queue(struct net_driver *, queue_type_t);
static int e1000_push_buf(struct net_driver *, queue_type_t);
static int e1000_pop_buf(struct net_driver *, queue_type_t);

struct e1000_hw {
	uint32_t	io_base;
	uint32_t	io_size;
	uint8_t		irqline; 
};

struct e1000_adaptor {
	irq_t		irq;		/* irq handle */

	int		num_tx_queues;
	int		num_rx_queues;
	struct e1000_tx_desc *tx_desc;
	struct e1000_rx_desc *rx_desc;

	struct e1000_hw	hw;
};

static list_t pci_device_list; /* list of pci devices probed */

static struct net_driver_operations e1000_ops = {
	/* init */	e1000_net_init,
	/* start */	e1000_net_start,
	/* stop */	e1000_net_stop,
	/* alloc_queue */ e1000_alloc_queue,
	/* release_queue */ e1000_release_queue,
	/* submit_buf */ e1000_submit_buf,
};

static struct net_driver e1000_net_driver = {
	/* driver */	&e1000_driver,
	/* ops */	&e1000_ops,
	/* inteface */	NETIF_ETHERNET,
};

struct driver e1000_driver = {
	/* name */	"e1000",
	/* devsops */	NULL,
	/* devsz */	sizeof(struct e1000_adaptor),
	/* flags */	0,
	/* probe */	e1000_probe,
	/* init */	e1000_init,
	/* shutdown */	NULL,
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

	/* After MAC reset, force reload of EEPROM to restore power-on
	 * settings to device.  Later controllers reload the EEPROM
	 * automatically, so just wait for reload to complete.
	 */
	delay_usec(20000);

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

	adaptor->irq = irq_attach(hw->irqline, IPL_NET, 0,
				  e1000_isr, e1000_ist, adaptor);

	return 0;
}

static int 
e1000_net_start(struct net_driver *self)
{
	return 0;
}

static int
e1000_net_stop(struct net_driver *self)
{
	return 0;
}

static int
e1000_alloc_queue(struct net_driver *self, queue_req_t req)
{
	struct e1000_adaptor *adaptor;
	adaptor = net_driver_private(self);
	int i, nbufs = req->nbufs;
	size_t desc_size;
	datagram_buffer_t *bufs req->bufs;

	if (req->type == TX_QUEUE) {
		struct e1000_tx_desc *desc;
		adaptor->num_tx_queues = nbufs;

		/* allocate tx descriptors */
		desc_size = sizeof(struct e1000_tx_desc) *
				adaptor->num_tx_queues;
		adaptor->tx_desc = kmem_alloc(desc_size);
		memset(adaptor->tx_desc, 0, desc_size);
		desc = adaptor->tx_desc;

		/*
		 * Setup the transmit ring registers.
		 */
		sw(TDBAL, adaptor->tx_desc);
		sw(TDBAH, 0);
		sw(TDLEN, rx_desc_size);
		sw(TDH, 0); /* tx descriptor head */
		sw(TDT, 0); /* tx descriptor tail */

		for (i = 0; i < nbufs; i++)
			desc[i].buffer_addr = dqbuf_get_paddr(bufs[i]);

	} else if (req->type == RX_QUEUE) {
		struct e1000_rx_desc *desc;
		adaptor->num_rx_queues = nbufs;

		/* allocate rx descriptors */
		desc_size = sizeof(struct e1000_rx_desc) *
				adaptor->num_rx_queues;
		adaptor->rx_desc = kmem_alloc(desc_size);
		memset(adaptor->rx_desc, 0, desc_size);
		desc = adaptor->tx_desc;

		adaptor->num_rx_queues = E1000_RXDESC_NR;
		/*
		 * Setup the receive ring registers.
		 */
		sw(RDBAL, adaptor->rx_desc);
		sw(RDBAH, 0);
		sw(RDLEN, tx_desc_size);
		sw(RDH, 0); /* rx descriptor head */
		sw(RDT, 0); /* rx descriptor tail */

		for (i = 0; i < nbufs; i++)
			desc[i].buffer_addr = dqbuf_get_paddr(bufs[i]);
	} else
		return EINVAL;

	return 0;
}

static int
e1000_release_queue(struct net_driver *self, queue_type_t type)
{
	struct e1000_adaptor *adaptor;
	adaptor = net_driver_private(self);

	switch (type) {
	case TX_QUEUE:
		kmem_free(adaptor->tx_desc);
		break;
	case RX_QUEUE:
		kmem_free(adaptor->tx_desc);
		break;
	default:
		return EINVAL;
	}

	return 0;
}

/*
 * this method is called via net coordinator when
 * a READY buffer is ready for transmiision or an FREE
 * buffer is ready for receving.
 *
 * return ENOMEM if transmit buffer is full
 */
static int
e1000_push_buf(struct net_driver *self, queue_type_t type)
{
	struct e1000_adaptor *adaptor;
	adaptor = net_driver_private(self);
	uint32_t head, tail;
	struct e1000_tx_desc *desc;
	dqbuf_state_t state;

	state = dqbuf_get_state(buf);

	switch (type) 
	{
	case QUEUE_RX: { /* a ready buffer is queued for transmition */
		head = sr(TDH);
		tail = sr(TDL);

		DPRINTF(DEBUG, "%s(): TX: head=%d, tail=%d\n",
			__func__, head, tail);

		if (tail == head)
			return ENOMEM;

		desc = &adaptor->tx_desc[tail];

		/* Mark this descriptor ready. */
		desc->lower.data = E1000_TXD_CMD_IFCS | E1000_TXD_CMD_RS;
		desc->lower.flags.length = dqbuf_get_data_size(buf);
		desc->upper.data = 0; /* status */

		/* Mark end-of-packet if this is last descriptor */
		if (tail == adaptor->num_tx_queues - 1)
			desc->lower.data |= E1000_TXD_CMD_EOP;

		/* Advance the counter */
		tail = (tail + 1) % adaptor->num_tx_queues;
		sw(TDL, tail);
		break;
	}
	case QUEUE_YX: { /* an empty buffer is queued for receiving */
		head = sr(RDH);
		tail = sr(RDL);

		DPRINTF(DEBUG, "%s(): RX: head=%d, tail=%d\n",
			__func__, head, tail);

		desc = &adaptor->rx_desc[tail];

		/* Mark this descriptor ready. */
		desc->lower.data = 0;
		desc->lower.flags.length = dqbuf_get_buf_size(buf);
		desc->upper.data = 0; /* status */
		desc->buffer_addr = dqbuf_get_paddr(buf);

		/* Mark end-of-packet if this is last descriptor */
		if (tail == adaptor->num_rx_queues - 1)
			desc->lower.data |= E1000_TXD_CMD_EOP;

		/* Advance the counter */
		tail = (tail + 1) % adaptor->num_rx_queues;
		sw(RDL, head);
	}

	default:
		return EINVAL:
	}

	return 0;
}

static int
e1000_init(struct driver *init)
{
	/* Nothings to do in driver layer init() */
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
