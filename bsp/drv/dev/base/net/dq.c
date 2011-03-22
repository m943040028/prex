#include <driver.h>
#include <sys/list.h>
#include <sys/ioctl.h>
#include "dq.h"

struct datagram_buffer *
dq_alloc_buf(int nr_pages)
{
	paddr_t p;
	struct datagram_buffer *buf;
	if ((p = page_alloc(nr_pages * PAGE_SIZE)) == 0)
		return NULL;

	buf = ptokv(p);
	memset(buf, 0, nr_pages * PAGE_SIZE);
	buf->magic = DATAGRAM_HDR_MAGIC;
	buf->state = DB_INITIALIZED;
	buf->buf_pages = nr_pages;
	queue_init(&buf->link);

	return 0;
}

int
dq_release_buf(struct datagram_buffer *buf)
{
	ASSERT((buf->magic == DATAGRAM_HDR_MAGIC));

	page_free(kvtop(buf), buf->buf_pages);
	return 0;
}

/* DDI interface, used by network device drivers */

/* release a tranfered buffer to datagram queue */
int
dq_buf_release(dbuf_t buf)
{
}

/* request an empty buffer for receving from datagram queue */
int
dq_buf_request(dbuf_t *buf)
{
}

/* add a recevied buffer to datagram queue */
int
dq_buf_add(dbuf_t buf);
{
}

paddr_t
dq_buf_get_paddr(dbuf_t buf)
{
	struct datagram_buffer *dbuf =
		(struct datagram_buffer *)buf
	return dbuf->data_start;
}

size_t
dq_buf_get_size(dbuf_t buf)
{
	struct datagram_buffer *dbuf =
		(struct datagram_buffer *)buf
	return dbuf->buf_pages * PAGE_SIZE;
}
