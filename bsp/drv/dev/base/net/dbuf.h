#ifndef _DBUF_H_
#define _DBUF_H_
struct net_driver;

#include <sys/queue.h>
/*
 * For efficiency, all datagram reported from data-link layer is
 * memory mapped.
 *
 * From the user point of view, following steps should be taken
 * to transfer and receive frames:
 *
 * - open /dev/netc, the net coordinator.
 * - query the network interface(s) via devctl(NC_QUERY_NETIF).
 * - open the network interface(s), e.g. /dev/netX.
 * - request buffer for datagram_queue via ioctl(DQ_REQUEST_BUF).
 *   (you need to allocate two datagram_queues, one for tx, and the
 *   other for rx)
 * - initial each allocated buffer.
 * - for transmit, full data in buffer(s), then queue the buffer
 *   via ioctl(DQ_QUEUE_BUF).
 * - after transmission, net coordinator will change buffer state to
 *   free, then it can be recycled via ioctl(DQ_DEQUEUE_BUF).
 * - for receive, the operation is reversed.
 */

typedef enum dbuf_state {
	DB_FREE         = 0x0001,
	DB_READY        = 0x0002,
} dbuf_state_t;

__BEGIN_DECLS
int netdrv_buf_pool_init(struct net_driver *);
int netdrv_q_rxbuf(struct net_driver *, dbuf_t);
int netdrv_dq_rxbuf(struct net_driver *, dbuf_t *);
int netdrv_dq_txbuf(struct net_driver *, dbuf_t *);
__END_DECLS

#endif /* _DBUF_H_ */
