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

#ifndef _PPC_MMU_H
#define _PPC_MMU_H

#include <sys/types.h>

#if defined(CONFIG_PPC_BOOKE)

typedef uint32_t	*pgd_t;		/* page directory */
typedef uint32_t	*pte_t;		/* page table entry */

/* page directory entry */
#define PDE_PRESENT	0x00000001
#define PDE_ADDRESS	0xffc00000

/* page table entry */
#define PTE_PRESENT	0x00000001
#define PTE_WRITE       0x00000002
#define PTE_EXEC        0x00000004
#define PTE_NCACHE	0x00000008
#define PTE_WT		0x00000010
#define PTE_USER	0x00000020
#define PTE_ADDRESS	0xfffff000

#define PTE_IO		PTE_NCACHE

/*
 *  Virtual and physical address translation
 */
#define PAGE_DIR(virt)      (int)((((vaddr_t)(virt)) >> 22) & 0x3ff)
#define PAGE_TABLE(virt)    (int)((((vaddr_t)(virt)) >> 12) & 0xff)

/*
 * Page Directory Operator
 */
#define pte_present(pgd, virt) (pgd[PAGE_DIR(virt)] & PDE_PRESENT)

/*
 * Page Table Entry Operator
 */
#define page_present(pte, virt) (pte[PAGE_TABLE(virt)] & PTE_PRESENT)
#define page_writable(pte, virt) (pte[PAGE_TABLE(virt)] & PTE_WRITE)
#define page_executable(pte, virt) (pte[PAGE_TABLE(virt)] & PTE_EXEC)
#define page_io(pte, virt) (pte[PAGE_TABLE(virt)] & PTE_IO)
#define page_user(pte, virt) (pte[PAGE_TABLE(virt)] & PTE_USER)

/*
 * TLB Entry Operator
 */
#define tlb_lock(tlb) ((tlb)->tlb_locked = 1)
#define tlb_unlock(tlb) ((tlb)->tlb_locked = 0)
#define tlb_locked(tlb) ((tlb)->tlb_locked)
#define tlb_writalbe(tlb) ((tlb)->tlb_writable)
#define tlb_cachable(tlb) ((tlb)->tlb_cachable)
#define tlb_executable(tlb) ((tlb)->tlb_executable)

#define vtopte(pgd, virt) \
            (pte_t)ptokv((pgd)[PAGE_DIR(virt)] & PDE_ADDRESS)

#define ptetopg(pte, virt) \
            ((pte)[PAGE_TABLE(virt)] & PTE_ADDRESS)

__BEGIN_DECLS
pgd_t	mmu_get_current_pgd(void);
void	mmu_tlb_index_update(void);
int	mmu_replace_tlb_entry(vaddr_t, paddr_t, pte_t);
__END_DECLS

#elif

typedef uint32_t	*pgd_t;		/* page directory */

struct pte {
	uint32_t	pte_hi;
	uint32_t	pte_lo;
};
typedef struct pte	*pte_t;		/* page table entry */

/*
 * Page table entry
 */
/* high word: */
#define PTE_VALID	0x00000001	/* valid */
#define	PTE_VSID	0x7fffff80	/* virtual segment */
#define	PTE_HID		0x00000040	/* hash */
#define	PTE_API		0x0000003f	/* abbreviated page index */

/* low word: */
#define	PTE_RPN		0xfffff000	/* real page number */
#define	PTE_REF		0x00000100	/* reference */
#define	PTE_CHG		0x00000080	/* change */
#define	PTE_WIMG	0x00000078	/* memory/cache control */
#define	PTE_PP		0x00000003	/* page protection */

/*
 *  Virtual and physical address translation
 */
#define PAGE_DIR(virt)
#define PAGE_TABLE(virt)

#define pte_present(pgd, virt)

#define page_present(pte, virt)

#define vtopte(pgd, virt)

#define ptetopg(pte, virt)

#endif /* !CONFIG_PPC_BOOKE */
#endif /* !_PPC_MMU_H */
