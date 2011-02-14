/*-
 * Copyright (c) 2009, Kohsuke Ohtani
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

/*
 * mmu_booke.c - memory management unit support for
 * PowerPC Book-E Architecture.
 */

#include <machine/syspage.h>
#include <kernel.h>
#include <page.h>
#include <kmem.h>
#include <mmu.h>
#include <cpu.h>
#include <cpufunc.h>

/*
 * Boot page directory.
 * This works as a template for all page directory.
 */
static pgd_t boot_pgd = (pgd_t)BOOT_PGD;
static pgd_t current_pgd;

/* TLB entry index */
static uint8_t tlb_entry_index;
#define CONFIG_NTLB_ENTS	64

typedef struct tlb_entry_s {
	uint8_t	tlb_locked : 1;
	uint8_t tlb_cachable : 1;
	uint8_t tlb_write_through : 1;
	uint8_t tlb_writable : 1;
	uint8_t tlb_executable : 1;
	uint8_t tlb_pagesize;
	paddr_t	phys_addrs;
	vaddr_t virt_addrs;
} tlb_entry_t;
static tlb_entry_t tlb_entries[CONFIG_NTLB_ENTS];

void __inline
_mmu_init_tlb_entry(tlb_entry_t *e, uint8_t page_size, int cachable)
{

	e->tlb_cachable = cachable;
	e->tlb_write_through = 0;
	e->tlb_pagesize = page_size;
}

void __inline
mmu_init_tlb_entry(tlb_entry_t *e, uint8_t page_size)
{
	_mmu_init_tlb_entry(e, page_size, 1);
}

void __inline
mmu_init_tlb_entry_rw(tlb_entry_t *e, uint8_t page_size)
{

	e->tlb_writable = 1;
	_mmu_init_tlb_entry(e, page_size, 1);
}

void __inline
mmu_init_tlb_entry_exec(tlb_entry_t *e, uint8_t page_size)
{

	e->tlb_executable = 1;
	mmu_init_tlb_entry_rw(e, page_size);
}

void __inline
mmu_init_tlb_entry_io(tlb_entry_t *e, uint8_t page_size)
{

	_mmu_init_tlb_entry(e, page_size, 0);
}

void
mmu_dump_tlb_entries(void)
{
	int i = 0;
	for (; i < CONFIG_NTLB_ENTS; i++) {
		tlb_entry_t *e = &tlb_entries[i];
		printf("TLB %02d, %08x -> %08x, %c%c%c%c\n",
			i, e->virt_addrs, e->phys_addrs,
			e->tlb_writable ? 'w':'-',
			e->tlb_executable ? 'x' : '-',
			e->tlb_cachable ? 'c' : '-',
			e->tlb_locked ? 'l' : '-');
	}
}

/*
 * Called via every timer ticks, used to randomly choose a
 * victim TLB entry.
 */
void __inline
mmu_tlb_index_update(void)
{

	tlb_entry_index++;
	if (tlb_entry_index == CONFIG_NTLB_ENTS)
		tlb_entry_index = 0;
}

uint8_t
mmu_find_first_usable_tlb_entry(void)
{

	uint8_t first = tlb_entry_index;
	while (tlb_locked(&tlb_entries[tlb_entry_index]))
	{
		mmu_tlb_index_update();
		if (first == tlb_entry_index)
			panic("No free TLB entry is avaiable\n");
	}

	return tlb_entry_index;
}

void
mmu_update_tlb_entry(tlb_entry_t *e)
{
	write_tlb_entry(e-tlb_entries,
			e->phys_addrs |
			(tlb_writable(e) ? TLB_WR : 0) |
			(tlb_executable(e) ? TLB_EX : 0),
			e->virt_addrs | TLB_VALID |
			TLB_PAGESZ(e->tlb_pagesize));
}

