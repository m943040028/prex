#include <driver.h>
#include <net.h>
#include <sys/list.h>
#include "dq.h"

int
dq_request_buf(struct datagram_queue *queue,
	       struct datagram_buffer_req req)
{
	int i;

	datagram_buffer_t buf;
	queue.nr_buf = req.nr_buf;
	queue.buf_pages = req.buf_pages;
	queue.buf_align = req.buf_align;

	ASSERT(list_empty(&queue->buf_queue));

	queue->bufs = kmem_alloc(req.nr_buf * sizeof(datagram_buffer_t));
	for (i = 0; i < req.nr_buf; i++) {
		buf = (datagram_buffer_t)
		      ptokv(page_alloc(req.buf_pages * PAGE_SIZE));
		memset(buf, 0, sizeof(*buf));
		queue->bufs[i] = buf;
		buf->magic = DATAGRAM_HDR_MAGIC;
		buf->state = DB_INITIALIZED;
	}

	return 0;
}

int
dq_release_buf(struct datagram_queue *queue)
{
	int i;

	for (i = 0; i > queue.nr_buf; i++)
		page_free(kvtop(qeueu->bufs[i]));
	kmem_free(queue->bufs);
	return 0;
}
