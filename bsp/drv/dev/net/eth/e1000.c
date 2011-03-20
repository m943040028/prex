#include <driver.h>
#include <pci.h>
#include <net.h>

#define	DEBUG_E1000	1

#include "e1000_hw.h"
#include "e1000.h"

#ifdef DEBUG_E1000
int debugflags = DBGBIT(INFO);
#endif

/* forward declaration */
static int e1000_alloc_iodesc(struct e1000_adaptor *);
static int e1000_isr(void *);
static int e1000_ist(void *);
static void e1000_fill_rx_buffer(struct e1000_adaptor *);

/* driver layer operations */
static int e1000_probe(struct driver *);
static int e1000_init(struct driver *);

/* net_driver layer operations */
static int e1000_net_init(struct net_driver *);
static int e1000_net_start(struct net_driver *);
static int e1000_net_stop(struct net_driver *);
static int e1000_transmit(struct net_driver *, dbuf_t);

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
	dbuf_t		*rx_buf;
	dbuf_t		*tx_buf;
	int		rx_ptr;
	int		tx_ptr;

	struct e1000_hw	hw;
};

static list_t pci_device_list; /* list of pci devices probed */

static struct net_driver_operations e1000_ops = {
	/* init */	e1000_net_init,
	/* start */	e1000_net_start,
	/* stop */	e1000_net_stop,
	/* transmit */	e1000_transmit,
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

	e1000_alloc_iodesc(adaptor);

	adaptor->irq = irq_attach(hw->irqline, IPL_NET, 0,
				  e1000_isr, e1000_ist, adaptor);

	return 0;
}

static void
e1000_link_changed(struct e1000_adaptor *adaptor)
{
	return 0;
}

/* load as many rx buffers as possible */
static void
e1000_fill_rx_buffer(struct e1000_adaptor *adaptor)
{
	int tail = sr(RDL);
	int head = sr(RDH)

	while (ENOMEM != dq_buf_request(dbuf) || tail != head)
	{
		desc[tail].buffer_addr = dq_buf_get_paddr(dbuf);
		desc[tail].length = dq_buf_buf_length(dbuf);
		adaptor->rx_bufs[tail] = dbuf;
		if (++tail >= adaptor->num_rx_queue)
			tail = 0;
	}
	sw(RDL, tail);
}

static int
e1000_rx(struct e1000_adaptor *adaptor)
{
	dbuf_t rx_buf;
	int head, rx_ptr;

	head = sr(RDH);
	rx_ptr = adaptor->rx_ptr;

	while (rx_ptr < head)
	{
		rx_buf = adaptor->rx_bufs[rx_ptr];
		dq_buf_add(rx_buf);

		if (++rx_ptr >= adaptor->num_rx_queue)
			rx_ptr = 0;
	}

	adaptor->rx_ptr = rx_ptr;

	/* reload rx_buffer */
	e1000_fill_rx_buffer(adaptor);
	return 0;
}

static int
e1000_release_tx_buffer(adaptor)
{
	dbuf_t tx_buf;
	int head, tx_ptr;

	head = sr(TDH);
	while (tx_ptr < head)
	{
		tx_buf = adaptor->tx_bufs[tx_ptr];
		dq_buf_release(tx_buf);

		if (++tx_ptr >= adaptor->num_rx_queue)
			tx_ptr = 0;
	}

	adaptor->tx_ptr = tx_ptr;
}

static int
e1000_isr(void *args)
{
	struct e1000_adaptor *adaptor;
	adaptor = args;

	uint32_t cause;

	/* Read the Interrupt Cause Read register. */
	if ((cause = e1000_reg_read(e, E1000_REG_ICR)))
	{   
		if (cause & E1000_ICR_LSC)
			e1000_link_changed(adaptor);

		if (cause & (E1000_ICR_RXO | E1000_REG_ICR_RXT0))
			e1000_rx(adaptor);

		if ((cause & E1000_ICR_TXQE) ||
				(cause & E1000_ICR_TXDW))
			e1000_release_tx_buffer(adaptor);
	}

}

static int
e1000_ist(void *args)
{
	return 0;
}

