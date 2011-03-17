#ifndef _NET_H_
#define _NET_H_

#include <driver.h>
#include <sys/list.h>

struct net_softc;
struct datagram_buffer;
struct datagram_queue;
typedef struct datagram_buffer *datagram_buffer_t;

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

typedef enum dqbuf_state {
	DB_INITIALIZED	= 0x0001,
	DB_FREE		= 0x0002,
	DB_MAPPED	= 0x0004,
	DB_READY	= 0x0008,
} dqbuf_state_t;

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
int	dqbuf_set_state(datagram_buffer_t, dqbuf_state_t);
dqbuf_state_t dqbuf_get_state(datagram_buffer_t);
paddr_t dqbuf_get_paddr(datagram_buffer_t);
size_t  dqbuf_get_data_size(datagram_buffer_t);
size_t  dqbuf_get_buf_size(datagram_buffer_t);
__END_DECLS

#endif /* _NET_H_ */
