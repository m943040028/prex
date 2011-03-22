#include <driver.h>
#include <sys/list.h>
#include <net.h>

#include "dq.h"

#define MAX_NET_DEVS	4

static int net_open(device_t, int);
static int net_close(device_t);
static int net_ioctl(device_t, u_long, void *);
static int net_devctl(device_t, u_long, void *);
static int net_init(struct driver *);

static struct list net_driver_list = LIST_INIT(net_driver_list);

struct net_softc {
	device_t dev;
	device_t net_devs[MAX_NET_DEVS];
	struct datagram_queue *queue;
};

static struct devops net_devops = {
	/* open */	net_open,
	/* close */	net_close,
	/* read */	no_read,
	/* write */	no_write,
	/* ioctl */	net_ioctl,
	/* devctl */	net_devctl,
};

struct driver net_driver = {
	/* name */	"net",
	/* devsops */	&net_devops,
	/* devsz */	sizeof(struct net_softc),
	/* flags */	0,
	/* probe */	NULL,
	/* init */	net_init,
	/* shutdown */	NULL,
};

int
net_driver_attach(struct net_driver *driver)
{
	list_init(&driver->link);
	list_insert(&net_driver_list, &driver->link);
	return 0;
}

void *
net_driver_private(struct net_driver *driver)
{
	device_t dev;
	struct net_softc *nc;
	nc = driver->nc;
	dev = nc->net_devs[driver->id];
	return device_private(dev);
}

static int
net_init(struct driver *self)
{
	struct net_driver *nd;
	struct net_softc *nc;
	device_t dev;
	int id = 0;

	dev = device_create(self, "netc", D_NET|D_PROT);
	if (!dev) {
		return ENODEV;
	}

	nc = device_private(dev);
	nc->dev = dev;

	list_t  n, head = &net_driver_list;
	for (n = list_first(&net_driver_list); n != head;
	     n = list_next(n)) {
		int ret;
		char name[20];
		nd = list_entry(n, struct net_driver, link);

		sprintf(name, "net%d", id);
		nd->id = id;
		nd->nc = nc;
		nc->net_devs[id] = device_create(nd->driver,
						 name, D_NET|D_PROT);

		ret = nd->ops->init(nd);
		if (!ret) {
			device_destroy(nc->net_devs[id]);
			continue;
		}

		id++;
		if (id >= MAX_NET_DEVS)
			break;
	}
	return 0;
}

static int net_open(device_t dev, int mode)
{
	return 0;
}

static int net_close(device_t dev)
{
	return 0;
}

static int net_ioctl(device_t dev, u_long cmd, void *args)
{
	return 0;
}

static int net_devctl(device_t dev, u_long cmd, void *args)
{
	return 0;
}