static int 
e1000_net_start(struct net_driver *self)
{
	struct e1000_adaptor *adaptor;
	adaptor = net_driver_private(self);
	u32 rctl, tctl;

	e1000_fill_rx_buffer(adaptor);

	/* start RX & TX engine */
	rctl = sr(RCTL);
	/* TODO: should depend on requested buffer size */
	rctl | E1000_RCTL_EN | E1000_RCTL_BSEX | E1000_RCTL_SZ_4096;
	sw(RCTL, rctl);
	tctl = src(TCTL);
	tctl |= E1000_TCTL_EN | E1000_TCTL_PSP;
	sw(TCTL, tctl);

	return 0;
}

static int
e1000_net_stop(struct net_driver *self)
{
	return 0;
}

static int
e1000_alloc_iodesc(struct e1000_adaptor *adaptor)
{
	size_t desc_size, buf_size;
	struct e1000_tx_desc *desc;
	struct e1000_rx_desc *desc;
	dbuf_t dbuf;

	adaptor->num_tx_queues = E1000_NUM_TX_QUEUE;
	adaptor->num_rx_queues = E1000_NUM_RX_QUEUE;

	/* allocate tx descriptors */
	desc_size = sizeof(struct e1000_tx_desc) *
		adaptor->num_tx_queues;
	buf_size = sizeof(dbuf_t) * adaptor->num_tx_queues;
	adaptor->tx_desc = kmem_alloc(desc_size);
	memset(adaptor->tx_desc, 0, desc_size);
	adaptor->tx_bufs = kmem_alloc(buf_size);
	memset(adaptor->tx_bufs, 0 ,buf_size);
	desc = adaptor->tx_desc;

	/*
	 * Setup the transmit ring registers.
	 */
	sw(TDBAL, adaptor->tx_desc);
	sw(TDBAH, 0);
	sw(TDLEN, rx_desc_size);
	sw(TDH, 0); /* tx descriptor head */
	sw(TDT, 0); /* tx descriptor tail */
	adaptor->tx_ptr = 0;

	/* allocate rx descriptors */
	desc_size = sizeof(struct e1000_rx_desc) *
		adaptor->num_rx_queues;
	buf_size = sizeof(dbuf_t) * adaptor->num_rx_queues;
	adaptor->rx_desc = kmem_alloc(desc_size);
	memset(adaptor->rx_desc, 0, desc_size);
	adaptor->rx_bufs = kmem_alloc(buf_size);
	memset(adaptor->rx_bufs, 0 ,buf_size);
	desc = adaptor->tx_desc;

	/*
	 * Setup the receive ring registers.
	 */
	sw(RDBAL, adaptor->rx_desc);
	sw(RDBAH, 0);
	sw(RDLEN, tx_desc_size);
	sw(RDH, 0); /* rx descriptor head */
	sw(RDT, 0); /* rx descriptor tail */
	adaptor->rx_ptr = 0;

	return 0;
}

/*
 * this method is called via net coordinator when
 * a READY buffer is ready for transmiision
 *
 * return ENOMEM if transmit buffer is full
 */
static int
e1000_transmit(struct net_driver *self, queue_type_t type)
{
	struct e1000_adaptor *adaptor;
	adaptor = net_driver_private(self);
	uint32_t head, tail;
	struct e1000_tx_desc *desc;

	head = sr(TDH);
	tail = sr(TDL);

	DPRINTF(DEBUG, "%s(): head=%d, tail=%d\n",
			__func__, head, tail);

	if (tail == head)
		return ENOMEM;

	desc = &adaptor->tx_desc[tail];

	/* Mark this descriptor ready. */
	desc->lower.data = E1000_TXD_CMD_IFCS | E1000_TXD_CMD_RS;
	desc->lower.flags.length = dqbuf_get_data_length(buf);
	desc->upper.data = 0; /* status */

	/* Mark end-of-packet if this is last descriptor */
	if (tail == adaptor->num_tx_queues - 1)
		desc->lower.data |= E1000_TXD_CMD_EOP;

	/* Advance the counter */
	tail = (tail + 1) % adaptor->num_tx_queues;
	sw(TDL, tail);

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
