
#include <stdint.h>
#include <stddef.h>
#include <psp2kern/types.h>
#include "enso/functions.h"
#include "arm_opcode.h"
#include "hen.h"

#define DCACHE_ALIGN(x_) ((x_) & ~0x3F)

#ifndef FORCE_SD0_BOOT_PATCHES
#define FORCE_SD0_BOOT_PATCHES 0
#endif

#if defined(FW_360) || defined(FW_365) //Good for 3.60 and 3.65 CEX
	//Offsets in SceSysStateMgr .text
	#define SSM_BLX_ISEXTBOOTMODE_OFFSET 0x76
	#define SSM_BLX_ISDECRYPTALLOW_OFFSET 0xE28
	#define SSM_BLX_ISMANUFMODE_OFFSET 0x1502

	//Offset to the segment 0's base in module object
	#define MODOBJ_SEG0_BASE_VADDR_OFFSET (0x7C)
#endif

#if defined(FW_365)
#define sceKernelLoadModule_ADDROF 0x51017A6C
#define sceSblAuthMgrAuthHeader_ADDROF 0x51016EA4
#define sceSblAuthMgrSetupAuthSegment_ADDROF 0x51016F94
#define sceSblAuthMgrAuthSegment_ADDROF 0x51016FD0

static void* (*const ReferModuleObject)(int) = (void*)(0x51017784 | 1);

#elif defined(FW_360)
#define sceKernelLoadModule_ADDROF 0x51017930
#define sceSblAuthMgrAuthHeader_ADDROF 0x51016D68
#define sceSblAuthMgrSetupAuthSegment_ADDROF 0x51016E58
#define sceSblAuthMgrAuthSegment_ADDROF 0x51016E94


static void* (*const ReferModuleObject)(int) = (void*)(0x51017648 | 1);
#else
#error Unsupported firmware.
#endif

static int sceKernelLoadModule(const char *path, int flags, void *option){

	int res;

	if((flags & 0xfff8260f) == 0){
		/* Better algorithm? :shrug:
		if (strncmp(path, "os0:", 4) != 0) { //Only redirect os0
			res = sceKernelLoadModuleForPidInternal(0x10005, path, flags, option);
		} else { //Try redirected
			DACR_OFF(path[0] = 's'; path[1] = 'd';)
			res = sceKernelLoadModuleForPidInternal(0x10005, path, flags, option);

			if (res == FILE NOT FOUND) { //Fallback to os0
				sceKernelPrintf("NOT FOUND ");
				DACR_OFF(path[0] = 'o'; path[1] = 's';)
				return sceKernelLoadModuleForPidInternal(0x10005, path, flags, option);
			}
		}
		if (res < 0) {
			sceKernelPrintf("%s res:0x%X %s 0x%X\n", __FUNCTION__, res, path, flags);
		}

		return res;
		*/

		if(strncmp(path, "os0:kd/acmgr.skprx", 18) == 0){
			path = "sd0:kd/acmgr.skprx";
		}else if(strncmp(path, "os0:kd/deci4p_sdbgp.skprx", 25) == 0){
			path = "sd0:kd/deci4p_sdbgp.skprx";
		}else if(strncmp(path, "os0:kd/deci4p_sdrfp.skprx", 25) == 0){
			path = "sd0:kd/deci4p_sdrfp.skprx";
		}else if(strncmp(path, "os0:kd/sdbgsdio.skprx", 21) == 0){
			path = "sd0:kd/sdbgsdio.skprx";
		}else if(strncmp(path, "os0:kd/intrmgr.skprx", 21) == 0){
			path = "sd0:kd/intrmgr.skprx";
		}else if(strncmp(path, "os0:psp2bootconfig.skprx", 24) == 0){
			path = "sd0:psp2bootconfig.skprx";
		}else if (strncmp(path, "os0:kd/hdmi.skprx", 18) == 0) {
			path = "sd0:kd/hdmi.skprx";
		}

		res = sceKernelLoadModuleForPidInternal(0x10005, path, flags, option);
		if(res < 0){
			sceKernelPrintf("%s res:0x%X %s 0x%X\n", __FUNCTION__, res, path, flags);
		}

		if (FORCE_SD0_BOOT_PATCHES && res > 0 && strncmp(path + 4, "kd/sysstagemgr.skprx", 25) == 0) { //Patch sysstatemgr
			//Obtain module's .text base
			uintptr_t pModule = (uintptr_t)ReferModuleObject(res);
			uintptr_t seg0Base = *((uintptr_t*)(pModule + MODOBJ_SEG0_BASE_VADDR_OFFSET));

			//Replace a few function calls with a pseudo "return 1"
			uint32_t movw_r0_1 = 0x0001F240;
			
			uint32_t dacr;
			__asm__ volatile("mrc p15, 0, %0, c3, c0, 0" : "=r"(dacr));
			__asm__ volatile("mcr p15, 0, %0, c3, c0, 0" :: "r"(0xFFFFFFFF));
			*(uint32_t*)(seg0Base + SSM_BLX_ISEXTBOOTMODE_OFFSET) = movw_r0_1;
			*(uint32_t*)(seg0Base + SSM_BLX_ISMANUFMODE_OFFSET) = movw_r0_1;
			*(uint32_t*)(seg0Base + SSM_BLX_ISDECRYPTALLOW_OFFSET) = movw_r0_1;
			__asm__ volatile("mcr p15, 0, %0, c3, c0, 0" :: "r"(dacr));

			clean_dcache((void*)DCACHE_ALIGN(seg0Base + SSM_BLX_ISEXTBOOTMODE_OFFSET), 0x40);
			clean_dcache((void*)DCACHE_ALIGN(seg0Base + SSM_BLX_ISMANUFMODE_OFFSET), 0x40);
			clean_dcache((void*)DCACHE_ALIGN(seg0Base + SSM_BLX_ISDECRYPTALLOW_OFFSET), 0x40);
			flush_icache();
		}

		return res;
	}

	sceKernelPrintf("%s flags error\n", __FUNCTION__);

	return 0x8002000A;
}

