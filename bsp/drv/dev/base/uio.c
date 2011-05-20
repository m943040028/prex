/*-
 * Copyright (c) 2011 Sheng-Yu Chiu
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

/* uio.c - the userspace i/o driver */

/*#define DBG */
#define MODULE_NAME		"uio"

#include <driver.h>
#include <sys/list.h>
#include <sys/queue.h>
#include <sys/ioctl.h>
#include <sys/dbg.h>
#include <sys/signal.h>

#ifdef DBG
static int debugflags = DBGBIT(INFO) | DBGBIT(TRACE);
#endif

#define MAX_NET_DEVS		10

static int uio_open(device_t, int);
static int uio_close(device_t);
static int uio_ioctl(device_t, u_long, void *);
static int uio_devctl(device_t, u_long, void *);
static int uio_init(struct driver *);

struct uio_irq {
	struct list		link;
	task_t			task;
	irq_t			irq;
	int			nr;
	int			ipl;
};

struct uio_user {
	struct list		link;
	task_t			task;
	struct list		irqs;
	int			nr_irqs;
};

struct uio_softc {
	struct list		owners;
	int			nr_owners;
};

static struct devops uio_devops = {
	/* open */	uio_open,
	/* close */	uio_close,
	/* read */	no_read,
	/* write */	no_write,
	/* ioctl */	uio_ioctl,
	/* devctl */	uio_devctl,
};

struct driver uio_driver = {
	/* name */	"uio",
	/* devsops */	&uio_devops,
	/* devsz */	sizeof(struct uio_softc),
	/* flags */	0,
	/* probe */	NULL,
	/* init */	uio_init,
	/* shutdown */	NULL,
};

static struct uio_user *
uio_find_task(struct uio_softc *uc, task_t task)
{
	list_t n, p;
	p = list_first(&uc->owners);
	for (n = p; n == p; n = list_next(n))
	{
		struct uio_user *user = list_entry(n, struct uio_user, link);
		if (task == user->task)
			return user;
	}
	return NULL;
}

static int
uio_add_task(struct uio_softc *uc, task_t task)
{
	struct uio_user *user = kmem_alloc(sizeof(struct uio_user));
	if (!user)
		return ENOMEM;
	list_init(&user->link);
	list_init(&user->irqs);
	user->task = task;
	user->nr_irqs = 0;
	list_insert(&uc->owners, &user->link);
	return 0;
}

static int
uio_add_irq(struct uio_user *user, struct irq_req *req,
	    int (*handler)(void *))
{
	struct uio_irq *irq = kmem_alloc(sizeof(struct uio_irq));
	if (!irq)
		return ENOMEM;
	irq->nr = req->nr;
	irq->ipl = req->ipl;
	list_init(&irq->link);
	irq->task = user->task;
	irq->irq = irq_attach(irq->nr, irq->ipl, 0,
			      handler, IST_NONE, irq);
	if (!irq->irq)
		goto err_free_uio_irq;

	DPRINTF(INFO, "irq %d attached with IPL %d\n",
		irq->nr, irq->ipl);
	user->nr_irqs ++;
	list_insert(&user->irqs, &irq->link);
	return 0;

err_free_uio_irq:
	kmem_free(irq);
	return EBUSY;
}

static int
generic_irq_handler(void *args)
{
	struct uio_irq *irq = args;
	DPRINTF(DBG, "irq %d occurs\n", irq->nr);
	exception_post(irq->task, SIGIO);
	DPRINTF(DBG, "post signal to task %d\n", irq->task);
	return 0;
}

static int
uio_init(struct driver *self)
{
	struct uio_softc *uc;
	device_t dev;

	dev = device_create(self, "uio", D_CHR);
	if (!dev) {
		return ENODEV;
	}

	uc = device_private(dev);
	uc->nr_owners = 0;
	list_init(&uc->owners);

	return 0;
}

static int uio_open(device_t dev, int mode)
{
	if (!task_capable(CAP_USERIO))
		return EPERM;

	return 0;
}

static int uio_close(device_t dev)
{
	return 0;
}

static int uio_ioctl(device_t dev, u_long cmd, void *args)
{
	struct uio_softc *uc = device_private(dev);
	task_t request_task;
	struct irq_req irq_req;
	struct uio_user *uio_user;
	int err;

	LOG_FUNCTION_NAME_ENTRY();
	switch (cmd) {

	case UIO_CONNECT:
		/* Connection request from the user */
		if (copyin(args, &request_task, sizeof(task_t)))
			return EFAULT;
		if (uio_find_task(uc, request_task))
			return EBUSY;
		err = uio_add_task(uc, request_task);
		if (err)
			return err;
		DPRINTF(INFO, "connectted with %d\n", request_task);
		break;
	case UIO_REQUEST_IRQ:
		if (copyin(args, &irq_req, sizeof(struct irq_req)))
			return EFAULT;
		uio_user = uio_find_task(uc, irq_req.task);
		if (!uio_user)
			return EINVAL;
		err = uio_add_irq(uio_user, &irq_req, generic_irq_handler);
		if (err)
			return err;
		break;
	default:
		return EINVAL;
	};
	LOG_FUNCTION_NAME_EXIT(0);
	return 0;
}

static int uio_devctl(device_t dev, u_long cmd, void *args)
{
	return 0;
}
