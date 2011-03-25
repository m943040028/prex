#ifndef _NETDRV_H_
#define _NETDRV_H_

#include <driver.h>
#include <net.h>
#include <sys/queue.h>

struct net_driver {
	struct driver           *driver;
	struct netdrv_ops       *ops;
	netif_type_t            interface;
	int                     id;
	struct list             link;
	struct net_softc        *nc;

	/* per-device dbuf pool */
	struct queue    free_list;
	struct queue    rx_queue;
	struct queue    tx_queue;
};

#endif
