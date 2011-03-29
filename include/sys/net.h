#ifndef _SYS_NET_H_
#define _SYS_NET_H_

typedef enum netif_type {
	NETIF_ETHERNET  = 0x0001,
} netif_type_t;

struct dbuf_user {
#define DATAGRAM_HDR_MAGIC	0x9a0a
	uint16_t	magic;
	uint16_t	index;
	void		*data_start; /* address of the data buffer */
	size_t          data_length; /* actual data length */
};
typedef struct dbuf_user *pbuf_t;

#endif /* _SYS_NET_H_ */
