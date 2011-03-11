#ifndef _NET_H_
#define _NET_H_

#include <driver.h>
#include <sys/list.h>

struct net_softc;

struct net_driver {
	struct driver *driver;
	struct net_driver_operations *ops;
	struct list link;
	int id;
	struct net_softc *nc;
};

struct packet {
	void *bufptr;
	struct list link;
};

struct net_driver_operations {
	int	(*init)(struct net_driver *);
	int	(*enqueue)(struct net_driver *, struct packet *);
	int	(*dequeue)(struct net_driver *, struct packet *);
};


__BEGIN_DECLS
int	net_driver_attach(struct net_driver *driver);
void*	net_driver_private(struct net_driver *driver);
__END_DECLS


#endif /* _NET_H_ */
