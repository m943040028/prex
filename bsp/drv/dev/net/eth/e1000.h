#ifdef DEBUG_E1000
enum {
	DBG_INFO, DBG_TRACE, DBG_TX, DBG_RX
};
#define DBGBIT(x)       (1<<DBG_##x)
extern int debugflags;

#define DPRINTF(what, fmt, ...) do { \
	if (debugflags & DBGBIT(what)) \
		printf("e1000: " fmt, ## __VA_ARGS__); \
	} while (0)
#else
#define DPRINTF(what, fmt, ...) do {} while (0)                                                                                                                                                                                               
#endif


#define LOG_FUNCTION_NAME_ENTRY() \
DPRINTF(TRACE, "Calling %s()\n",  __func__);
#define LOG_FUNCTION_NAME_EXIT_NORET() \
DPRINTF(TRACE, "Leaving %s()\n",  __func__);
#define LOG_FUNCTION_NAME_EXIT(r) \
DPRINTF(TRACE, "Leaving %s(), ret=%d\n",  __func__, r);

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
