#include <sys/net.h>
#include <sys/list.h>
#include "config.h"

struct netif;
struct queue;

struct pif {
	char		name[10];
	device_t	dev;
	netif_type_t 	type;
	struct netif 	nif;
	uint16_t 	mtu;
	dbuf_t		tx_buf[DEF_NR_TXBUF];
	struct list	tx_free_list;
	int		free_txbufs;
	dbuf_t		rx_buf[DEF_NR_RXBUF];
	struct list	rx_free_list;
	int		free_rxbufs;
};

#define to_pif(netif) \
	container_of(netif, struct netif, nif)

err_t pif_init(struct netif *netif);
