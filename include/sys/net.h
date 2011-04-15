#ifndef _SYS_NET_H_
#define _SYS_NET_H_
#include <sys/list.h>

typedef enum netif_type {
	NETIF_ETHERNET  = 0x0001,
} netif_type_t;

#ifndef KERNEL
struct dbuf_user {
#define DATAGRAM_HDR_MAGIC	0x9a0a
	uint16_t	magic;
	void		*data_start; /* address of the data buffer */
	size_t          data_length; /* actual data length */
	struct list	link;
};

typedef struct dbuf_user *dbuf_t;
#endif

#endif /* _SYS_NET_H_ */
