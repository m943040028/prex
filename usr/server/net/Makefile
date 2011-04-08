PROG:=		net
SRCS:=		main.c \
		pif.c \
		arch/sys_arch.c \
		lwip/src/api/api_lib.c \
		lwip/src/api/api_msg.c \
		lwip/src/api/err.c \
		lwip/src/api/netbuf.c \
		lwip/src/api/netdb.c \
		lwip/src/api/netifapi.c \
		lwip/src/api/sockets.c \
		lwip/src/api/tcpip.c \
		lwip/src/core/init.c \
		lwip/src/core/tcp_in.c \
		lwip/src/core/dhcp.c \
		lwip/src/core/mem.c \
		lwip/src/core/memp.c \
		lwip/src/core/netif.c \
		lwip/src/core/pbuf.c \
		lwip/src/core/raw.c \
		lwip/src/core/stats.c \
		lwip/src/core/sys.c \
		lwip/src/core/tcp.c \
		lwip/src/core/ipv4/ip_addr.c \
		lwip/src/core/ipv4/icmp.c \
		lwip/src/core/ipv4/ip.c \
		lwip/src/core/ipv4/ip_frag.c \
		lwip/src/core/ipv4/inet_chksum.c \
		lwip/src/core/ipv4/inet.c \
		lwip/src/core/tcp_out.c \
		lwip/src/core/udp.c \
		lwip/src/netif/etharp.c \
		lwip/src/netif/loopif.c \

#DISASM= 	net.lst
#SYMBOL= 	net.sym

include $(SRCDIR)/mk/prog.mk

# Extra cflags
CFLAGS+=	-Ilwip/src/include \
		-Ilwip/src/include/ipv4

