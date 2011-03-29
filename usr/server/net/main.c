
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

struct net_priv {
	device_t *devs;
	int num_if;
};

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

int
main(int argc, char *argv[])
{
	device_t netc;
	struct net_priv *priv;
	int num_if, i;

	sys_log("Starting net server\n");

	/* Boost thread priority. */
	thread_setpri(thread_self(), PRI_NET);

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
	priv = malloc(sizeof(struct net_priv));
	priv->num_if = num_if;
	priv->devs = malloc(sizeof(device_t) * num_if);

	DPRINTF(("net: %d interface probed:", num_if));
	for (i = 0; i < num_if; i++) {
		char name[10];
		struct net_if_caps caps;
		sprintf(name, "net%d", i);
		device_open(name, 0, &priv->devs[i]);
		device_ioctl(priv->devs[i], NETIO_GET_IF_CAPS, &caps);
		DPRINTF(("  %s, mtu %d\n", 
			interface_name(caps.type), caps.mtu));
	}

	for (;;);
	return 0;
}
