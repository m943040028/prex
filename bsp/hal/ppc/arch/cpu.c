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
 * cpu.c - cpu dependent routines
 */

#include <kernel.h>
#include <cpu.h>
#include <locore.h>
#include <cpufunc.h>

static int curspl = 15;

int
splhigh(void)
{
	int oldspl = curspl;

	sploff();
	curspl = 15;
	return oldspl;
}

int
spl0(void)
{
	int oldspl = curspl;

	curspl = 0;
	splon();
	return oldspl;
}

void
splx(int s)
{

	curspl = s;
	if (curspl == 0)
		splon();
	else
		sploff();
}


int
copyin(const void *uaddr, void *kaddr, size_t len)
{

	if (user_area(uaddr) && user_area((u_char *)uaddr + len)) {
		memcpy(kaddr, uaddr, len);
		return 0;
	}
	return EFAULT;
}

int
copyout(const void *kaddr, void *uaddr, size_t len)
{

	if (user_area(uaddr) && user_area((u_char *)uaddr + len)) {
		memcpy(uaddr, kaddr, len);
		return 0;
	}
	return EFAULT;
}

int
copyinstr(const void *uaddr, void *kaddr, size_t len)
{

	if (user_area(uaddr) && user_area((u_char *)uaddr + len)) {
		strlcpy(kaddr, uaddr, len);
		return 0;
	}
	return EFAULT;
}

#if defined(CONFIG_MMU)
void __inline
write_tlb_entry(uint8_t entry, uint32_t data, uint32_t tag)
{
	__asm__ __volatile__(
		"tlbwe  %1, %0, 1\n" /* data */
		"tlbwe  %2, %0, 0\n" /* tag  */
		"isync\n"
		: /* no output */
		: "r" (entry), "r" (data), "r" (tag)
		: "memory"
	);
}

void __inline
read_tlb_entry(uint8_t entry, uint32_t *data, uint32_t *tag,
	       uint32_t *asid)
{
	uint32_t orig_asid;

	__asm__ __volatile__(
		"mfspr	%[orig_as], %[pid]\n"
		"tlbre	%[t], %[e], 0\n" /* tag */
		"mfspr	%[as], %[pid]\n" /* PID */
		"tlbre	%[d], %[e], 1\n" /* data */
		"mtspr	%[pid], %[orig_as]"
		: [d] "=r"(*data), [t] "=r" (*tag),
		  [as] "=r" (*asid), [orig_as] "=r" (orig_asid)
		: [e] "r" (entry), [pid]"i"(SPR_PID)
		: "cc"
	);
}
#endif
