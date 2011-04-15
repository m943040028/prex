
/*
 * Network server - interface to lwIP stack
 */

#define DEBUG_NET 1

#ifdef DEBUG_NET
#define DPRINTF(a) dprintf a
#else
#define DPRINTF(a)
#endif

#include <sys/prex.h>
#include <sys/capability.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <ipc/proc.h>
#include <ipc/ipc.h>
#include <ipc/exec.h>
#include <sys/list.h>

#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* from lwip stack */
#include <lwip/netif.h>
#include <lwip/tcpip.h>
#include <lwip/stats.h>
#include <lwip/netbuf.h>
#include <netif/etharp.h>

#include "pif.h"

struct net_state {
	int num_if;
	struct pif *pif;
};

static struct net_state ns;

static char
interface_type[][20] =
{
	"Ethernet",
};

static const char *
interface_name(netif_type_t type)
{
	return interface_type[type];
}

/*
 * Wait until specified server starts.
 */
static void
wait_server(const char *name, object_t *pobj)
{
	int i, error = 0;

	/* Give chance to run other servers. */
	thread_yield();

	/*
	 * Wait for server loading. timeout is 1 sec.
	 */
	for (i = 0; i < 100; i++) {
		error = object_lookup((char *)name, pobj);
		if (error == 0)
			break;

		/* Wait 10msec */
		timer_sleep(10, 0);
		thread_yield();
	}
	if (error)
		sys_panic("net: server not found");
}

static void
allocate_dbuf(struct pif *pif)
{
	int i;
	dbuf_t dbuf;
	list_init(&pif->tx_free_list);
	list_init(&pif->rx_free_list);

	for (i = 0; i < DEF_NR_TXBUF; i++)
	{
		/* allocate TX buffers */
		vm_allocate(task_self(),
			    (void *)&pif->tx_buf[i], PAGE_SIZE, 1);
		dbuf = pif->tx_buf[i];
		dbuf->magic = DATAGRAM_HDR_MAGIC;
		list_init(&dbuf->link);
		list_insert(&pif->tx_free_list, &dbuf->link);
	}
	pif->free_txbufs = DEF_NR_TXBUF;

	for (i = 0; i < DEF_NR_RXBUF; i++)
	{
		/* allocate RX buffers */
		vm_allocate(task_self(),
			    (void *)&pif->rx_buf[i], PAGE_SIZE, 1);
		dbuf = pif->rx_buf[i];
		dbuf->magic = DATAGRAM_HDR_MAGIC;
		list_init(&dbuf->link);
		list_insert(&pif->tx_free_list, &dbuf->link);
	}
	pif->free_rxbufs = DEF_NR_RXBUF;
}

int
main(int argc, char *argv[])
{
	device_t netc;
	struct bind_msg bm;
	object_t execobj;
	int num_if, i;

	sys_log("Starting net server\n");

	/* Boost thread priority. */
	thread_setpri(thread_self(), PRI_NET);

	/*
	 * Wait until all required system servers
	 * become available.
	 */
	wait_server("!exec", &execobj);

	/*
	 * Request to bind a new capabilities for us.
	 */
	bm.hdr.code = EXEC_BINDCAP;
	strlcpy(bm.path, "/boot/net", sizeof(bm.path));
	msg_send(execobj, &bm, sizeof(bm));

	/*
	 * Connect to the network coordinator
	 */
	if (device_open("netc", 0, &netc) != 0) {
		/*
		 * Bad config...
		 */
		sys_panic("net: cannot open net coordinator");
	}

	device_ioctl(netc, NETIO_QUERY_NR_IF, &num_if);
	ns.num_if = num_if;
	ns.pif = malloc(sizeof(struct netif *) * num_if);

	DPRINTF(("net: %d interface probed:\n", num_if));
	for (i = 0; i < num_if; i++) {
		char name[10];
		struct net_if_caps caps;
		struct pif *pif = &ns.pif[i];
		struct ip_addr ipaddr, netmask, gateway;
		ipaddr.addr  = inet_addr("10.0.2.15");
		netmask.addr = inet_addr("255.255.255.0");
		gateway.addr = inet_addr("10.0.2.2");	
DPRINTF(("pif=%08x\n", (uint32_t)pif));
DPRINTF(("pif->dev=%08x\n", (uint32_t)&pif->dev));

		sprintf(name, "net%d", i);
		device_open(name, 0, &pif->dev);
		device_ioctl(pif->dev, NETIO_GET_IF_CAPS, &caps);
		DPRINTF(("  %s, mtu %d\n", 
			interface_name(caps.type), caps.mtu));
		strncpy(pif->name, name, sizeof(name));
		pif->mtu = caps.mtu;
		pif->type = caps.type;
		allocate_dbuf(pif);

		if (0 == netif_add(&pif->nif, &ipaddr, &netmask, &gateway,
				   pif, pif_init, ip_input))
			panic("lwip_init: error in netif_add\n");
	}
	for (;;);

	return 0;
}
