#define er32(reg) \
	(pci_bus_read32(hw->io_base + E1000_##reg))
#define ew32(reg, value) \
	(pci_bus_write32(hw->io_base + E1000_##reg, (value)))

#define er32_p(reg, offset) \
	(pci_bus_read32(hw->io_base + E1000_##reg + (offset) * 4))
#define ew32_p(reg, value, offset) \
	(pci_bus_write32(hw->io_base + E1000_##reg + (offset) * 4, (value)))

#define	E1000_NUM_TX_QUEUE	32
#define	E1000_NUM_RX_QUEUE	32
