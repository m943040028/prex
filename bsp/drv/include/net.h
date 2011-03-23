#ifndef _NET_H_
#define _NET_H_

#include <sys/cdefs.h>
#include <types.h>

struct net_driver;

typedef enum netif_type {
	NETIF_ETHERNET	= 0x0001,
} netif_type_t;

struct netdrv_ops 
{
	int	(*init)(struct net_driver *);
	int	(*start)(struct net_driver *);
	int	(*stop)(struct net_driver *);
	int	(*transmit)(struct net_driver *, dbuf_t);
};

__BEGIN_DECLS
int	netdrv_attach(struct netdrv_ops *, struct driver *, netif_type_t);
void*	netdrv_private(struct net_driver *);
__END_DECLS

#endif /* _NET_H_ */
