#ifndef _NET_H_
#define _NET_H_

#include <sys/cdefs.h>
#include <types.h>

typedef enum netif {
	NETIF_ETHERNET	= 0x0001,
} netif_t;

struct net_driver {
	struct driver *driver;
	struct net_driver_operations *ops;
	netif_t interface;
	int id;
	struct list link;
	struct net_softc *nc;
};

struct net_driver_operations 
{
	int	(*init)(struct net_driver *);
	int	(*start)(struct net_driver *);
	int	(*stop)(struct net_driver *);
	int	(*transmit)(struct net_driver *, dbuf_t);
};

__BEGIN_DECLS
int	net_driver_attach(struct net_driver *driver);
void*	net_driver_private(struct net_driver *driver);
__END_DECLS

#endif /* _NET_H_ */
