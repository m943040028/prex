#ifndef _DATAGRAM_Q_H
#define _DATAGRAM_Q_H_

#include <net.h>

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

struct datagram_buffer {
#define DATAGRAM_HDR_MAGIC	0x9a0a
	uint16_t magic;
	uint16_t index;
	dq_state_t state;
	off_t data_offset; /* offset of the data buffer */
	size_t data_size;  /* data size */
};

struct datagram_queue 
{
	uint8_t nr_bufs		/* number of allocated datagram_buffer */
	uint8_t buf_pages;	/* memory pages per datagram_buffer */
	uint8_t buf_align;	/* alignment in buffer for performance */
	datagram_buffer_t *bufs;/* list of datagram_buffer */
	queue_type_t type;	/* type of the queue: RX or TX */
};


__BEGIN_DECLS
int	dq_request_buf(struct datagram_queue *);
int	dq_release_buf(struct datagram_queue *);
int	dq_queue_buf(struct datagram_queue *, datagram_buffer_t);
int	dq_dequeue_buf(struct datagram_queue *, datagram_buffer_t *);
__END_DECLS

#endif
