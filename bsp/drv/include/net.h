#ifndef _NET_H_
#define _NET_H_

#include <sys/cdefs.h>
#include <sys/net.h>
#include <types.h>

struct net_driver;

struct netdrv_ops 
{
	int	(*init)(struct net_driver *);
	int	(*start)(struct net_driver *);
	int	(*stop)(struct net_driver *);
	int	(*transmit)(struct net_driver *, dbuf_t);
};

__BEGIN_DECLS
int	 netdrv_attach(struct netdrv_ops *, struct driver *,
		       netif_type_t);
void*	 netdrv_private(struct net_driver *);

int      dbuf_release(struct net_driver *, dbuf_t buf);
int      dbuf_request(struct net_driver *, dbuf_t *buf);
int      dbuf_add(struct net_driver *, dbuf_t buf);
size_t   dbuf_get_size(dbuf_t buf);
size_t   dbuf_get_data_length(dbuf_t buf);
paddr_t  dbuf_get_paddr(dbuf_t buf);
void     dbuf_set_data_length(dbuf_t buf, uint16_t);

__END_DECLS

#endif /* _NET_H_ */
