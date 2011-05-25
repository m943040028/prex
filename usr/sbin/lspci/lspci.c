
#include <sys/prex.h>
#include <pci.h>
#include <stdio.h>

int
main(int argc, char **argv)
{
	struct pci_handle *handle = pci_attach();
	struct pci_func_handle *func_handle;
	pci_probe_t probe;
	
	probe.type = PCI_MATCH_PROBE;
	pci_scan_device(handle, probe);
	
	printf("list of pci devices:\n");
	while (pci_get_func(handle, &func_handle) != EOF)
	{
		int i;
		pci_func_id_t func;
		uint8_t irqline;
		pci_func_get_func_id(func_handle, &func);
		printf("%02x:%02x.%d (%04x:%04x)\n",
			pci_func_busno(func),
			pci_func_dev(func),
			pci_func_func(func),
			pci_func_vendor(func),
			pci_func_product(func));
		for (i = 0; i < 6; i++) {
			struct pci_mem_region region;
			pci_func_get_mem_region(func_handle, i, &region);
			if (region.size)
				printf(" bar[%d] %08x - %08x\n", i,
					(uint32_t)region.base,
					(uint32_t)(region.base+region.size));
		}
		pci_func_get_irqline(func_handle, &irqline);
		printf(" irqline: %d\n", irqline);
	}

	pci_detach(handle);
	return 0;
}
