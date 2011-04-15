#include <driver.h>
#include <sys/list.h>
#include <sys/ioctl.h>
#include <sys/queue.h>

#include "dbuf.h"
#include "netdrv.h"

struct dbuf {
#define DATAGRAM_HDR_MAGIC      0x9a0a
	uint16_t        magic;
	void            *data_start; /* address of the data buffer */
	size_t          data_length; /* actual data length */
	struct queue    link;
	dbuf_state_t    state;
	uint8_t         buf_pages;/* memory pages per datagram_buffer */
	uint8_t         buf_align;/* alignment in buffer for performance */
};

static int
add_free_buf(struct net_driver *driver, dbuf_t buf)
{
	struct dbuf *dbuf = (struct dbuf *)buf;
	ASSERT(dbuf->magic == DATAGRAM_HDR_MAGIC);
	dbuf->state = DB_FREE;

	enqueue(&driver->pool.free_list, &dbuf->link);
	return 0;
}

static int
remove_free_buf(struct net_driver *driver, dbuf_t *buf)
{
	struct dbuf *dbuf;
	queue_t q;

	q = dequeue(&driver->pool.free_list);
	if (!q)
		return ENOMEM;
	dbuf = queue_entry(q, struct dbuf, link);

	*buf = (dbuf_t)dbuf;
	return 0;
}

/* Interface to network coordinator */
int
netdrv_buf_pool_init(struct net_driver *driver)
{
	queue_init(&driver->pool.free_list);
	queue_init(&driver->pool.tx_queue);
	queue_init(&driver->pool.rx_queue);

	return 0;
}

/* remove a received buffer from dbuf pool */
int
netdrv_dq_rxbuf(struct net_driver *driver, dbuf_t *buf)
{
	struct dbuf *dbuf;
	queue_t n;

	ASSERT(buf != NULL);
	n = dequeue(&driver->pool.rx_queue);
	if (!n)
		return ENOENT;
	dbuf = queue_entry(n, struct dbuf, link);
	*buf = (dbuf_t)dbuf;
	return 0;
}

/* add a buffer to free_list to be requested by device driver */
int
netdrv_q_rxbuf(struct net_driver *driver, dbuf_t buf)
{
	return add_free_buf(driver, buf);
}

/* remove a buffer from free_list for transmit */
int
netdrv_dq_txbuf(struct net_driver *driver, dbuf_t *buf)
{
	return remove_free_buf(driver, buf);
}
/* end of exported API */

/* DDI interface, used by network device drivers */

/* release a tranfered buffer to dbuf pool */
int
dbuf_release(struct net_driver *driver, dbuf_t buf)
{
	return add_free_buf(driver, buf);
}

/* request an empty buffer for receving from dbuf pool */
int
dbuf_request(struct net_driver *driver, dbuf_t *buf)
{
	return remove_free_buf(driver, buf);
}

/* add a received buffer to dbuf pool */
int
dbuf_add(struct net_driver *driver, dbuf_t buf)
{
	struct dbuf *dbuf = (struct dbuf *)buf;
	ASSERT(dbuf->magic == DATAGRAM_HDR_MAGIC);
	dbuf->state = DB_READY;

	enqueue(&driver->pool.rx_queue, &dbuf->link);
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
/* end of DDI interface */
