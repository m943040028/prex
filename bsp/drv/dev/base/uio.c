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

#ifdef DBG
static int debugflags = DBGBIT(INFO) | DBGBIT(TRACE);
#endif

#define MAX_NET_DEVS		10

static int uio_open(device_t, int);
static int uio_close(device_t);
static int uio_ioctl(device_t, u_long, void *);
static int uio_devctl(device_t, u_long, void *);
static int uio_init(struct driver *);

struct uio_softc {
	device_t		uio_devs[MAX_NET_DEVS];
	struct uio_driver	*uio_drvs[MAX_NET_DEVS];
	int			nrdevs;
	int			isopen;
};
static struct uio_softc *uio_softc;
static struct list uiodrv_list = LIST_INIT(uiodrv_list);

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
	uio_softc = uc;

	return 0;
}

static int uio_open(device_t dev, int mode)
{
	struct uio_softc *nc = uio_softc;

	if (!task_capable(CAP_USERIO))
		return EPERM;

	return 0;
}

static int uio_close(device_t dev)
{
	struct uio_softc *nc = uio_softc;

	if (!task_capable(CAP_USERIO))
		return EPERM;
	return 0;
}

static int uio_ioctl(device_t dev, u_long cmd, void *args)
{
	struct uio_softc *nc = uio_softc;

	LOG_FUNCTION_NAME_ENTRY();
	if (!task_capable(CAP_USERIO))
		return EPERM;
	switch (cmd) {
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
