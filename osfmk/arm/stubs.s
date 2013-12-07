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
 * Unimplemented stubs.
 */

#define _genQuoteString(x) #x
#define genString(x) _genQuoteString(x is not implemented.)

#define UNIMPLEMENTED_STUB(Function)            \
    .align 4;                                   \
    .globl Function                         ;   \
    Function:                               ;   \
        nop                                 ;   \
        nop                                 ;   \
        nop                                 ;   \
        nop                                 ;   \
        nop                                 ;   \
        nop                                 ;   \
        nop                                 ;   \
        nop                                 ;   \
        nop                                 ;   \
        nop                                 ;   \
        nop                                 ;   \
        nop                                 ;   \
        ldr     r0, ps_ptr_ ##Function      ;   \
        blx     _panic                      ;   \
    ps_ptr_ ##Function:                     ;   \
        .long   panicString_ ##Function     ;   \
    panicString_ ##Function:                ;   \
        .asciz  genString(Function)         ;

UNIMPLEMENTED_STUB(_LockTimeOut)
UNIMPLEMENTED_STUB(__MachineStateCount)
UNIMPLEMENTED_STUB(_aout_db_init)
UNIMPLEMENTED_STUB(_aout_db_line_at_pc)
UNIMPLEMENTED_STUB(_aout_db_lookup)
UNIMPLEMENTED_STUB(_aout_db_lookup_incomplete)
UNIMPLEMENTED_STUB(_aout_db_print_completion)
UNIMPLEMENTED_STUB(_aout_db_search_by_addr)
UNIMPLEMENTED_STUB(_aout_db_search_symbol)
UNIMPLEMENTED_STUB(_aout_db_sym_init)
UNIMPLEMENTED_STUB(_aout_db_symbol_values)
UNIMPLEMENTED_STUB(_coredumpok)
UNIMPLEMENTED_STUB(_cpu_sleep)
UNIMPLEMENTED_STUB(_db_inst_load)
UNIMPLEMENTED_STUB(_dcache_incoherent_io_flush64)
UNIMPLEMENTED_STUB(_dcache_incoherent_io_store64)
UNIMPLEMENTED_STUB(_debug_task)
UNIMPLEMENTED_STUB(_fasttrap_pid_getarg)
UNIMPLEMENTED_STUB(_fasttrap_pid_probe)
UNIMPLEMENTED_STUB(_fasttrap_return_probe)
UNIMPLEMENTED_STUB(_fasttrap_tracepoint_init)
UNIMPLEMENTED_STUB(_fasttrap_tracepoint_install)
UNIMPLEMENTED_STUB(_fasttrap_tracepoint_remove)
UNIMPLEMENTED_STUB(_fasttrap_usdt_getarg)
UNIMPLEMENTED_STUB(_fbt_invop)
UNIMPLEMENTED_STUB(_fbt_perfCallback)
UNIMPLEMENTED_STUB(_fbt_provide_module)
UNIMPLEMENTED_STUB(_flush_dcache64)
UNIMPLEMENTED_STUB(_gIOHibernateRestoreStack)
UNIMPLEMENTED_STUB(_gIOHibernateRestoreStackEnd)
UNIMPLEMENTED_STUB(_hibernate_machine_entrypoint)
UNIMPLEMENTED_STUB(_hibernate_newruntime_map)
UNIMPLEMENTED_STUB(_hibernate_page_list_allocate)
UNIMPLEMENTED_STUB(_hibernate_page_list_set_volatile)
UNIMPLEMENTED_STUB(_hibernate_page_list_setall_machine)
UNIMPLEMENTED_STUB(_hibernate_processor_setup)
UNIMPLEMENTED_STUB(_hibernate_restore_phys_page)
UNIMPLEMENTED_STUB(_hibernate_vm_lock)
UNIMPLEMENTED_STUB(_hibernate_vm_unlock)
UNIMPLEMENTED_STUB(_kdb_on)
UNIMPLEMENTED_STUB(_kern_dump)
UNIMPLEMENTED_STUB(_machine_signal_idle)
UNIMPLEMENTED_STUB(_mapping_set_mod)
UNIMPLEMENTED_STUB(_ml_nofault_copydeclare_stub)
UNIMPLEMENTED_STUB(_ml_stack_remaining)
UNIMPLEMENTED_STUB(_sdt_invop)
UNIMPLEMENTED_STUB(_slave_machine_init)
UNIMPLEMENTED_STUB(_tempDTraceTrapHook)
UNIMPLEMENTED_STUB(_thread_kdb_return)

UNIMPLEMENTED_STUB(_chudxnu_thread_ast)
UNIMPLEMENTED_STUB(_cpu_data_ptr)
UNIMPLEMENTED_STUB(_cpu_exit_wait)
UNIMPLEMENTED_STUB(_cpuid_cpusubtype)
UNIMPLEMENTED_STUB(_cpuid_cputype)
UNIMPLEMENTED_STUB(_dtrace_modload)
UNIMPLEMENTED_STUB(_dtrace_modunload)
UNIMPLEMENTED_STUB(_gIOHibernateCurrentHeader)
UNIMPLEMENTED_STUB(_gIOHibernateDebugFlags)
UNIMPLEMENTED_STUB(_gIOHibernateHandoffPageCount)
UNIMPLEMENTED_STUB(_gIOHibernateHandoffPages)
UNIMPLEMENTED_STUB(_gIOHibernateState)
UNIMPLEMENTED_STUB(_handle_pending_TLB_flushes)
UNIMPLEMENTED_STUB(_hibernate_page_bitmap_count)
UNIMPLEMENTED_STUB(_hibernate_page_bitmap_pin)
UNIMPLEMENTED_STUB(_hibernate_page_bitset)
UNIMPLEMENTED_STUB(_hibernate_page_bittst)
UNIMPLEMENTED_STUB(_hibernate_sum_page)
UNIMPLEMENTED_STUB(_kdp_machine_get_breakinsn)
UNIMPLEMENTED_STUB(_lockstat_probe)
UNIMPLEMENTED_STUB(_lockstat_probemap)
UNIMPLEMENTED_STUB(_machine_callstack)
UNIMPLEMENTED_STUB(_machine_processor_shutdown)
UNIMPLEMENTED_STUB(_machine_task_get_state)
UNIMPLEMENTED_STUB(_machine_task_set_state)
UNIMPLEMENTED_STUB(_machine_timeout_suspended)
UNIMPLEMENTED_STUB(_machine_trace_thread)
UNIMPLEMENTED_STUB(_machine_trace_thread64)
UNIMPLEMENTED_STUB(_panic_display_pal_info)
UNIMPLEMENTED_STUB(_save_kdebug_enable)
UNIMPLEMENTED_STUB(_sigreturn)
UNIMPLEMENTED_STUB(_thread_setsinglestep)