int
mmu_replace_tlb_entry(vaddr_t va, paddr_t pa, pte_t pte)
{

	uint8_t index = mmu_find_first_usable_tlb_entry();
	tlb_entry_t *e = &tlb_entries[index];
	asid_t asid;

	/* TODO: Handle more than 4k pages */
	if (page_executable(pte, va))
		mmu_init_tlb_entry_exec(e, PAGESZ_4K);
	else if (page_writable(pte, va))
		mmu_init_tlb_entry_rw(e, PAGESZ_4K);
	else if (page_io(pte, va))
		mmu_init_tlb_entry_io(e, PAGESZ_4K);
	else
		panic("Unknown page table entry field\n");

	e->virt_addrs = (va & ~PAGE_MASK);
	e->phys_addrs = (pa & ~PAGE_MASK);
	asid = pgd_get_asid(mmu_get_current_pgd());

	DPRINTF(("TLB %d(ASID %d), %08x -> %08x, %c%c%c\n",
		index, asid, e->virt_addrs, e->phys_addrs,
		e->tlb_writable ? 'w':'-',
		e->tlb_executable ? 'x' : '-',
		e->tlb_cachable ? 'c' : '-'
		));
	
	mmu_update_tlb_entry(e);
	return 0;
}

/*
 * Map physical memory range into virtual address
 *
 * Returns 0 on success, or ENOMEM on failure.
 *
 * Map type can be one of the following type.
 *   PG_UNMAP  - Remove mapping
 *   PG_READ   - Read only mapping
 *   PG_WRITE  - Read/write allowed
 *   PG_KERNEL - Kernel page
 *   PG_IO     - I/O memory
 *
 * Setup the appropriate page tables for mapping. If there is no
 * page table for the specified address, new page table is
 * allocated.
 *
 * This routine does not return any error even if the specified
 * address has been already mapped to other physical address.
 * In this case, it will just override the existing mapping.
 *
 * In order to unmap the page, pg_type is specified as 0.  But,
 * the page tables are not released even if there is no valid
 * page entry in it. All page tables are released when mmu_delmap()
 * is called when task is terminated.
 */
int
mmu_map(pgd_t pgd, paddr_t pa, vaddr_t va, size_t size, int type)
{
	uint32_t pte_flag = 0;
	uint32_t pde_flag = 0;
	pte_t pte;
	paddr_t pg;

	pa = round_page(pa);
	va = round_page(va);
	size = trunc_page(size);

	pde_flag = (uint32_t)PDE_PRESENT;
	/*
	 * Set page flag
	 */
	switch (type) {
	case PG_UNMAP:
		pte_flag = 0;
		break;
	case PG_READ:
		pte_flag = (uint32_t)(PTE_PRESENT | PTE_USER | PTE_EXEC);
		break;
	case PG_WRITE:
		pte_flag = (uint32_t)(PTE_PRESENT | PTE_WRITE | PTE_USER);
		break;
	case PG_SYSTEM:
		pte_flag = (uint32_t)(PTE_PRESENT | PTE_WRITE | PTE_EXEC);
		break;
	case PG_IOMEM:
		pte_flag = (uint32_t)(PTE_PRESENT | PTE_WRITE | PTE_IO);
		break;
	default:
		panic("mmu_map");
	}
	DPRINTF(("%s: %08x -> %08x size %08x, type %08x\n",
		__func__, va, pa, size, type));
	/*
	 * Map all pages
	 */
	while (size > 0) {
		if (pte_present(pgd, va)) {
			/* Page table already exists for the address */
			pte = vtopte(pgd, va);
		} else {
			ASSERT(pte_flag != 0);
			/* Page table does not exist, allocate one */
			if ((pg = page_alloc(PAGE_SIZE)) == 0) {
				DPRINTF(("Error: MMU mapping failed\n"));
				return ENOMEM;
			}
			pgd_get_kv(pgd)[PAGE_DIR(va)] = (uint32_t)pg | pde_flag;
			pte = (pte_t)ptokv(pg);
			memset(pte, 0, PAGE_SIZE);
		}
		/* Set new entry into page table */
		pte[PAGE_TABLE(va)] = (uint32_t)pa | pte_flag;

		/* Process next page */
		pa += PAGE_SIZE;
		va += PAGE_SIZE;
		size -= PAGE_SIZE;
	}
	/*flush_tlb();*/
	return 0;
}

asid_t
mmu_allocate_asid(void)
{

	static asid_t asid = 0;
	return asid++;
}

/*
 * Create new page map.
 *
 * Returns a page directory on success, or NULL on failure.  This
 * routine is called when new task is created. All page map must
 * have the same kernel page table in it. So, the kernel page
 * tables are copied to newly created map.
 */
