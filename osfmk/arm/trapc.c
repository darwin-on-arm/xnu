/*
 * Copyright 2013, winocm. <rms@velocitylimitless.org>
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
 * Instruction stuff from QEMU.
 */
/*
 * ARM trap handlers.
 */

#include <mach/mach_types.h>
#include <mach_assert.h>
#include <mach_kdp.h>
#include <kern/thread.h>
#include <kern/kalloc.h>
#include <stdarg.h>
#include <vm/vm_kern.h>
#include <vm/pmap.h>
#include <stdarg.h>
#include <machine/machine_routines.h>
#include <arm/misc_protos.h>
#include <pexpert/pexpert.h>
#include <pexpert/arm/boot.h>
#include <pexpert/arm/protos.h>
#include <vm/vm_map.h>
#include <libkern/OSByteOrder.h>
#include <arm/armops.h>

#define FAILURE_TRANSLATION     5   /* Translation fault on page */
#define FAILURE_SECTION         7   /* Translation fault on section */

#define FSR_FAIL                0xF /* Bits for failure in DFSR register */

struct opcode32
{
  unsigned long arch;		/* Architecture defining this insn.  */
  unsigned long value, mask;	/* Recognise insn if (op&mask)==value.  */
  const char *assembler;	/* How to disassemble this insn.  */
};

typedef enum {
    FPU_FPA_EXT_V1 = 0,
    FPU_NEON_EXT_V1,
    FPU_VFP_EXT_V1,
    FPU_VFP_EXT_V1xD,
    FPU_VFP_EXT_V2,
    FPU_VFP_EXT_V3
} opcode_archs;

