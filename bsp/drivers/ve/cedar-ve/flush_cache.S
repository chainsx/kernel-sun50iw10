#include <linux/linkage.h>

#define ENDPIPROC(x)			\
	.globl	__pi_##x;		\
	.type 	__pi_##x, %function;	\
	.set	__pi_##x, x;		\
	.size	__pi_##x, . - x;	\
	ENDPROC(x)

/*
 *	cedar_dma_flush_range(start, size)
 *
 *	clean & invalidate D / U line
 *
 *	- start   - virtual start address of region
 *	- size    - size in question
 */
#if IS_ENABLED(CONFIG_ARM64)
SYM_FUNC_START_PI(cedar_dma_flush_range)
    mrs	x4, DAIF
    msr	DAIFSet, #3
    mrs	x2, SP_EL0
    ldr	x2, [x2, #8]
    mrs	x3, TTBR1_EL1
    extr	x3, x3, x2, #48
    ror	x3, x3, #16
    msr	TTBR1_EL1, x3
    isb
    msr	TTBR0_EL1, x2
    isb
    msr	DAIF, x4
    mrs	x3, CTR_EL0
    nop
    ubfx	x3, x3, #16, #4
    mov	x2, #4
    lsl	x2, x2, x3
    add	x1, x0, x1
    sub	x3, x2, #1
    bic	x0, x0, x3
    dc	civac, x0
    add	x0, x0, x2
    cmp	x0, x1
    b.lo	#-12
    dsb	sy
    mrs	x2, DAIF
    msr	DAIFSet, #3
    mrs	x1, TTBR1_EL1
    and	x1, x1, #0xffffffffffff
    sub	x1, x1, #1, lsl #12     // =4096
    msr	TTBR0_EL1, x1
    isb
    add	x1, x1, #1, lsl #12     // =4096
    msr	TTBR1_EL1, x1
    isb
    msr	DAIF, x2
    ret
SYM_FUNC_END_PI(cedar_dma_flush_range)
#else
ENTRY(cedar_dma_flush_range)
	mrc     15, 0, r3, cr0, cr0, 1
	lsr     r3, r3, #16
	and     r3, r3, #15
	mov     r2, #4
	lsl     r2, r2, r3
	sub     r3, r2, #1
	bic     r0, r0, r3
1:
	mcr     15, 0, r0, cr7, cr14, 1
	add     r0, r0, r2
	cmp     r0, r1
	bcc     1b
	dsb     st
	bx      lr
ENDPROC(cedar_dma_flush_range)
#endif
