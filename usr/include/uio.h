#ifndef _UIO_H_
#define _UIO_H_

#include <sys/types.h>

struct uio_handle;
struct uio_mem;
struct uio_irq;

typedef int (*uio_irqfn_t)(void *args);

__BEGIN_DECLS
struct uio_handle *uio_init(void);
struct uio_mem *uio_map_iomem(struct uio_handle *handle, const char *name,
		       paddr_t phys, size_t size);
int	uio_unmap_iomem(struct uio_handle *handle, struct uio_mem *mem);
void	uio_deinit(struct uio_handle *handle);

struct uio_irq *uio_attach_irq(struct uio_handle *handle, uio_irqfn_t irqfn,
			       void *args);

void	uio_write32(struct uio_mem *uio, off_t offset, uint32_t data);
uint32_t uio_read32(struct uio_mem *uio, off_t offset);
__END_DECLS

#endif
