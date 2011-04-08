
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

struct netif {
	device_t dev;
	int mtu;
	netif_type_t type;
	int nr_txbufs;
	int nr_rxbufs;
};
struct net_state {
	int num_if;
	struct netif *nif;
};

static struct net_state ns;

/* configurable parameters */
#define DEF_NR_TXBUF	32
#define DEF_NR_RXBUF	32


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
allocate_dbuf(struct netif *netif)
{
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
	ns.nif = malloc(sizeof(struct netif *) * num_if);

	DPRINTF(("net: %d interface probed:\n", num_if));
	for (i = 0; i < num_if; i++) {
		char name[10];
		struct net_if_caps caps;
		struct netif *nif = &ns.nif[i];
	
		sprintf(name, "net%d", i);
		device_open(name, 0, &nif->dev);
		device_ioctl(nif->dev, NETIO_GET_IF_CAPS, &caps);

		DPRINTF(("  %s, mtu %d\n", 
			interface_name(caps.type), caps.mtu));
		nif->mtu = caps.mtu;
		nif->type = caps.type;
		allocate_dbuf(nif);
	}

	for (;;);

	return 0;
}