int g_sigpatch_disabled = 0;
int g_homebrew_decrypt = 0;
#define SBLAUTHMGR_OFFSET_PATCH_ARG (168)

int sceSblAuthMgrAuthHeader_patch(uint32_t ctx, const void *header, int len, void *args);

static int sceSblAuthMgrAuthHeader(uint32_t ctx, const void *header, int len, void *args){

	int ret = sceSblAuthMgrAuthHeader_patch(ctx, header, len, args);

	g_homebrew_decrypt = (ret < 0);

	if(g_homebrew_decrypt){
		*(uint32_t *)(args + SBLAUTHMGR_OFFSET_PATCH_ARG) = 0x40;
		ret = 0;
	}

	return ret;
}

int sceSblAuthMgrSetupAuthSegment_patch(int ctx, uint32_t segidx);

static int sceSblAuthMgrSetupAuthSegment(int ctx, uint32_t segidx){

	if(g_homebrew_decrypt){
		return 2; // always compressed!
	}

	return sceSblAuthMgrSetupAuthSegment_patch(ctx, segidx);
}

int sceSblAuthMgrAuthSegment_patch(uint32_t ctx, void *buf, int sz);

static int sceSblAuthMgrAuthSegment(uint32_t ctx, void *buf, int sz){

	if(g_homebrew_decrypt){
		return 0;
	}

	return sceSblAuthMgrAuthSegment_patch(ctx, buf, sz);
}

int nskbl_install_hen(void){

	uintptr_t patch_func;
	int opcode[3];

	// for load kernel module sd0:
	if(sceSblAimgrIsTool() != 0 || *(uint16_t *)(0x40200100 + 0xA2) == 0x101 || FORCE_SD0_BOOT_PATCHES){
		patch_func = (uintptr_t)sceKernelLoadModule;

		get_movw_opcode(&opcode[0], 12, (uint16_t)(patch_func));
		get_movt_opcode(&opcode[1], 12, (uint16_t)(patch_func >> 16));

		opcode[2] = 0xBF004760; // blx ip, nop

		memcpy((void *)sceKernelLoadModule_ADDROF, opcode, sizeof(opcode));

		clean_dcache((void *)sceKernelLoadModule_ADDROF, 0x40);
		flush_icache();

		patch_func = (uintptr_t)sceSblAuthMgrAuthHeader;

		get_movw_opcode(&opcode[0], 12, (uint16_t)(patch_func));
		get_movt_opcode(&opcode[1], 12, (uint16_t)(patch_func >> 16));

		opcode[2] = 0xBF004760; // blx ip, nop

		memcpy((void *)DCACHE_ALIGN(sceSblAuthMgrAuthHeader_ADDROF), opcode, sizeof(opcode));

		clean_dcache((void *)DCACHE_ALIGN(sceSblAuthMgrAuthHeader_ADDROF), 0x40);
		flush_icache();

		patch_func = (uintptr_t)sceSblAuthMgrSetupAuthSegment;

		get_movw_opcode(&opcode[0], 12, (uint16_t)(patch_func));
		get_movt_opcode(&opcode[1], 12, (uint16_t)(patch_func >> 16));

		opcode[2] = 0xBF004760; // blx ip, nop

		memcpy((void *)sceSblAuthMgrSetupAuthSegment_ADDROF, opcode, sizeof(opcode));

		clean_dcache((void *)DCACHE_ALIGN(sceSblAuthMgrSetupAuthSegment_ADDROF), 0x40);
		flush_icache();

		patch_func = (uintptr_t)sceSblAuthMgrAuthSegment;

		get_movw_opcode(&opcode[0], 12, (uint16_t)(patch_func));
		get_movt_opcode(&opcode[1], 12, (uint16_t)(patch_func >> 16));

		opcode[2] = 0xBF004760; // blx ip, nop

		memcpy((void *)sceSblAuthMgrAuthSegment_ADDROF, opcode, sizeof(opcode));

		clean_dcache((void *)DCACHE_ALIGN(sceSblAuthMgrAuthSegment_ADDROF), 0x40);
		flush_icache();
	}

    return 0;
}
