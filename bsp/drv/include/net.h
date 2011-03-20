#ifndef _NET_H_
#define _NET_H_

#include <driver.h>
#include <sys/list.h>

struct net_softc;
struct datagram_buffer;
typedef struct datagram_buffer *dbuf_t;

typedef enum netif {
	NETIF_ETHERNET	= 0x0001,
} netif_t;

struct net_driver {
	struct driver *driver;
	struct net_driver_operations *ops;
	netif_t interface;
	struct list link;
	int id;
	struct net_softc *nc;
};

struct net_driver_operations 
{
	int	(*init)(struct net_driver *);
	int	(*start)(struct net_driver *);
	int	(*stop)(struct net_driver *);
	int	(*configure)(struct net_driver *);
};

__BEGIN_DECLS
/* net.c */
int	net_driver_attach(struct net_driver *driver, netif_t);
void*	net_driver_private(struct net_driver *driver);

/* dq.c */
/* release a tranfered buffer to datagram queue */
int	dq_buf_release(dbuf_t);
/* request an empty buffer for receving from datagram queue */
int	dq_buf_request(dbuf_t *);
/* add a recevied buffer to datagram queue */
int	dq_buf_add(dbuf_t);
__END_DECLS

#endif /* _NET_H_ */
