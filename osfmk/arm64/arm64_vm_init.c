/*
 * Copyright 2013, winocm. <winocm@icloud.com>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 *   Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * 
 *   Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 * 
 *   If you are going to use this software in any form that does not involve
 *   releasing the source to this project or improving it, let me know beforehand.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * ARM VM initialization
 */

#include <mach_debug.h>
#include <debug.h>
#include <mach/vm_types.h>
#include <mach/vm_param.h>
#include <mach/thread_status.h>
#include <kern/misc_protos.h>
#include <kern/assert.h>
#include <kern/cpu_number.h>
#include <kern/thread.h>
#include <console/serial_protos.h>
#include <libkern/kernel_mach_header.h>
#include <arm/pmap.h>
#include <arm/misc_protos.h>

/*
 * cpu_ttb contains the current TTB (translation-table
 * base) of the processor. This is a physical address.
 */
uint64_t    cpu_ttb;

/*
 * The section offset is set by the bootloader.
 */
uint32_t    sectionOffset = 0x2000;

/*
 * sane_size, max_mem and mem_size are controlled by arm_vm_init
 *
 * At the moment, sane_size is forced to the size of memory
 * the booter passes to the kernel.
 */
uint64_t    sane_size   = 0;
vm_size_t   mem_size    = 0;

/*
 * The physical and virtual base of the kernel. These are used in 
 */
unsigned long gPhysBase = 0x0, gVirtBase = 0x0;
unsigned long gTopOfKernel;

addr64_t vm_last_addr = VM_MAX_KERNEL_ADDRESS;

/*
 * Break down the KASLR. (pronunced like "castle" but with a twangy accent).
 */

/*
 * These variables are initialized during vm init.
 */
ppnum_t		vm_kernel_base_page;
vm_offset_t	vm_kernel_base;
vm_offset_t	vm_kernel_top;
vm_offset_t	vm_kernel_stext;
vm_offset_t	vm_kernel_etext;
vm_offset_t	vm_kernel_slide;

/*
 * These both are initialized to the same value.
 */
uint64_t        max_mem;
uint64_t        mem_actual;

/*
 * The sectXxx stuff contains the current kernel Mach-O section information.
 */
vm_offset_t sectTEXTB;
unsigned long sectSizeTEXT;
vm_offset_t sectDATAB;
unsigned long sectSizeDATA;
vm_offset_t sectLINKB;
unsigned long sectSizeLINK;
vm_offset_t sectKLDB;
unsigned long sectSizeKLD;
vm_offset_t sectPRELINKB;
unsigned long sectSizePRELINK;
vm_offset_t sectHIBB;
unsigned long sectSizeHIB;

vm_offset_t end, etext, edata;

extern void* ExceptionVectorsBase;

/*
 * These both represent the first physical page we can use in the system,
 * and the end of the physical memory region.
 */
uint64_t first_avail;
uint64_t avail_end;