static const struct opcode32 coprocessor_opcodes[] =
{
  /* Register load/store */
  {FPU_NEON_EXT_V1, 0x0d200b00, 0x0fb00f01, "vstmdb%c\t%16-19r%21'!, %B"},
  {FPU_NEON_EXT_V1, 0x0d300b00, 0x0fb00f01, "vldmdb%c\t%16-19r%21'!, %B"},
  {FPU_NEON_EXT_V1, 0x0c800b00, 0x0f900f01, "vstmia%c\t%16-19r%21'!, %B"},
  {FPU_NEON_EXT_V1, 0x0c900b00, 0x0f900f01, "vldmia%c\t%16-19r%21'!, %B"},
  {FPU_NEON_EXT_V1, 0x0d000b00, 0x0f300f00, "vstr%c\t%12-15,22D, %C"},
  {FPU_NEON_EXT_V1, 0x0d100b00, 0x0f300f00, "vldr%c\t%12-15,22D, %C"},

  /* Data transfer between ARM and NEON registers */
  {FPU_NEON_EXT_V1, 0x0e800b10, 0x0ff00f70, "vdup%c.32\t%16-19,7D, %12-15r"},
  {FPU_NEON_EXT_V1, 0x0e800b30, 0x0ff00f70, "vdup%c.16\t%16-19,7D, %12-15r"},
  {FPU_NEON_EXT_V1, 0x0ea00b10, 0x0ff00f70, "vdup%c.32\t%16-19,7Q, %12-15r"},
  {FPU_NEON_EXT_V1, 0x0ea00b30, 0x0ff00f70, "vdup%c.16\t%16-19,7Q, %12-15r"},
  {FPU_NEON_EXT_V1, 0x0ec00b10, 0x0ff00f70, "vdup%c.8\t%16-19,7D, %12-15r"},
  {FPU_NEON_EXT_V1, 0x0ee00b10, 0x0ff00f70, "vdup%c.8\t%16-19,7Q, %12-15r"},
  {FPU_NEON_EXT_V1, 0x0c400b10, 0x0ff00fd0, "vmov%c\t%0-3,5D, %12-15r, %16-19r"},
  {FPU_NEON_EXT_V1, 0x0c500b10, 0x0ff00fd0, "vmov%c\t%12-15r, %16-19r, %0-3,5D"},
  {FPU_NEON_EXT_V1, 0x0e000b10, 0x0fd00f70, "vmov%c.32\t%16-19,7D[%21d], %12-15r"},
  {FPU_NEON_EXT_V1, 0x0e100b10, 0x0f500f70, "vmov%c.32\t%12-15r, %16-19,7D[%21d]"},
  {FPU_NEON_EXT_V1, 0x0e000b30, 0x0fd00f30, "vmov%c.16\t%16-19,7D[%6,21d], %12-15r"},
  {FPU_NEON_EXT_V1, 0x0e100b30, 0x0f500f30, "vmov%c.%23?us16\t%12-15r, %16-19,7D[%6,21d]"},
  {FPU_NEON_EXT_V1, 0x0e400b10, 0x0fd00f10, "vmov%c.8\t%16-19,7D[%5,6,21d], %12-15r"},
  {FPU_NEON_EXT_V1, 0x0e500b10, 0x0f500f10, "vmov%c.%23?us8\t%12-15r, %16-19,7D[%5,6,21d]"},

  /* Floating point coprocessor (VFP) instructions */
  {FPU_VFP_EXT_V1xD, 0x0ef1fa10, 0x0fffffff, "fmstat%c"},
  {FPU_VFP_EXT_V1xD, 0x0ee00a10, 0x0fff0fff, "fmxr%c\tfpsid, %12-15r"},
  {FPU_VFP_EXT_V1xD, 0x0ee10a10, 0x0fff0fff, "fmxr%c\tfpscr, %12-15r"},
  {FPU_VFP_EXT_V1xD, 0x0ee60a10, 0x0fff0fff, "fmxr%c\tmvfr1, %12-15r"},
  {FPU_VFP_EXT_V1xD, 0x0ee70a10, 0x0fff0fff, "fmxr%c\tmvfr0, %12-15r"},
  {FPU_VFP_EXT_V1xD, 0x0ee80a10, 0x0fff0fff, "fmxr%c\tfpexc, %12-15r"},
  {FPU_VFP_EXT_V1xD, 0x0ee90a10, 0x0fff0fff, "fmxr%c\tfpinst, %12-15r\t@ Impl def"},
  {FPU_VFP_EXT_V1xD, 0x0eea0a10, 0x0fff0fff, "fmxr%c\tfpinst2, %12-15r\t@ Impl def"},
  {FPU_VFP_EXT_V1xD, 0x0ef00a10, 0x0fff0fff, "fmrx%c\t%12-15r, fpsid"},
  {FPU_VFP_EXT_V1xD, 0x0ef10a10, 0x0fff0fff, "fmrx%c\t%12-15r, fpscr"},
  {FPU_VFP_EXT_V1xD, 0x0ef60a10, 0x0fff0fff, "fmrx%c\t%12-15r, mvfr1"},
  {FPU_VFP_EXT_V1xD, 0x0ef70a10, 0x0fff0fff, "fmrx%c\t%12-15r, mvfr0"},
  {FPU_VFP_EXT_V1xD, 0x0ef80a10, 0x0fff0fff, "fmrx%c\t%12-15r, fpexc"},
  {FPU_VFP_EXT_V1xD, 0x0ef90a10, 0x0fff0fff, "fmrx%c\t%12-15r, fpinst\t@ Impl def"},
  {FPU_VFP_EXT_V1xD, 0x0efa0a10, 0x0fff0fff, "fmrx%c\t%12-15r, fpinst2\t@ Impl def"},
  {FPU_VFP_EXT_V1, 0x0e000b10, 0x0ff00fff, "fmdlr%c\t%z2, %12-15r"},
  {FPU_VFP_EXT_V1, 0x0e100b10, 0x0ff00fff, "fmrdl%c\t%12-15r, %z2"},
  {FPU_VFP_EXT_V1, 0x0e200b10, 0x0ff00fff, "fmdhr%c\t%z2, %12-15r"},
  {FPU_VFP_EXT_V1, 0x0e300b10, 0x0ff00fff, "fmrdh%c\t%12-15r, %z2"},
  {FPU_VFP_EXT_V1xD, 0x0ee00a10, 0x0ff00fff, "fmxr%c\t<impl def %16-19x>, %12-15r"},
  {FPU_VFP_EXT_V1xD, 0x0ef00a10, 0x0ff00fff, "fmrx%c\t%12-15r, <impl def %16-19x>"},
  {FPU_VFP_EXT_V1xD, 0x0e000a10, 0x0ff00f7f, "fmsr%c\t%y2, %12-15r"},
  {FPU_VFP_EXT_V1xD, 0x0e100a10, 0x0ff00f7f, "fmrs%c\t%12-15r, %y2"},
  {FPU_VFP_EXT_V1xD, 0x0eb50a40, 0x0fbf0f70, "fcmp%7'ezs%c\t%y1"},
  {FPU_VFP_EXT_V1, 0x0eb50b40, 0x0fbf0f70, "fcmp%7'ezd%c\t%z1"},
  {FPU_VFP_EXT_V1xD, 0x0eb00a40, 0x0fbf0fd0, "fcpys%c\t%y1, %y0"},
  {FPU_VFP_EXT_V1xD, 0x0eb00ac0, 0x0fbf0fd0, "fabss%c\t%y1, %y0"},
  {FPU_VFP_EXT_V1, 0x0eb00b40, 0x0fbf0fd0, "fcpyd%c\t%z1, %z0"},
  {FPU_VFP_EXT_V1, 0x0eb00bc0, 0x0fbf0fd0, "fabsd%c\t%z1, %z0"},
  {FPU_VFP_EXT_V1xD, 0x0eb10a40, 0x0fbf0fd0, "fnegs%c\t%y1, %y0"},
  {FPU_VFP_EXT_V1xD, 0x0eb10ac0, 0x0fbf0fd0, "fsqrts%c\t%y1, %y0"},
  {FPU_VFP_EXT_V1, 0x0eb10b40, 0x0fbf0fd0, "fnegd%c\t%z1, %z0"},
  {FPU_VFP_EXT_V1, 0x0eb10bc0, 0x0fbf0fd0, "fsqrtd%c\t%z1, %z0"},
  {FPU_VFP_EXT_V1, 0x0eb70ac0, 0x0fbf0fd0, "fcvtds%c\t%z1, %y0"},
  {FPU_VFP_EXT_V1, 0x0eb70bc0, 0x0fbf0fd0, "fcvtsd%c\t%y1, %z0"},
  {FPU_VFP_EXT_V1xD, 0x0eb80a40, 0x0fbf0fd0, "fuitos%c\t%y1, %y0"},
  {FPU_VFP_EXT_V1xD, 0x0eb80ac0, 0x0fbf0fd0, "fsitos%c\t%y1, %y0"},
  {FPU_VFP_EXT_V1, 0x0eb80b40, 0x0fbf0fd0, "fuitod%c\t%z1, %y0"},
  {FPU_VFP_EXT_V1, 0x0eb80bc0, 0x0fbf0fd0, "fsitod%c\t%z1, %y0"},
  {FPU_VFP_EXT_V1xD, 0x0eb40a40, 0x0fbf0f50, "fcmp%7'es%c\t%y1, %y0"},
  {FPU_VFP_EXT_V1, 0x0eb40b40, 0x0fbf0f50, "fcmp%7'ed%c\t%z1, %z0"},
  {FPU_VFP_EXT_V3, 0x0eba0a40, 0x0fbe0f50, "f%16?us%7?lhtos%c\t%y1, #%5,0-3k"},
  {FPU_VFP_EXT_V3, 0x0eba0b40, 0x0fbe0f50, "f%16?us%7?lhtod%c\t%z1, #%5,0-3k"},
  {FPU_VFP_EXT_V1xD, 0x0ebc0a40, 0x0fbe0f50, "fto%16?sui%7'zs%c\t%y1, %y0"},
  {FPU_VFP_EXT_V1, 0x0ebc0b40, 0x0fbe0f50, "fto%16?sui%7'zd%c\t%y1, %z0"},
  {FPU_VFP_EXT_V3, 0x0ebe0a40, 0x0fbe0f50, "fto%16?us%7?lhs%c\t%y1, #%5,0-3k"},
  {FPU_VFP_EXT_V3, 0x0ebe0b40, 0x0fbe0f50, "fto%16?us%7?lhd%c\t%z1, #%5,0-3k"},
  {FPU_VFP_EXT_V1, 0x0c500b10, 0x0fb00ff0, "fmrrd%c\t%12-15r, %16-19r, %z0"},
  {FPU_VFP_EXT_V3, 0x0eb00a00, 0x0fb00ff0, "fconsts%c\t%y1, #%0-3,16-19d"},
  {FPU_VFP_EXT_V3, 0x0eb00b00, 0x0fb00ff0, "fconstd%c\t%z1, #%0-3,16-19d"},
  {FPU_VFP_EXT_V2, 0x0c400a10, 0x0ff00fd0, "fmsrr%c\t%y4, %12-15r, %16-19r"},
  {FPU_VFP_EXT_V2, 0x0c400b10, 0x0ff00fd0, "fmdrr%c\t%z0, %12-15r, %16-19r"},
  {FPU_VFP_EXT_V2, 0x0c500a10, 0x0ff00fd0, "fmrrs%c\t%12-15r, %16-19r, %y4"},
  {FPU_VFP_EXT_V1xD, 0x0e000a00, 0x0fb00f50, "fmacs%c\t%y1, %y2, %y0"},
  {FPU_VFP_EXT_V1xD, 0x0e000a40, 0x0fb00f50, "fnmacs%c\t%y1, %y2, %y0"},
  {FPU_VFP_EXT_V1, 0x0e000b00, 0x0fb00f50, "fmacd%c\t%z1, %z2, %z0"},
  {FPU_VFP_EXT_V1, 0x0e000b40, 0x0fb00f50, "fnmacd%c\t%z1, %z2, %z0"},
  {FPU_VFP_EXT_V1xD, 0x0e100a00, 0x0fb00f50, "fmscs%c\t%y1, %y2, %y0"},
  {FPU_VFP_EXT_V1xD, 0x0e100a40, 0x0fb00f50, "fnmscs%c\t%y1, %y2, %y0"},
  {FPU_VFP_EXT_V1, 0x0e100b00, 0x0fb00f50, "fmscd%c\t%z1, %z2, %z0"},
  {FPU_VFP_EXT_V1, 0x0e100b40, 0x0fb00f50, "fnmscd%c\t%z1, %z2, %z0"},
  {FPU_VFP_EXT_V1xD, 0x0e200a00, 0x0fb00f50, "fmuls%c\t%y1, %y2, %y0"},
  {FPU_VFP_EXT_V1xD, 0x0e200a40, 0x0fb00f50, "fnmuls%c\t%y1, %y2, %y0"},
  {FPU_VFP_EXT_V1, 0x0e200b00, 0x0fb00f50, "fmuld%c\t%z1, %z2, %z0"},
  {FPU_VFP_EXT_V1, 0x0e200b40, 0x0fb00f50, "fnmuld%c\t%z1, %z2, %z0"},
  {FPU_VFP_EXT_V1xD, 0x0e300a00, 0x0fb00f50, "fadds%c\t%y1, %y2, %y0"},
  {FPU_VFP_EXT_V1xD, 0x0e300a40, 0x0fb00f50, "fsubs%c\t%y1, %y2, %y0"},
  {FPU_VFP_EXT_V1, 0x0e300b00, 0x0fb00f50, "faddd%c\t%z1, %z2, %z0"},
  {FPU_VFP_EXT_V1, 0x0e300b40, 0x0fb00f50, "fsubd%c\t%z1, %z2, %z0"},
  {FPU_VFP_EXT_V1xD, 0x0e800a00, 0x0fb00f50, "fdivs%c\t%y1, %y2, %y0"},
  {FPU_VFP_EXT_V1, 0x0e800b00, 0x0fb00f50, "fdivd%c\t%z1, %z2, %z0"},
  {FPU_VFP_EXT_V1xD, 0x0d200a00, 0x0fb00f00, "fstmdbs%c\t%16-19r!, %y3"},
  {FPU_VFP_EXT_V1xD, 0x0d200b00, 0x0fb00f00, "fstmdb%0?xd%c\t%16-19r!, %z3"},
  {FPU_VFP_EXT_V1xD, 0x0d300a00, 0x0fb00f00, "fldmdbs%c\t%16-19r!, %y3"},
  {FPU_VFP_EXT_V1xD, 0x0d300b00, 0x0fb00f00, "fldmdb%0?xd%c\t%16-19r!, %z3"},
  {FPU_VFP_EXT_V1xD, 0x0d000a00, 0x0f300f00, "fsts%c\t%y1, %A"},
  {FPU_VFP_EXT_V1, 0x0d000b00, 0x0f300f00, "fstd%c\t%z1, %A"},
  {FPU_VFP_EXT_V1xD, 0x0d100a00, 0x0f300f00, "flds%c\t%y1, %A"},
  {FPU_VFP_EXT_V1, 0x0d100b00, 0x0f300f00, "fldd%c\t%z1, %A"},
  {FPU_VFP_EXT_V1xD, 0x0c800a00, 0x0f900f00, "fstmias%c\t%16-19r%21'!, %y3"},
  {FPU_VFP_EXT_V1xD, 0x0c800b00, 0x0f900f00, "fstmia%0?xd%c\t%16-19r%21'!, %z3"},
  {FPU_VFP_EXT_V1xD, 0x0c900a00, 0x0f900f00, "fldmias%c\t%16-19r%21'!, %y3"},
  {FPU_VFP_EXT_V1xD, 0x0c900b00, 0x0f900f00, "fldmia%0?xd%c\t%16-19r%21'!, %z3"},

  {0, 0, 0, 0}
};


