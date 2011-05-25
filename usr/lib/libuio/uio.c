
#include <stdlib.h>
#include <string.h>
#include <sys/prex.h>
#include <sys/list.h>
#include <uio.h>

struct uio_mem {
	char		name[20];
	void		*addr;
	paddr_t		phys_addr;
	size_t		size;
	struct list	link;
};

struct uio_irq {
	const char	*name;
	int		irq_nr;
	task_t		owner;
	struct list	link;
};

struct uio_handle {
	struct list	mem;
	struct list	irqs;
	device_t	uio_dev;
};

struct uio_handle *
uio_init(void)
{
	struct uio_handle *uio_handle;
	int err;

	uio_handle = malloc(sizeof(struct uio_handle));
	if (!uio_handle)
		return NULL;

	err = device_open("uio", 0, &uio_handle->uio_dev);
	if (err)
		goto err_free_uio;

	list_init(&uio_handle->mem);
	list_init(&uio_handle->irqs);
	return uio_handle;

err_free_uio:
	free(uio_handle);
	return NULL;
}

struct uio_mem *
uio_map_iomem(struct uio_handle *handle, const char *name,
	      paddr_t phys, size_t size)
{
	struct uio_mem *mem;
	int ret;

	mem = malloc(sizeof(struct uio_mem));
	if (!mem)
		return NULL;

	mem->phys_addr = phys;
	mem->size = size;
	strncpy(mem->name, name, sizeof(name));
	ret = vm_map_phys(phys, size, &mem->addr);
	if (ret < 0)
		goto out_free_mem;

	ret = vm_attribute(task_self(), mem->addr, PROT_IO);
	if (ret < 0)
		goto out_free_mem;

	list_init(&mem->link);
	list_insert(&handle->mem, &mem->link);
	return mem;
out_free_mem:
	free(mem);
	return NULL;
}

int
uio_unmap_iomem(struct uio_handle *handle, struct uio_mem *mem)
{
	list_remove(&mem->link);
	vm_free(task_self(), (void *)mem->phys_addr);
	free(mem);
	return 0;
}

void
uio_write32(struct uio_mem *mem, off_t offset, uint32_t data)
{
	*((volatile uint32_t *)((uint32_t)mem->addr + offset)) = data;
}

uint32_t
uio_read32(struct uio_mem *mem, off_t offset)
{
	return *((volatile uint32_t *)((uint32_t)mem->addr + offset));
}

