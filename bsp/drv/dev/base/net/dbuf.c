/*-
 * Copyright (c) 2011, Sheng-Yu Chiu
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* dbuf.c - datagram buffer manager */

/*#define DEBUG                   1*/
#define MODULE_NAME             "dbuf"

#include <driver.h>
#include <sys/list.h>
#include <sys/ioctl.h>
#include <sys/queue.h>
#include <sys/dbg.h>

#include "dbuf.h"
#include "netdrv.h"

#ifdef DEBUG
static int debugflags = DBGBIT(INFO) | DBGBIT(TRACE);
#endif

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
	LOG_FUNCTION_NAME_ENTRY();
	struct dbuf *dbuf = (struct dbuf *)buf;
	ASSERT(dbuf->magic == DATAGRAM_HDR_MAGIC);
	dbuf->state = DB_FREE;

	enqueue(&driver->pool.free_list, &dbuf->link);
	LOG_FUNCTION_NAME_EXIT(0);
	return 0;
}

static int
remove_free_buf(struct net_driver *driver, dbuf_t *buf)
{
	struct dbuf *dbuf;
	queue_t q;

	LOG_FUNCTION_NAME_ENTRY();
	q = dequeue(&driver->pool.free_list);
	if (!q) {
		LOG_FUNCTION_NAME_EXIT(ENOMEM);
		return ENOMEM;
	}
	dbuf = queue_entry(q, struct dbuf, link);

	*buf = (dbuf_t)dbuf;
	LOG_FUNCTION_NAME_EXIT(0);
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

	LOG_FUNCTION_NAME_ENTRY();
	ASSERT(buf != NULL);
	n = dequeue(&driver->pool.rx_queue);
	if (!n)
		return ENOENT;
	dbuf = queue_entry(n, struct dbuf, link);
	*buf = (dbuf_t)dbuf;
	LOG_FUNCTION_NAME_EXIT(0);
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