pgd_t
mmu_newmap(void)
{
	paddr_t pg;
	pgd_t pgd;
	int i;

	/* Allocate page directory */
	if ((pg = page_alloc(PAGE_SIZE)) == 0)
		return NO_PGD;
	pgd = (pgd_t)ptokv(pg);
	memset(pgd, 0, PAGE_SIZE);

	/* Copy kernel page tables */
	i = PAGE_DIR(KERNBASE);
	memcpy(&pgd[i], &boot_pgd[i], (size_t)(1024 - i));

	/* Assign Address Space id for this new map */
	pgd_set_asid(pgd, mmu_allocate_asid());

	return pgd;
}

/*
 * Terminate all page mapping.
 */
void
mmu_terminate(pgd_t pgd)
{
	int i;
	pte_t pte;

	/*flush_tlb();*/

	/* Release all user page table */
	for (i = 0; i < PAGE_DIR(KERNBASE); i++) {
		pte = (pte_t)pgd_get_kv(pgd)[i];
		if (pte != 0)
			page_free((paddr_t)((paddr_t)pte & PTE_ADDRESS),
				  PAGE_SIZE);
	}
	/* Release page directory */
	page_free(kvtop(pgd_get_kv(pgd)), PAGE_SIZE);
}

/*
 * Switch to new page directory
 *
 * This is called when context is switched.
 */
void
mmu_switch(pgd_t pgd)
{
	current_pgd = pgd;

	mtspr(SPR_PID, pgd_get_asid(pgd));
	DPRINTF(("switch to asid %d\n", pgd_get_asid(pgd)));
}

/*
 * Get Current Working PGD
 */
pgd_t
mmu_get_current_pgd(void)
{
	return current_pgd;
}

/*
 * Returns the physical address for the specified virtual address.
 * This routine checks if the virtual area actually exist.
 * It returns 0 if at least one page is not mapped.
 */
paddr_t
mmu_extract(pgd_t pgd, vaddr_t va, size_t size)
{
	pte_t pte;
	vaddr_t start, end, pg;
	paddr_t pa;

	start = trunc_page(va);
	end = trunc_page(va + size - 1);

	/* Check all pages exist */
	for (pg = start; pg <= end; pg += PAGE_SIZE) {
		if (!pte_present(pgd, pg))
			return 0;
		pte = vtopte(pgd, pg);
		if (!page_present(pte, pg))
			return 0;
	}

	/* Get physical address */
	pte = vtopte(pgd, start);
	pa = (paddr_t)ptetopg(pte, start);
	return pa + (paddr_t)(va - start);
}

/*
 * Initialize mmu
 *
 * Paging is already enabled in locore.S. And, physical address
 * 0-4M has been already mapped into kernel space in locore.S.
 * Now, all physical memory is mapped into kernel virtual address
 * as straight 1:1 mapping. User mode access is not allowed for
 * these kernel pages.
 * page_init() must be called before calling this routine.
 *
 * Note: This routine requires 4K bytes to map 4M bytes memory. So,
 * if the system has a lot of RAM, the "used memory" by kernel will
 * become large, too. For example, page table requires 512K bytes
 * for 512M bytes system RAM.
 */
void
mmu_init(struct mmumap *mmumap_table)
{
	struct mmumap *map;
	int map_type = 0;

	for (map = mmumap_table; map->type != 0; map++) {
		switch (map->type) {
		case VMT_RAM:
		case VMT_ROM:
		case VMT_DMA:
			map_type = PG_SYSTEM;
			break;
		case VMT_IO:
			map_type = PG_IOMEM;
			break;
		}

		if (mmu_map(boot_pgd, map->phys, map->virt,
			    (size_t)map->size, map_type))
			panic("mmu_init");
	}

	/* 
	 * Reserve TLB63 for kernel flat mapping
	 *  (which is set via locore_44x.S)
	 */
	mmu_init_tlb_entry_rw(&tlb_entries[63], PAGESZ_16M);
	tlb_lock(&tlb_entries[63]);
}

/*
 * Map I/O memory for diagnostic device at very early stage.
 */
void
mmu_premap(paddr_t phys, vaddr_t virt)
{
	virt &= TLB_EPN_MASK;
	phys &= TLB_RPN_MASK;

	/* this tlb entry is safe to be replaced after boot_pgd is
	   setup for mmaped I/O space */
	write_tlb_entry(0, phys | TLB_WR,
			virt | TLB_VALID | TLB_PAGESZ(PAGESZ_4K));
	tlb_lock(&tlb_entries[0]);
}
