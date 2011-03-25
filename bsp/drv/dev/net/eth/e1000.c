#include <driver.h>
#include <pci.h>
#include <net.h>

#define	DEBUG_E1000	1

#include "e1000_hw.h"
#include "e1000.h"

#ifdef DEBUG_E1000
int debugflags = DBGBIT(INFO) | DBGBIT(TRACE) | DBGBIT(RX);
#endif

struct e1000_hw {
	uint32_t	io_base;
	uint32_t	io_size;
	uint8_t		irqline; 
};

struct e1000_adaptor {
	irq_t		irq;		/* irq handle */
	struct net_driver *driver;

	int		num_tx_queues;
	int		num_rx_queues;
	struct e1000_tx_desc *tx_desc;
	struct e1000_rx_desc *rx_desc;
	dbuf_t		*rx_bufs;
	dbuf_t		*tx_bufs;
	int		rx_ptr;
	int		tx_ptr;

	struct e1000_hw	hw;
};

/* forward declaration */
static int e1000_alloc_iodesc(struct e1000_adaptor *);
static int e1000_isr(void *);
static void e1000_fill_rx_buffer(struct e1000_adaptor *);

/* driver layer operations */
static int e1000_probe(struct driver *);
static int e1000_init(struct driver *);

/* net_driver layer operations */
static int e1000_net_init(struct net_driver *);
static int e1000_net_start(struct net_driver *);
static int e1000_net_stop(struct net_driver *);
static int e1000_transmit(struct net_driver *, dbuf_t);

/* global definitions */
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

static struct netdrv_ops e1000_netdrv_ops = {
	/* init */	e1000_net_init,
	/* start */	e1000_net_start,
	/* stop */	e1000_net_stop,
	/* transmit */	e1000_transmit,
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

	adaptor = netdrv_private(self);
	adaptor->driver = self;
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
				  e1000_isr, IST_NONE, adaptor);

	/* add braodcast address to rx filter */
	ew32_p(RA, 0xffffffff, 0);
	ew32_p(RA, 0xffff | E1000_RAH_AV, 1);

	return 0;
}

static void
e1000_link_changed(struct e1000_adaptor *adaptor)
{
}

/* load as many rx buffers as possible */
static void
e1000_fill_rx_buffer(struct e1000_adaptor *adaptor)
{
	struct e1000_hw *hw = &adaptor->hw;
	int tail = er32(RDT);
	int head = er32(RDH);
	dbuf_t dbuf;
	struct e1000_rx_desc *desc = adaptor->rx_desc;

	LOG_FUNCTION_NAME_ENTRY();

	while (ENOMEM != dbuf_request(adaptor->driver, &dbuf))
	{
		desc[tail].buffer_addr =
			cpu_to_le64(dbuf_get_paddr(dbuf));
		desc[tail].length =
			cpu_to_le16(dbuf_get_size(dbuf));
		adaptor->rx_bufs[tail] = dbuf;
		if (++tail >= adaptor->num_rx_queues)
			tail = 0;
		if (tail == head)
			break;
	}
	ew32(RDT, tail);
	LOG_FUNCTION_NAME_EXIT_NORET();
}

static int
e1000_rx(struct e1000_adaptor *adaptor)
{
	struct e1000_hw *hw = &adaptor->hw;
	struct e1000_rx_desc *desc;
	dbuf_t rx_buf;
	int head, rx_ptr;
	LOG_FUNCTION_NAME_ENTRY();

	head = er32(RDH);
	rx_ptr = adaptor->rx_ptr;

	while (rx_ptr < head)
	{
		desc = &adaptor->rx_desc[rx_ptr];
		DPRINTF(RX, "frame recevied, length=%08x\n",
			le16_to_cpu(desc->length));
		rx_buf = adaptor->rx_bufs[rx_ptr];
		dbuf_set_data_length(rx_buf, le16_to_cpu(desc->length));
		dbuf_add(adaptor->driver, rx_buf);

		if (++rx_ptr >= adaptor->num_rx_queues)
			rx_ptr = 0;
	}

	adaptor->rx_ptr = rx_ptr;

	/* reload rx_buffer */
	e1000_fill_rx_buffer(adaptor);
	LOG_FUNCTION_NAME_EXIT(0);
	return 0;
}

static int
e1000_release_tx_buffer(struct e1000_adaptor *adaptor)
{
	struct e1000_hw *hw = &adaptor->hw;
	dbuf_t tx_buf;
	int head, tx_ptr;

	head = er32(TDH);
	while (tx_ptr < head)
	{
		tx_buf = adaptor->tx_bufs[tx_ptr];
		dbuf_release(adaptor->driver, tx_buf);

		if (++tx_ptr >= adaptor->num_rx_queues)
			tx_ptr = 0;
	}

	adaptor->tx_ptr = tx_ptr;
	return 0;
}