typedef enum {
    SLEH_ABORT_TYPE_PREFETCH_ABORT = 3,
    SLEH_ABORT_TYPE_DATA_ABORT = 4,
} sleh_abort_reasons;

void
arm_mach_do_exception(void)
{
	exception_triage(0, 0, 0);
}

/**
 * sleh_abort
 *
 * Handle prefetch and data aborts. (EXC_BAD_ACCESS IS NOT HERE YET)
 */
void sleh_abort(void* context, int reason)
{
    vm_map_t map;
    thread_t thread;
    kern_return_t kr;
    abort_information_context_t* abort_context = (abort_information_context_t*)context;
    uint32_t prot = VM_PROT_READ;

    if(!abort_context) {
        panic("sleh_abort: abort handler called but with no context");
    }

    /* Could be a page fault */
    {
        map = kernel_map;
        thread = current_thread();
#if 0
        /* Dump current register values */
        kprintf("*** POSSIBLE PAGE FAULT ***\n");
        kprintf("sleh_abort: register dump %d: fault_addr=0x%x\n"
                          "r0: 0x%08x  r1: 0x%08x  r2: 0x%08x  r3: 0x%08x\n"
                          "r4: 0x%08x  r5: 0x%08x  r6: 0x%08x  r7: 0x%08x\n"
                          "r8: 0x%08x  r9: 0x%08x r10: 0x%08x r11: 0x%08x\n"
                          "12: 0x%08x  sp: 0x%08x  lr: 0x%08x  pc: 0x%08x\n"
                          "cpsr: 0x%08x fsr: 0x%08x far: 0x%08x\n",
                          reason, abort_context->far,
                          abort_context->gprs[0], abort_context->gprs[1], abort_context->gprs[2], abort_context->gprs[3],
                          abort_context->gprs[4], abort_context->gprs[5], abort_context->gprs[6], abort_context->gprs[7],
                          abort_context->gprs[8], abort_context->gprs[9], abort_context->gprs[10], abort_context->gprs[11],
                          abort_context->gprs[12], abort_context->sp, abort_context->lr, abort_context->pc,
                          abort_context->cpsr, abort_context->fsr, abort_context->far
                          );
#endif
        /* If it was a hardware error, let us know. */
        if((abort_context->fsr & 0xF) == 0x8) {
            if(abort_context->fsr & (1 << 12))
                panic_context(0, (void*)abort_context, "sleh_abort: axi slave error %d: fault_addr=0x%x\n"
                              "r0: 0x%08x  r1: 0x%08x  r2: 0x%08x  r3: 0x%08x\n"
                              "r4: 0x%08x  r5: 0x%08x  r6: 0x%08x  r7: 0x%08x\n"
                              "r8: 0x%08x  r9: 0x%08x r10: 0x%08x r11: 0x%08x\n"
                              "12: 0x%08x  sp: 0x%08x  lr: 0x%08x  pc: 0x%08x\n"
                              "cpsr: 0x%08x fsr: 0x%08x far: 0x%08x\n",
                              reason, abort_context->far,
                              abort_context->gprs[0], abort_context->gprs[1], abort_context->gprs[2], abort_context->gprs[3],
                              abort_context->gprs[4], abort_context->gprs[5], abort_context->gprs[6], abort_context->gprs[7],
                              abort_context->gprs[8], abort_context->gprs[9], abort_context->gprs[10], abort_context->gprs[11],
                              abort_context->gprs[12], abort_context->sp, abort_context->lr, abort_context->pc,
                              abort_context->cpsr, abort_context->fsr, abort_context->far
                              );
            else
                panic_context(0, (void*)abort_context, "sleh_abort: axi decode error %d: fault_addr=0x%x\n"
                              "r0: 0x%08x  r1: 0x%08x  r2: 0x%08x  r3: 0x%08x\n"
                              "r4: 0x%08x  r5: 0x%08x  r6: 0x%08x  r7: 0x%08x\n"
                              "r8: 0x%08x  r9: 0x%08x r10: 0x%08x r11: 0x%08x\n"
                              "12: 0x%08x  sp: 0x%08x  lr: 0x%08x  pc: 0x%08x\n"
                              "cpsr: 0x%08x fsr: 0x%08x far: 0x%08x\n",
                              reason, abort_context->far,
                              abort_context->gprs[0], abort_context->gprs[1], abort_context->gprs[2], abort_context->gprs[3],
                              abort_context->gprs[4], abort_context->gprs[5], abort_context->gprs[6], abort_context->gprs[7],
                              abort_context->gprs[8], abort_context->gprs[9], abort_context->gprs[10], abort_context->gprs[11],
                              abort_context->gprs[12], abort_context->sp, abort_context->lr, abort_context->pc,
                              abort_context->cpsr, abort_context->fsr, abort_context->far
                              );
        }

        /* Make sure the kernel map isn't null. Dump context if it is. */
        if(!kernel_map) {
            kprintf("*** kernel_map is null, did something fail during VM initialization?\n");
            goto panicOut;
        }
        
        /* Check to see if the thread isn't null */
        if(!thread) {
            kprintf("*** thread was null, did something break before the first thread was set?\n");
            goto panicOut;
        }
        
        /* Eeek. */
        if(get_preemption_level()) {
            kprintf("*** data abort called but preemption level %d is not zero!\n", get_preemption_level());
            goto panicOut;
        }
        
        /* Check to see if it is a fault */
        if(((abort_context->fsr & FSR_FAIL) == FAILURE_TRANSLATION) ||
           ((abort_context->fsr & FSR_FAIL) == FAILURE_SECTION)) {
            map = thread->map;
            assert(map);
            /* Attempt to fault it */
            kr = vm_fault(map, vm_map_trunc_page(abort_context->far), prot,
                        FALSE, THREAD_UNINT, NULL, 0);
            if(kr != KERN_SUCCESS)
                kprintf("*** vm_fault failed with code 0x%08x\n", kr);
            if(kr == KERN_SUCCESS)
                return;
        }
        
        /* Call the recovery routine if there is one. */
		if (thread != THREAD_NULL && thread->recover) {
			abort_context->pc = thread->recover;
			thread->recover = 0;
			return;
		}
        
        /* If it's a user process, let us know. */
        {
            ;
        }
        
    }
panicOut:
    switch(reason) {
        case SLEH_ABORT_TYPE_PREFETCH_ABORT: {
            /* Print out the long long nice prefetch abort. */
            panic_context(0, (void*)abort_context, "sleh_abort: prefetch abort type %d: fault_addr=0x%x\n"
                          "r0: 0x%08x  r1: 0x%08x  r2: 0x%08x  r3: 0x%08x\n"
                          "r4: 0x%08x  r5: 0x%08x  r6: 0x%08x  r7: 0x%08x\n"
                          "r8: 0x%08x  r9: 0x%08x r10: 0x%08x r11: 0x%08x\n"
                          "12: 0x%08x  sp: 0x%08x  lr: 0x%08x  pc: 0x%08x\n"
                          "cpsr: 0x%08x fsr: 0x%08x far: 0x%08x\n",
                          reason, abort_context->far,
                          abort_context->gprs[0], abort_context->gprs[1], abort_context->gprs[2], abort_context->gprs[3],
                          abort_context->gprs[4], abort_context->gprs[5], abort_context->gprs[6], abort_context->gprs[7],
                          abort_context->gprs[8], abort_context->gprs[9], abort_context->gprs[10], abort_context->gprs[11],
                          abort_context->gprs[12], abort_context->sp, abort_context->lr, abort_context->pc,
                          abort_context->cpsr, abort_context->fsr, abort_context->far
                          );
        }
        case SLEH_ABORT_TYPE_DATA_ABORT: {
            /* Print out the long long nice data abort. */
            panic_context(0, (void*)abort_context, "sleh_abort: data abort type %d: fault_addr=0x%x\n"
                       "r0: 0x%08x  r1: 0x%08x  r2: 0x%08x  r3: 0x%08x\n"
                       "r4: 0x%08x  r5: 0x%08x  r6: 0x%08x  r7: 0x%08x\n"
                       "r8: 0x%08x  r9: 0x%08x r10: 0x%08x r11: 0x%08x\n"
                       "12: 0x%08x  sp: 0x%08x  lr: 0x%08x  pc: 0x%08x\n"
                       "cpsr: 0x%08x fsr: 0x%08x far: 0x%08x\n",
                       reason, abort_context->far,
                       abort_context->gprs[0], abort_context->gprs[1], abort_context->gprs[2], abort_context->gprs[3],
                       abort_context->gprs[4], abort_context->gprs[5], abort_context->gprs[6], abort_context->gprs[7],
                       abort_context->gprs[8], abort_context->gprs[9], abort_context->gprs[10], abort_context->gprs[11],
                       abort_context->gprs[12], abort_context->sp, abort_context->lr, abort_context->pc,
                       abort_context->cpsr, abort_context->fsr, abort_context->far
                       );
        }
        default:
            panic("sleh_abort: unknown abort called (context: %p, reason: %d)!\n", context, reason);
    }
    
    while(1);
}

