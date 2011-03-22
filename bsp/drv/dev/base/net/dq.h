#ifndef _DATAGRAM_Q_H
#define _DATAGRAM_Q_H_
#include <sys/ioctl.h>
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

typedef enum dqbuf_state {
	DB_INITIALIZED  = 0x0001,
	DB_FREE         = 0x0002,
	DB_MAPPED       = 0x0004,
	DB_READY        = 0x0008,
} dqbuf_state_t;

struct datagram_buffer {
#define DATAGRAM_HDR_MAGIC	0x9a0a
	uint16_t magic;
	uint16_t index;
	dqbuf_state_t state;
	uint8_t buf_pages;/* memory pages per datagram_buffer */
	uint8_t buf_align;/* alignment in buffer for performance */
	void *data_start; /* address of the data buffer */
	size_t data_size; /* data size */
	queue_t link;
};

struct datagram_queue 
{
	uint8_t nr_bufs;	/* number of allocated datagram_buffer */
	dbuf_t *bufs;		/*  buffer array */
	queue_t tx_queue;	/* list of buffers ready to transmit */
	queue_t rx_queue;	/* list of buffers consisted recevied*/
	queue_t free_list;	/* list of buffers that is free to use */
};

__BEGIN_DECLS
struct datagram_buffer *dq_alloc_buf(int nr_pages);
int	dq_release_buf(struct datagram_queue *);
__END_DECLS

#endif
