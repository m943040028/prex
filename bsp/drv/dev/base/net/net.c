/*-
 * Copyright (c) 2011 Sheng-Yu Chiu
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* net.c - the Network Coordinator */

/*#define DBG */
#define MODULE_NAME		"netc"

#include <driver.h>
#include <sys/list.h>
#include <sys/queue.h>
#include <sys/ioctl.h>
#include <sys/dbg.h>
#include <net.h>

#include "dbuf.h"
#include "netdrv.h"

#ifdef DBG
static int debugflags = DBGBIT(INFO) | DBGBIT(TRACE);
#endif

#define MAX_NET_DEVS		10

static int net_open(device_t, int);
static int net_close(device_t);
static int net_ioctl(device_t, u_long, void *);
static int net_devctl(device_t, u_long, void *);
static int net_init(struct driver *);

struct net_softc {
	device_t		net_devs[MAX_NET_DEVS];
	struct net_driver	*net_drvs[MAX_NET_DEVS];
	int			nrdevs;
	int			isopen;
};
static struct net_softc *net_softc;
static struct list netdrv_list = LIST_INIT(netdrv_list);

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

static uint8_t
get_id_from_device(device_t dev)
{
	char devname[10];

	device_name(dev, devname);
	if (devname[3] == 'c')
		return 0xff;

	/* device name should be net[0-9] */
	return (devname[3] - '0');
}

static int
net_init(struct driver *self)
{
	struct net_driver *nd;
	struct net_softc *nc;
	device_t dev;
	int id = 0;

	dev = device_create(self, "netc", D_NET);
	if (!dev) {
		return ENODEV;
	}

	nc = device_private(dev);
	net_softc = nc;

	list_t  n, head = &netdrv_list;
	for (n = list_first(&netdrv_list); n != head;
	     n = list_next(n)) {
		int ret;
		char name[20];
		nd = list_entry(n, struct net_driver, link);

		sprintf(name, "net%d", id);
		nd->id = id;
		nd->driver->devops = &net_devops;
		nc->net_devs[id] =
			device_create(nd->driver,  name, D_NET);

		ret = nd->ops->init(nd);
		if (ret) {
			device_destroy(nc->net_devs[id]);
			continue;
		}
		nc->net_drvs[id] = nd;
		netdrv_buf_pool_init(nd);

		id++;
		if (id >= MAX_NET_DEVS)
			break;
	}
	nc->nrdevs = id;

	return 0;
}

static int net_open(device_t dev, int mode)
{
	int id = get_id_from_device(dev);
	struct net_softc *nc = net_softc;

	if (!task_capable(CAP_NETWORK))
		return EPERM;
	if (id == 0xff) {
		if (nc->isopen)
			return EBUSY;
		else {
			nc->isopen = 1;
			return 0;
		}
	} else {
		struct net_driver *drv =
			nc->net_drvs[id];
		if (drv->isopen)
			return EBUSY;
		drv->isopen = 1;
	}

	return 0;
}

static int net_close(device_t dev)
{
	int id = get_id_from_device(dev);
	struct net_softc *nc = net_softc;

	if (!task_capable(CAP_NETWORK))
		return EPERM;
	if (id == 0xff) {
		nc->isopen = 0;
	} else {
		struct net_driver *drv =
			nc->net_drvs[id];
		drv->isopen = 0;
	}

	return 0;
}

static int net_ioctl(device_t dev, u_long cmd, void *args)
{
	int id = get_id_from_device(dev);
	struct net_softc *nc = net_softc;
	struct net_driver *nd = nc->net_drvs[id];
	dbuf_t dbuf;

	LOG_FUNCTION_NAME_ENTRY();
	if (!task_capable(CAP_NETWORK))
		return EPERM;
	switch (cmd) {

	case NETIO_QUERY_NR_IF:
		if (copyout(&nc->nrdevs, args, sizeof(nc->nrdevs)))
			return EFAULT;
		break;
	case NETIO_GET_IF_CAPS:	
		break;
	case NETIO_GET_STATUS:
		break;
	case NETIO_START:
		nd->ops->start(nd);
		break;
	case NETIO_STOP:
		nd->ops->stop(nd);
		break;
	case NETIO_TX_QBUF:
		if (copyin(args, &dbuf, sizeof(dbuf_t)))
			return EFAULT;
		nd->ops->transmit(nd, dbuf);
		break;
	case NETIO_RX_QBUF:
		if (copyin(args, &dbuf, sizeof(dbuf_t)))
			return EFAULT;
		netdrv_q_rxbuf(nd, dbuf);
		break;
	case NETIO_TX_DQBUF:
		if (netdrv_dq_txbuf(nd, &dbuf))
			return ENOMEM;
		if (copyout(&dbuf, args, sizeof(dbuf_t)))
			return EFAULT;
		break;
	case NETIO_RX_DQBUF:
		if (netdrv_dq_rxbuf(nd, &dbuf))
			return ENOMEM;
		if (copyout(&dbuf, args, sizeof(dbuf_t)))
			return EFAULT;
		break;
	default:
		return EINVAL;
	};
	LOG_FUNCTION_NAME_EXIT(0);
	return 0;
}

static int net_devctl(device_t dev, u_long cmd, void *args)
{
	return 0;
}

/* exported interface */
int
netdrv_attach(struct netdrv_ops *ops, struct driver *driver,
	      netif_type_t type)
{
	struct net_driver *nd;
	nd = kmem_alloc(sizeof(*nd));
	if (!nd)
		return ENOMEM;
	
	memset(nd, 0, sizeof(*nd));
	nd->interface = type;
	nd->driver = driver;
	nd->ops = ops;
	list_init(&nd->link);
	list_insert(&netdrv_list, &nd->link);
	return 0;
}

void *
netdrv_private(struct net_driver *driver)
{
	device_t dev;
	struct net_softc *nc = net_softc;
	dev = nc->net_devs[driver->id];
	return device_private(dev);
}