/**
 * irq_handler
 *
 * Handle irqs and pass them over to the platform expert.
 */
boolean_t irq_handler(void* context)
{
    __disable_preemption();
    boolean_t ret = pe_arm_dispatch_interrupt(context);
    __enable_preemption();
    return ret;
}

/**
 * sleh_undef
 *
 * Handle undefined instructions and VFP usage.
 */
void sleh_undef(arm_saved_state_t* state)
{
    boolean_t is_kernel = FALSE;
    boolean_t matched = FALSE;
    thread_t current = current_thread();
    uint32_t instruction;
    uint32_t thumb_offset = 0;
    uint32_t mask;
    uint32_t value;
    const struct opcode32 *insn;
  
    /* Ugh. */
    assert(state);
    assert(current);
    
    /* If it's the kernel, god /damn/ us. */
    is_kernel = ((state->cpsr & 0x1F) != 0x10);
    thumb_offset = (state->cpsr & ~(1 << 5)) ? 1 : 0;

    /* Verify vfp state */
    if(is_kernel && current->machine.vfp_enable) {
        panic("why are you using vfp on a kernel thread?");
    }
    
    if(is_kernel) {                         /* if it isn't user, panic immediately. */
        panic_context(0, (void*)state, "sleh_undef: undefined kernel instruction\n"
                                                "r0: 0x%08x  r1: 0x%08x  r2: 0x%08x  r3: 0x%08x\n"
                                                "r4: 0x%08x  r5: 0x%08x  r6: 0x%08x  r7: 0x%08x\n"
                                                "r8: 0x%08x  r9: 0x%08x r10: 0x%08x r11: 0x%08x\n"
                                                "12: 0x%08x  sp: 0x%08x  lr: 0x%08x  pc: 0x%08x\n"
                                                "cpsr: 0x%08x\n",
                                                state->r[0], state->r[1], state->r[2], state->r[3],
                                                state->r[4], state->r[5], state->r[6], state->r[7],
                                                state->r[8], state->r[9], state->r[10], state->r[11],
                                                state->r[12], state->sp, state->lr, state->pc, state->cpsr);
    }
    
    /* Attempt to handle it */
    copyin((uint8_t*)(state->pc) + thumb_offset, &instruction, sizeof(uint32_t));
    instruction = OSSwapInt32(instruction);
    
    /*
     * This is really overboard for just checking VFP/NEON opcodes,
     * but I really couldn't be arsed to find the documentation
     * for it, so I just took it from the qemu dissassembler.
     *
     * It works for what its worth.
     */
    for(insn = coprocessor_opcodes; insn->assembler; insn++) {
        mask = insn->mask;
        value = insn->value;
        if(thumb_offset) {
            mask |= 0xF0000000;
            value |= 0xE0000000;
        } else {
            /* conditionals in arm are different */
            if ((instruction & 0xF0000000) == 0xF0000000) {
                mask |= 0xF0000000;
            }
        }
        if((instruction & mask) == value) {
            matched = TRUE;
            break;
        }
    }

    /* This is a VFP instruction */
    if(matched && !is_kernel) {
        current->machine.vfp_dirty = 0;
        if(!current->machine.vfp_enable) {
            vfp_enable_exception(TRUE);
            vfp_context_load(&current->machine.vfp_regs);
            current->machine.vfp_enable = 1;
        }
        return;
    }

    /* should notify user and not die here. */
    panic_context(0, (void*)state, "sleh_undef: undefined user instruction\n"
                  "r0: 0x%08x  r1: 0x%08x  r2: 0x%08x  r3: 0x%08x\n"
                  "r4: 0x%08x  r5: 0x%08x  r6: 0x%08x  r7: 0x%08x\n"
                  "r8: 0x%08x  r9: 0x%08x r10: 0x%08x r11: 0x%08x\n"
                  "12: 0x%08x  sp: 0x%08x  lr: 0x%08x  pc: 0x%08x\n"
                  "cpsr: 0x%08x\n",
                  state->r[0], state->r[1], state->r[2], state->r[3],
                  state->r[4], state->r[5], state->r[6], state->r[7],
                  state->r[8], state->r[9], state->r[10], state->r[11],
                  state->r[12], state->sp, state->lr, state->pc, state->cpsr);
}