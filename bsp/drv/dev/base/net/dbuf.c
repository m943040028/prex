#include <driver.h>
#include <sys/list.h>
#include <sys/ioctl.h>
#include <sys/queue.h>

#include "dbuf.h"

struct dbuf_pool {
	struct queue	free_list;
	struct queue	rx_queue;
	struct queue	tx_queue;
};
static struct dbuf_pool dbuf_pool;

static struct dbuf *
dbuf_alloc(int nr_pages)
{
	paddr_t p;
	struct dbuf *buf;
	if ((p = page_alloc(nr_pages*PAGE_SIZE)) == 0)
		return NULL;

	buf = ptokv(p);
	memset(buf, 0, nr_pages*PAGE_SIZE);
	buf->magic = DATAGRAM_HDR_MAGIC;
	buf->state = DB_FREE;
	buf->buf_pages = nr_pages;
	/* FIXME */
	buf->data_start = buf + 1024;
	queue_init(&buf->link);

	return buf;
}

static int
dbuf_pool_init(struct dbuf_pool *pool, int num_dbufs)
{
	int i;

	queue_init(&pool->free_list);
	queue_init(&pool->tx_queue);
	queue_init(&pool->rx_queue);

	for (i = 0; i < num_dbufs; i++)
	{
		struct dbuf *buf;
		buf = dbuf_alloc(1);
		if (!buf)
			return ENOMEM;
		enqueue(&pool->free_list, &buf->link);
	}
	return 0;
}

int
dbuf_init(int num_dbufs)
{
	if (dbuf_pool_init(&dbuf_pool, num_dbufs) != 0)
		return ENOMEM;
	return 0;
}

/* DDI interface, used by network device drivers */

/* release a tranfered buffer to dbuf pool */
int
dbuf_release(dbuf_t buf)
{
	struct dbuf *dbuf = (struct dbuf *)buf;
	ASSERT(dbuf->magic == DATAGRAM_HDR_MAGIC);
	dbuf->state = DB_FREE;

	enqueue(&dbuf_pool.free_list, &dbuf->link);
	return 0;
}

/* request an empty buffer for receving from dbuf pool */
int
dbuf_request(dbuf_t *buf)
{
	struct dbuf *dbuf;
	queue_t q;

	q = dequeue(&dbuf_pool.free_list);
	if (!q)
		return ENOMEM;
	dbuf = queue_entry(q, struct dbuf, link);

	*buf = (dbuf_t)dbuf;
	return 0;
}

/* add a recevied buffer to datagram queue to dbuf pool */
int
dbuf_add(dbuf_t buf)
{
	struct dbuf *dbuf = (struct dbuf *)buf;
	ASSERT(dbuf->magic == DATAGRAM_HDR_MAGIC);
	dbuf->state = DB_READY;

	enqueue(&dbuf_pool.tx_queue, &dbuf->link);
	return 0;
}

paddr_t
dbuf_get_paddr(dbuf_t buf)
{
	struct dbuf *dbuf = (struct dbuf *)buf;
	ASSERT(dbuf->magic == DATAGRAM_HDR_MAGIC);
	return kvtop(dbuf->data_start);
}

size_t
dbuf_get_size(dbuf_t buf)
{
	struct dbuf *dbuf = (struct dbuf *)buf;
	ASSERT(dbuf->magic == DATAGRAM_HDR_MAGIC);
	return dbuf->buf_pages * PAGE_SIZE;
}

size_t
dbuf_get_data_length(dbuf_t buf)
{
	struct dbuf *dbuf = (struct dbuf *)buf;
	ASSERT(dbuf->magic == DATAGRAM_HDR_MAGIC);
	return dbuf->data_length;
}

void
dbuf_set_data_length(dbuf_t buf, uint16_t length)
{
	struct dbuf *dbuf = (struct dbuf *)buf;
	ASSERT(dbuf->magic == DATAGRAM_HDR_MAGIC);
	dbuf->data_length = length;
}