static int
e1000_isr(void *args)
{
	struct e1000_adaptor *adaptor = args;
	struct e1000_hw *hw = &adaptor->hw;
	uint32_t cause;
	LOG_FUNCTION_NAME_ENTRY();

	/* Read the Interrupt Cause Read register. */
	if ((cause = er32(ICR)))
	{   
		if (cause & E1000_ICR_LSC)
			e1000_link_changed(adaptor);

		if (cause & (E1000_ICR_RXO | E1000_ICR_RXT0))
			e1000_rx(adaptor);

		if ((cause & E1000_ICR_TXQE) ||
				(cause & E1000_ICR_TXDW))
			e1000_release_tx_buffer(adaptor);
	}
	LOG_FUNCTION_NAME_EXIT(0);
	return 0;
}

static int 
e1000_net_start(struct net_driver *self)
{
	struct e1000_adaptor *adaptor;
	struct e1000_hw *hw;

	LOG_FUNCTION_NAME_ENTRY();
	adaptor = netdrv_private(self);
	hw = &adaptor->hw;
	uint32_t rctl, tctl, ims;

	e1000_fill_rx_buffer(adaptor);

	/* start RX & TX engine */
	rctl = er32(RCTL);
	/* TODO: should depend on requested buffer size */
	rctl |= E1000_RCTL_EN | E1000_RCTL_BSEX | E1000_RCTL_SZ_4096;
	ew32(RCTL, rctl);
	tctl = er32(TCTL);
	tctl |= E1000_TCTL_EN | E1000_TCTL_PSP;
	ew32(TCTL, tctl);

	/* enable interrupt */
	ims = E1000_ICR_LSC | E1000_ICR_RXO | E1000_ICR_RXT0 | 
	      E1000_ICR_TXQE | E1000_ICR_TXDW;
	ew32(IMS, ims);

	LOG_FUNCTION_NAME_EXIT(0);
	return 0;
}

static int
e1000_net_stop(struct net_driver *self)
{
	LOG_FUNCTION_NAME_ENTRY();
	LOG_FUNCTION_NAME_EXIT_NORET();
	return 0;
}

static int
e1000_alloc_iodesc(struct e1000_adaptor *adaptor)
{
	struct e1000_hw *hw = &adaptor->hw;
	size_t desc_size, buf_size;

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

	/*
	 * Setup the transmit ring registers.
	 */
	ew32(TDBAL, kvtop(adaptor->tx_desc));
	ew32(TDBAH, 0);
	ew32(TDLEN, desc_size);
	ew32(TDH, 0); /* tx descriptor head */
	ew32(TDT, 0); /* tx descriptor tail */
	adaptor->tx_ptr = 0;

	/* allocate rx descriptors */
	desc_size = sizeof(struct e1000_rx_desc) *
		adaptor->num_rx_queues;
	buf_size = sizeof(dbuf_t) * adaptor->num_rx_queues;
	adaptor->rx_desc = kmem_alloc(desc_size);
	memset(adaptor->rx_desc, 0, desc_size);
	adaptor->rx_bufs = kmem_alloc(buf_size);
	memset(adaptor->rx_bufs, 0 ,buf_size);

	/*
	 * Setup the receive ring registers.
	 */
	ew32(RDBAL, kvtop(adaptor->rx_desc));
	ew32(RDBAH, 0);
	ew32(RDLEN, desc_size);
	ew32(RDH, 0); /* rx descriptor head */
	ew32(RDT, 0); /* rx descriptor tail */
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
e1000_transmit(struct net_driver *self, dbuf_t buf)
{
	struct e1000_adaptor *adaptor;
	struct e1000_hw *hw;
	adaptor = netdrv_private(self);
	hw = &adaptor->hw;
	uint32_t head, tail;
	struct e1000_tx_desc *desc;

	head = er32(TDH);
	tail = er32(TDT);

	DPRINTF(TX, "%s(): head=%d, tail=%d\n",
		__func__, head, tail);

	if (tail == head)
		return ENOMEM;

	desc = &adaptor->tx_desc[tail];

	/* Mark this descriptor ready. */
	desc->lower.data =
		cpu_to_le32(E1000_TXD_CMD_IFCS | E1000_TXD_CMD_RS);
	desc->lower.flags.length =
		cpu_to_le16(dbuf_get_data_length(buf));
	desc->upper.data = 0; /* status */

	/* Mark end-of-packet if this is last descriptor */
	if (tail == adaptor->num_tx_queues - 1)
		desc->lower.data |= cpu_to_le32(E1000_TXD_CMD_EOP);

	/* Advance the counter */
	tail = (tail + 1) % adaptor->num_tx_queues;
	ew32(TDT, tail);

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
	netdrv_attach(&e1000_netdrv_ops, &e1000_driver, NETIF_ETHERNET);

	return 0;
}
