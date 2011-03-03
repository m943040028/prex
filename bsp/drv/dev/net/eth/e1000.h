#ifdef DEBUG_E1000
enum {
	DBG_INFO,
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

#define er32(reg) \
	(bus_read_32(hw->io_base + E1000_##reg))

#define ew32(reg, value) \
	(bus_write_32(hw->io_base + E1000_##reg, (value)))
