
	.syntax unified
	.cpu cortex-a9
	.text
	.balign 4

#if defined(FW_360)
	.set RET2LOAD_ADDR, 0x510012E8
	@For the ADDROF, take address of the function and add 0xD.
	@For the jmp_target, take the destination of BNE.
	.set sceSblAuthMgrAuthHeader_ADDROF, 0x51016D75
	.set sceSblAuthMgrSetupAuthSegment_ADDROF, 0x51016E65
	.set sceSblAuthMgrSetupAuthSegment_jmp_target, 0x51016E88
	.set sceSblAuthMgrAuthSegment_ADDROF, 0x51016EA1
	.set sceSblAuthMgrAuthSegment_jmp_target, 0x51016F1C
#elif defined(FW_365)
	.set RET2LOAD_ADDR, 0x510012E8
	.set sceSblAuthMgrAuthHeader_ADDROF, 0x51016EB1
	.set sceSblAuthMgrSetupAuthSegment_ADDROF, 0x51016FA1
	.set sceSblAuthMgrSetupAuthSegment_jmp_target, 0x51016FC4
	.set sceSblAuthMgrAuthSegment_ADDROF, 0x51016FDD
	.set sceSblAuthMgrAuthSegment_jmp_target, 0x51017058
#else
	Invalid firmware
#endif

	.global _start
	.type   _start, %function
	.section .text._start
_start:
	ldr pc, boot_main_ptr

boot_main_ptr:
	.word boot_main

	.global psp2bootconfig_load_hook
	.type   psp2bootconfig_load_hook, %function
psp2bootconfig_load_hook:
	@ Get CPU ID
	mrc p15, 0, ip, c0, c0, 5
	and ip, #0xF
	cmp ip, #0
	bne return2load_kernel

	push {r0, r2, r3}
	blx psp2bootconfig_load_hook_main
	pop {r0, r2, r3}

return2load_kernel:
	@Same instructions for 3.60 and 3.65 were overwritten
	movt r0, #0x5116
	movt r3, #0x5102
	mov r1, #0x110000

	movw ip, #:lower16:RET2LOAD_ADDR
	movt ip, #:upper16:RET2LOAD_ADDR
	add ip, #0xC @ Back to original code
	orr ip, #1   @ Thumb
	blx ip

	.global sceSblAuthMgrAuthHeader_patch
	.type   sceSblAuthMgrAuthHeader_patch, %function
sceSblAuthMgrAuthHeader_patch:

	cmp  r0, #0x1
	push {r4, r5, r6, r7, r8, lr}
	mov r5, r1
	mov r7, r2
	mov r4, r3

	movw ip, #:lower16:sceSblAuthMgrAuthHeader_ADDROF
	movt ip, #:upper16:sceSblAuthMgrAuthHeader_ADDROF

	bx ip

	.global sceSblAuthMgrSetupAuthSegment_patch
	.type   sceSblAuthMgrSetupAuthSegment_patch, %function
sceSblAuthMgrSetupAuthSegment_patch:

	cmp r0, #0x1
	push {r3, r4, r5, lr}
	mov r5, r1

	// Must to not jump
	bne sceSblAuthMgrSetupAuthSegment_jmp_target
	movw  r3, #0xf0f8

	movw ip, #:lower16:sceSblAuthMgrSetupAuthSegment_ADDROF
	movt ip, #:upper16:sceSblAuthMgrSetupAuthSegment_ADDROF

	bx ip

	.global sceSblAuthMgrAuthSegment_patch
	.type   sceSblAuthMgrAuthSegment_patch, %function
sceSblAuthMgrAuthSegment_patch:

	cmp  r0, #0x1
	push {r4, r5, r6, r7, r8, lr }
	mov  r4, r1
	mov  r6, r2

	// Must to not jump
	bne sceSblAuthMgrAuthSegment_jmp_target

	movw ip, #:lower16:sceSblAuthMgrAuthSegment_ADDROF
	movt ip, #:upper16:sceSblAuthMgrAuthSegment_ADDROF

	bx ip
