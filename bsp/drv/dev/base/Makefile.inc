
SRCS+=			dev/base/main.c \
			dev/base/init.c \
			dev/base/tty.c \
			dev/base/uio.c

SRCS-$(CONFIG_PM)+=	dev/base/pm.c
SRCS-$(CONFIG_CONS)+=	dev/base/cons.c
SRCS-$(CONFIG_WSCONS)+=	dev/base/wscons.c

SRCS-$(CONFIG_NET)+=	dev/base/net/net.c \
			dev/base/net/dbuf.c

ifeq ($(DEBUG),1)
SRCS-$(CONFIG_KD)+=	dev/base/kd.c
endif
