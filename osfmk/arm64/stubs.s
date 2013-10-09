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

/*
 * To whoever who reads this file, it came from osfmk/arm/stubs.s. The new
 * pretty ARM64 'as' is a frontend for 'clang' and does not support the pretty
 * voodoo header magic I use. I would file a radar, but no one seems to care.
 *
 * I'm sorry for the inconvenience, and your eyes.
 *
 * To generate this, I ran:
 *  $ clang -E osfmk/arm/stubs.s | tr ';' '\n' > osfmk/arm64/stubs.s
 */

#define blx     bl      /* BLX doesn't exist. */

#if 0
# 1 "osfmk/arm64/stubs.s"
# 1 "<built-in>" 1
# 1 "osfmk/arm64/stubs.s" 2
# 59 "osfmk/arm64/stubs.s"
#endif

#define _Debugger _panic

.align 6
 .globl _LockTimeOut 
 _LockTimeOut: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__LockTimeOut 
 blx _Debugger 
 ps_ptr__LockTimeOut: 
 .long panicString__LockTimeOut 
 panicString__LockTimeOut: 
 .asciz "_LockTimeOut is not implemented." 


.align 6
 .globl __MachineStateCount 
 __MachineStateCount: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr___MachineStateCount 
 blx _Debugger 
 ps_ptr___MachineStateCount: 
 .long panicString___MachineStateCount 
 panicString___MachineStateCount: 
 .asciz "__MachineStateCount is not implemented." 

.align 6
 .globl __longjmp 
 __longjmp: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr___longjmp 
 blx _Debugger 
 ps_ptr___longjmp: 
 .long panicString___longjmp 
 panicString___longjmp: 
 .asciz "__longjmp is not implemented." 

.align 6
 .globl __setjmp 
 __setjmp: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr___setjmp 
 blx _Debugger 
 ps_ptr___setjmp: 
 .long panicString___setjmp 
 panicString___setjmp: 
 .asciz "__setjmp is not implemented." 


.align 6
 .globl _aout_db_init 
 _aout_db_init: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__aout_db_init 
 blx _Debugger 
 ps_ptr__aout_db_init: 
 .long panicString__aout_db_init 
 panicString__aout_db_init: 
 .asciz "_aout_db_init is not implemented." 

.align 6
 .globl _aout_db_line_at_pc 
 _aout_db_line_at_pc: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__aout_db_line_at_pc 
 blx _Debugger 
 ps_ptr__aout_db_line_at_pc: 
 .long panicString__aout_db_line_at_pc 
 panicString__aout_db_line_at_pc: 
 .asciz "_aout_db_line_at_pc is not implemented." 

.align 6
 .globl _aout_db_lookup 
 _aout_db_lookup: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__aout_db_lookup 
 blx _Debugger 
 ps_ptr__aout_db_lookup: 
 .long panicString__aout_db_lookup 
 panicString__aout_db_lookup: 
 .asciz "_aout_db_lookup is not implemented." 

.align 6
 .globl _aout_db_lookup_incomplete 
 _aout_db_lookup_incomplete: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__aout_db_lookup_incomplete 
 blx _Debugger 
 ps_ptr__aout_db_lookup_incomplete: 
 .long panicString__aout_db_lookup_incomplete 
 panicString__aout_db_lookup_incomplete: 
 .asciz "_aout_db_lookup_incomplete is not implemented." 

.align 6
 .globl _aout_db_print_completion 
 _aout_db_print_completion: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__aout_db_print_completion 
 blx _Debugger 
 ps_ptr__aout_db_print_completion: 
 .long panicString__aout_db_print_completion 
 panicString__aout_db_print_completion: 
 .asciz "_aout_db_print_completion is not implemented." 

.align 6
 .globl _aout_db_search_by_addr 
 _aout_db_search_by_addr: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__aout_db_search_by_addr 
 blx _Debugger 
 ps_ptr__aout_db_search_by_addr: 
 .long panicString__aout_db_search_by_addr 
 panicString__aout_db_search_by_addr: 
 .asciz "_aout_db_search_by_addr is not implemented." 

.align 6
 .globl _aout_db_search_symbol 
 _aout_db_search_symbol: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__aout_db_search_symbol 
 blx _Debugger 
 ps_ptr__aout_db_search_symbol: 
 .long panicString__aout_db_search_symbol 
 panicString__aout_db_search_symbol: 
 .asciz "_aout_db_search_symbol is not implemented." 

.align 6
 .globl _aout_db_sym_init 
 _aout_db_sym_init: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__aout_db_sym_init 
 blx _Debugger 
 ps_ptr__aout_db_sym_init: 
 .long panicString__aout_db_sym_init 
 panicString__aout_db_sym_init: 
 .asciz "_aout_db_sym_init is not implemented." 

.align 6
 .globl _aout_db_symbol_values 
 _aout_db_symbol_values: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__aout_db_symbol_values 
 blx _Debugger 
 ps_ptr__aout_db_symbol_values: 
 .long panicString__aout_db_symbol_values 
 panicString__aout_db_symbol_values: 
 .asciz "_aout_db_symbol_values is not implemented." 


.align 6
 .globl _atomic_add_64 
 _atomic_add_64: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__atomic_add_64 
 blx _Debugger 
 ps_ptr__atomic_add_64: 
 .long panicString__atomic_add_64 
 panicString__atomic_add_64: 
 .asciz "_atomic_add_64 is not implemented." 

.align 6
 .globl _consider_machine_adjust 
 _consider_machine_adjust: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__consider_machine_adjust 
 blx _Debugger 
 ps_ptr__consider_machine_adjust: 
 .long panicString__consider_machine_adjust 
 panicString__consider_machine_adjust: 
 .asciz "_consider_machine_adjust is not implemented." 

.align 6
 .globl _coredumpok 
 _coredumpok: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__coredumpok 
 blx _Debugger 
 ps_ptr__coredumpok: 
 .long panicString__coredumpok 
 panicString__coredumpok: 
 .asciz "_coredumpok is not implemented." 

.align 6
 .globl _cpu_sleep 
 _cpu_sleep: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__cpu_sleep 
 blx _Debugger 
 ps_ptr__cpu_sleep: 
 .long panicString__cpu_sleep 
 panicString__cpu_sleep: 
 .asciz "_cpu_sleep is not implemented." 


.align 6
 .globl _db_inst_load 
 _db_inst_load: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__db_inst_load 
 blx _Debugger 
 ps_ptr__db_inst_load: 
 .long panicString__db_inst_load 
 panicString__db_inst_load: 
 .asciz "_db_inst_load is not implemented." 


.align 6
 .globl _dcache_incoherent_io_flush64 
 _dcache_incoherent_io_flush64: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__dcache_incoherent_io_flush64 
 blx _Debugger 
 ps_ptr__dcache_incoherent_io_flush64: 
 .long panicString__dcache_incoherent_io_flush64 
 panicString__dcache_incoherent_io_flush64: 
 .asciz "_dcache_incoherent_io_flush64 is not implemented." 

.align 6
 .globl _dcache_incoherent_io_store64 
 _dcache_incoherent_io_store64: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__dcache_incoherent_io_store64 
 blx _Debugger 
 ps_ptr__dcache_incoherent_io_store64: 
 .long panicString__dcache_incoherent_io_store64 
 panicString__dcache_incoherent_io_store64: 
 .asciz "_dcache_incoherent_io_store64 is not implemented." 


.align 6
 .globl _debug_task 
 _debug_task: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__debug_task 
 blx _Debugger 
 ps_ptr__debug_task: 
 .long panicString__debug_task 
 panicString__debug_task: 
 .asciz "_debug_task is not implemented." 


.align 6
 .globl _fasttrap_pid_getarg 
 _fasttrap_pid_getarg: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__fasttrap_pid_getarg 
 blx _Debugger 
 ps_ptr__fasttrap_pid_getarg: 
 .long panicString__fasttrap_pid_getarg 
 panicString__fasttrap_pid_getarg: 
 .asciz "_fasttrap_pid_getarg is not implemented." 

.align 6
 .globl _fasttrap_pid_probe 
 _fasttrap_pid_probe: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__fasttrap_pid_probe 
 blx _Debugger 
 ps_ptr__fasttrap_pid_probe: 
 .long panicString__fasttrap_pid_probe 
 panicString__fasttrap_pid_probe: 
 .asciz "_fasttrap_pid_probe is not implemented." 

.align 6
 .globl _fasttrap_return_probe 
 _fasttrap_return_probe: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__fasttrap_return_probe 
 blx _Debugger 
 ps_ptr__fasttrap_return_probe: 
 .long panicString__fasttrap_return_probe 
 panicString__fasttrap_return_probe: 
 .asciz "_fasttrap_return_probe is not implemented." 

.align 6
 .globl _fasttrap_tracepoint_init 
 _fasttrap_tracepoint_init: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__fasttrap_tracepoint_init 
 blx _Debugger 
 ps_ptr__fasttrap_tracepoint_init: 
 .long panicString__fasttrap_tracepoint_init 
 panicString__fasttrap_tracepoint_init: 
 .asciz "_fasttrap_tracepoint_init is not implemented." 

.align 6
 .globl _fasttrap_tracepoint_install 
 _fasttrap_tracepoint_install: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__fasttrap_tracepoint_install 
 blx _Debugger 
 ps_ptr__fasttrap_tracepoint_install: 
 .long panicString__fasttrap_tracepoint_install 
 panicString__fasttrap_tracepoint_install: 
 .asciz "_fasttrap_tracepoint_install is not implemented." 

.align 6
 .globl _fasttrap_tracepoint_remove 
 _fasttrap_tracepoint_remove: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__fasttrap_tracepoint_remove 
 blx _Debugger 
 ps_ptr__fasttrap_tracepoint_remove: 
 .long panicString__fasttrap_tracepoint_remove 
 panicString__fasttrap_tracepoint_remove: 
 .asciz "_fasttrap_tracepoint_remove is not implemented." 

.align 6
 .globl _fasttrap_usdt_getarg 
 _fasttrap_usdt_getarg: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__fasttrap_usdt_getarg 
 blx _Debugger 
 ps_ptr__fasttrap_usdt_getarg: 
 .long panicString__fasttrap_usdt_getarg 
 panicString__fasttrap_usdt_getarg: 
 .asciz "_fasttrap_usdt_getarg is not implemented." 


.align 6
 .globl _fbt_invop 
 _fbt_invop: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__fbt_invop 
 blx _Debugger 
 ps_ptr__fbt_invop: 
 .long panicString__fbt_invop 
 panicString__fbt_invop: 
 .asciz "_fbt_invop is not implemented." 

.align 6
 .globl _fbt_perfCallback 
 _fbt_perfCallback: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__fbt_perfCallback 
 blx _Debugger 
 ps_ptr__fbt_perfCallback: 
 .long panicString__fbt_perfCallback 
 panicString__fbt_perfCallback: 
 .asciz "_fbt_perfCallback is not implemented." 

.align 6
 .globl _fbt_provide_module 
 _fbt_provide_module: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__fbt_provide_module 
 blx _Debugger 
 ps_ptr__fbt_provide_module: 
 .long panicString__fbt_provide_module 
 panicString__fbt_provide_module: 
 .asciz "_fbt_provide_module is not implemented." 

.align 6
 .globl _flush_dcache64 
 _flush_dcache64: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__flush_dcache64 
 blx _Debugger 
 ps_ptr__flush_dcache64: 
 .long panicString__flush_dcache64 
 panicString__flush_dcache64: 
 .asciz "_flush_dcache64 is not implemented." 


.align 6
 .globl _gIOHibernateRestoreStack 
 _gIOHibernateRestoreStack: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__gIOHibernateRestoreStack 
 blx _Debugger 
 ps_ptr__gIOHibernateRestoreStack: 
 .long panicString__gIOHibernateRestoreStack 
 panicString__gIOHibernateRestoreStack: 
 .asciz "_gIOHibernateRestoreStack is not implemented." 

.align 6
 .globl _gIOHibernateRestoreStackEnd 
 _gIOHibernateRestoreStackEnd: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__gIOHibernateRestoreStackEnd 
 blx _Debugger 
 ps_ptr__gIOHibernateRestoreStackEnd: 
 .long panicString__gIOHibernateRestoreStackEnd 
 panicString__gIOHibernateRestoreStackEnd: 
 .asciz "_gIOHibernateRestoreStackEnd is not implemented." 


.align 6
 .globl _hibernate_machine_entrypoint 
 _hibernate_machine_entrypoint: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__hibernate_machine_entrypoint 
 blx _Debugger 
 ps_ptr__hibernate_machine_entrypoint: 
 .long panicString__hibernate_machine_entrypoint 
 panicString__hibernate_machine_entrypoint: 
 .asciz "_hibernate_machine_entrypoint is not implemented." 

.align 6
 .globl _hibernate_newruntime_map 
 _hibernate_newruntime_map: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__hibernate_newruntime_map 
 blx _Debugger 
 ps_ptr__hibernate_newruntime_map: 
 .long panicString__hibernate_newruntime_map 
 panicString__hibernate_newruntime_map: 
 .asciz "_hibernate_newruntime_map is not implemented." 

.align 6
 .globl _hibernate_page_list_allocate 
 _hibernate_page_list_allocate: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__hibernate_page_list_allocate 
 blx _Debugger 
 ps_ptr__hibernate_page_list_allocate: 
 .long panicString__hibernate_page_list_allocate 
 panicString__hibernate_page_list_allocate: 
 .asciz "_hibernate_page_list_allocate is not implemented." 

.align 6
 .globl _hibernate_page_list_set_volatile 
 _hibernate_page_list_set_volatile: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__hibernate_page_list_set_volatile 
 blx _Debugger 
 ps_ptr__hibernate_page_list_set_volatile: 
 .long panicString__hibernate_page_list_set_volatile 
 panicString__hibernate_page_list_set_volatile: 
 .asciz "_hibernate_page_list_set_volatile is not implemented." 

.align 6
 .globl _hibernate_page_list_setall_machine 
 _hibernate_page_list_setall_machine: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__hibernate_page_list_setall_machine 
 blx _Debugger 
 ps_ptr__hibernate_page_list_setall_machine: 
 .long panicString__hibernate_page_list_setall_machine 
 panicString__hibernate_page_list_setall_machine: 
 .asciz "_hibernate_page_list_setall_machine is not implemented." 

.align 6
 .globl _hibernate_processor_setup 
 _hibernate_processor_setup: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__hibernate_processor_setup 
 blx _Debugger 
 ps_ptr__hibernate_processor_setup: 
 .long panicString__hibernate_processor_setup 
 panicString__hibernate_processor_setup: 
 .asciz "_hibernate_processor_setup is not implemented." 

.align 6
 .globl _hibernate_restore_phys_page 
 _hibernate_restore_phys_page: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__hibernate_restore_phys_page 
 blx _Debugger 
 ps_ptr__hibernate_restore_phys_page: 
 .long panicString__hibernate_restore_phys_page 
 panicString__hibernate_restore_phys_page: 
 .asciz "_hibernate_restore_phys_page is not implemented." 

.align 6
 .globl _hibernate_vm_lock 
 _hibernate_vm_lock: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__hibernate_vm_lock 
 blx _Debugger 
 ps_ptr__hibernate_vm_lock: 
 .long panicString__hibernate_vm_lock 
 panicString__hibernate_vm_lock: 
 .asciz "_hibernate_vm_lock is not implemented." 

.align 6
 .globl _hibernate_vm_unlock 
 _hibernate_vm_unlock: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__hibernate_vm_unlock 
 blx _Debugger 
 ps_ptr__hibernate_vm_unlock: 
 .long panicString__hibernate_vm_unlock 
 panicString__hibernate_vm_unlock: 
 .asciz "_hibernate_vm_unlock is not implemented." 


.align 6
 .globl _kdb_on 
 _kdb_on: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__kdb_on 
 blx _Debugger 
 ps_ptr__kdb_on: 
 .long panicString__kdb_on 
 panicString__kdb_on: 
 .asciz "_kdb_on is not implemented." 


.align 6
 .globl _kern_dump 
 _kern_dump: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__kern_dump 
 blx _Debugger 
 ps_ptr__kern_dump: 
 .long panicString__kern_dump 
 panicString__kern_dump: 
 .asciz "_kern_dump is not implemented." 


.align 6
 .globl _machine_signal_idle 
 _machine_signal_idle: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__machine_signal_idle 
 blx _Debugger 
 ps_ptr__machine_signal_idle: 
 .long panicString__machine_signal_idle 
 panicString__machine_signal_idle: 
 .asciz "_machine_signal_idle is not implemented." 

.align 6
 .globl _mapping_set_mod 
 _mapping_set_mod: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__mapping_set_mod 
 blx _Debugger 
 ps_ptr__mapping_set_mod: 
 .long panicString__mapping_set_mod 
 panicString__mapping_set_mod: 
 .asciz "_mapping_set_mod is not implemented." 


.align 6
 .globl _ml_nofault_copydeclare_stub 
 _ml_nofault_copydeclare_stub: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__ml_nofault_copydeclare_stub 
 blx _Debugger 
 ps_ptr__ml_nofault_copydeclare_stub: 
 .long panicString__ml_nofault_copydeclare_stub 
 panicString__ml_nofault_copydeclare_stub: 
 .asciz "_ml_nofault_copydeclare_stub is not implemented." 

.align 6
 .globl _ml_stack_remaining 
 _ml_stack_remaining: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__ml_stack_remaining 
 blx _Debugger 
 ps_ptr__ml_stack_remaining: 
 .long panicString__ml_stack_remaining 
 panicString__ml_stack_remaining: 
 .asciz "_ml_stack_remaining is not implemented." 


.align 6
 .globl _pmsControl 
 _pmsControl: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__pmsControl 
 blx _Debugger 
 ps_ptr__pmsControl: 
 .long panicString__pmsControl 
 panicString__pmsControl: 
 .asciz "_pmsControl is not implemented." 


.align 6
 .globl _sdt_invop 
 _sdt_invop: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__sdt_invop 
 blx _Debugger 
 ps_ptr__sdt_invop: 
 .long panicString__sdt_invop 
 panicString__sdt_invop: 
 .asciz "_sdt_invop is not implemented." 


.align 6
 .globl _slave_machine_init 
 _slave_machine_init: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__slave_machine_init 
 blx _Debugger 
 ps_ptr__slave_machine_init: 
 .long panicString__slave_machine_init 
 panicString__slave_machine_init: 
 .asciz "_slave_machine_init is not implemented." 


.align 6
 .globl _tempDTraceTrapHook 
 _tempDTraceTrapHook: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__tempDTraceTrapHook 
 blx _Debugger 
 ps_ptr__tempDTraceTrapHook: 
 .long panicString__tempDTraceTrapHook 
 panicString__tempDTraceTrapHook: 
 .asciz "_tempDTraceTrapHook is not implemented." 


.align 6
 .globl _thread_kdb_return 
 _thread_kdb_return: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__thread_kdb_return 
 blx _Debugger 
 ps_ptr__thread_kdb_return: 
 .long panicString__thread_kdb_return 
 panicString__thread_kdb_return: 
 .asciz "_thread_kdb_return is not implemented." 

.align 6
 .globl _PE_initialize_console 
 _PE_initialize_console: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__PE_initialize_console 
 blx _Debugger 
 ps_ptr__PE_initialize_console: 
 .long panicString__PE_initialize_console 
 panicString__PE_initialize_console: 
 .asciz "_PE_initialize_console is not implemented." 

.align 6
 .globl __vm_commpage_init 
 __vm_commpage_init: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr___vm_commpage_init 
 blx _Debugger 
 ps_ptr___vm_commpage_init: 
 .long panicString___vm_commpage_init 
 panicString___vm_commpage_init: 
 .asciz "__vm_commpage_init is not implemented." 

.align 6
 .globl _act_thread_catt 
 _act_thread_catt: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__act_thread_catt 
 blx _Debugger 
 ps_ptr__act_thread_catt: 
 .long panicString__act_thread_catt 
 panicString__act_thread_catt: 
 .asciz "_act_thread_catt is not implemented." 

.align 6
 .globl _act_thread_csave 
 _act_thread_csave: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__act_thread_csave 
 blx _Debugger 
 ps_ptr__act_thread_csave: 
 .long panicString__act_thread_csave 
 panicString__act_thread_csave: 
 .asciz "_act_thread_csave is not implemented." 

.align 6
 .globl _adler32_vec 
 _adler32_vec: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__adler32_vec 
 blx _Debugger 
 ps_ptr__adler32_vec: 
 .long panicString__adler32_vec 
 panicString__adler32_vec: 
 .asciz "_adler32_vec is not implemented." 

.align 6
 .globl _allow_data_exec 
 _allow_data_exec: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__allow_data_exec 
 blx _Debugger 
 ps_ptr__allow_data_exec: 
 .long panicString__allow_data_exec 
 panicString__allow_data_exec: 
 .asciz "_allow_data_exec is not implemented." 

.align 6
 .globl _allow_stack_exec 
 _allow_stack_exec: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__allow_stack_exec 
 blx _Debugger 
 ps_ptr__allow_stack_exec: 
 .long panicString__allow_stack_exec 
 panicString__allow_stack_exec: 
 .asciz "_allow_stack_exec is not implemented." 

.align 6
 .globl _bzero_phys_nc 
 _bzero_phys_nc: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__bzero_phys_nc 
 blx _Debugger 
 ps_ptr__bzero_phys_nc: 
 .long panicString__bzero_phys_nc 
 panicString__bzero_phys_nc: 
 .asciz "_bzero_phys_nc is not implemented." 

.align 6
 .globl _chudxnu_thread_ast 
 _chudxnu_thread_ast: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__chudxnu_thread_ast 
 blx _Debugger 
 ps_ptr__chudxnu_thread_ast: 
 .long panicString__chudxnu_thread_ast 
 panicString__chudxnu_thread_ast: 
 .asciz "_chudxnu_thread_ast is not implemented." 


.align 6
 .globl _commpage_text_populate 
 _commpage_text_populate: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__commpage_text_populate 
 blx _Debugger 
 ps_ptr__commpage_text_populate: 
 .long panicString__commpage_text_populate 
 panicString__commpage_text_populate: 
 .asciz "_commpage_text_populate is not implemented." 

.align 6
 .globl _consider_machine_collect 
 _consider_machine_collect: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__consider_machine_collect 
 blx _Debugger 
 ps_ptr__consider_machine_collect: 
 .long panicString__consider_machine_collect 
 panicString__consider_machine_collect: 
 .asciz "_consider_machine_collect is not implemented." 

.align 6
 .globl _cpu_control 
 _cpu_control: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__cpu_control 
 blx _Debugger 
 ps_ptr__cpu_control: 
 .long panicString__cpu_control 
 panicString__cpu_control: 
 .asciz "_cpu_control is not implemented." 

.align 6
 .globl _cpu_data_ptr 
 _cpu_data_ptr: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__cpu_data_ptr 
 blx _Debugger 
 ps_ptr__cpu_data_ptr: 
 .long panicString__cpu_data_ptr 
 panicString__cpu_data_ptr: 
 .asciz "_cpu_data_ptr is not implemented." 

.align 6
 .globl _cpu_exit_wait 
 _cpu_exit_wait: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__cpu_exit_wait 
 blx _Debugger 
 ps_ptr__cpu_exit_wait: 
 .long panicString__cpu_exit_wait 
 panicString__cpu_exit_wait: 
 .asciz "_cpu_exit_wait is not implemented." 

.align 6
 .globl _cpu_info 
 _cpu_info: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__cpu_info 
 blx _Debugger 
 ps_ptr__cpu_info: 
 .long panicString__cpu_info 
 panicString__cpu_info: 
 .asciz "_cpu_info is not implemented." 

.align 6
 .globl _cpu_info_count 
 _cpu_info_count: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__cpu_info_count 
 blx _Debugger 
 ps_ptr__cpu_info_count: 
 .long panicString__cpu_info_count 
 panicString__cpu_info_count: 
 .asciz "_cpu_info_count is not implemented." 

.align 6
 .globl _cpu_start 
 _cpu_start: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__cpu_start 
 blx _Debugger 
 ps_ptr__cpu_start: 
 .long panicString__cpu_start 
 panicString__cpu_start: 
 .asciz "_cpu_start is not implemented." 

.align 6
 .globl _cpuid_cpusubtype 
 _cpuid_cpusubtype: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__cpuid_cpusubtype 
 blx _Debugger 
 ps_ptr__cpuid_cpusubtype: 
 .long panicString__cpuid_cpusubtype 
 panicString__cpuid_cpusubtype: 
 .asciz "_cpuid_cpusubtype is not implemented." 

.align 6
 .globl _cpuid_cputype 
 _cpuid_cputype: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__cpuid_cputype 
 blx _Debugger 
 ps_ptr__cpuid_cputype: 
 .long panicString__cpuid_cputype 
 panicString__cpuid_cputype: 
 .asciz "_cpuid_cputype is not implemented." 

.align 6
 .globl _disable_preemption 
 _disable_preemption: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__disable_preemption 
 blx _Debugger 
 ps_ptr__disable_preemption: 
 .long panicString__disable_preemption 
 panicString__disable_preemption: 
 .asciz "_disable_preemption is not implemented." 

.align 6
 .globl _dtrace_modload 
 _dtrace_modload: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__dtrace_modload 
 blx _Debugger 
 ps_ptr__dtrace_modload: 
 .long panicString__dtrace_modload 
 panicString__dtrace_modload: 
 .asciz "_dtrace_modload is not implemented." 

.align 6
 .globl _dtrace_modunload 
 _dtrace_modunload: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__dtrace_modunload 
 blx _Debugger 
 ps_ptr__dtrace_modunload: 
 .long panicString__dtrace_modunload 
 panicString__dtrace_modunload: 
 .asciz "_dtrace_modunload is not implemented." 

.align 6
 .globl _enable_preemption 
 _enable_preemption: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__enable_preemption 
 blx _Debugger 
 ps_ptr__enable_preemption: 
 .long panicString__enable_preemption 
 panicString__enable_preemption: 
 .asciz "_enable_preemption is not implemented." 

.align 6
 .globl _find_user_regs 
 _find_user_regs: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__find_user_regs 
 blx _Debugger 
 ps_ptr__find_user_regs: 
 .long panicString__find_user_regs 
 panicString__find_user_regs: 
 .asciz "_find_user_regs is not implemented." 

.align 6
 .globl _gIOHibernateCurrentHeader 
 _gIOHibernateCurrentHeader: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__gIOHibernateCurrentHeader 
 blx _Debugger 
 ps_ptr__gIOHibernateCurrentHeader: 
 .long panicString__gIOHibernateCurrentHeader 
 panicString__gIOHibernateCurrentHeader: 
 .asciz "_gIOHibernateCurrentHeader is not implemented." 

.align 6
 .globl _gIOHibernateDebugFlags 
 _gIOHibernateDebugFlags: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__gIOHibernateDebugFlags 
 blx _Debugger 
 ps_ptr__gIOHibernateDebugFlags: 
 .long panicString__gIOHibernateDebugFlags 
 panicString__gIOHibernateDebugFlags: 
 .asciz "_gIOHibernateDebugFlags is not implemented." 

.align 6
 .globl _gIOHibernateHandoffPageCount 
 _gIOHibernateHandoffPageCount: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__gIOHibernateHandoffPageCount 
 blx _Debugger 
 ps_ptr__gIOHibernateHandoffPageCount: 
 .long panicString__gIOHibernateHandoffPageCount 
 panicString__gIOHibernateHandoffPageCount: 
 .asciz "_gIOHibernateHandoffPageCount is not implemented." 

.align 6
 .globl _gIOHibernateHandoffPages 
 _gIOHibernateHandoffPages: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__gIOHibernateHandoffPages 
 blx _Debugger 
 ps_ptr__gIOHibernateHandoffPages: 
 .long panicString__gIOHibernateHandoffPages 
 panicString__gIOHibernateHandoffPages: 
 .asciz "_gIOHibernateHandoffPages is not implemented." 

.align 6
 .globl _gIOHibernateState 
 _gIOHibernateState: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__gIOHibernateState 
 blx _Debugger 
 ps_ptr__gIOHibernateState: 
 .long panicString__gIOHibernateState 
 panicString__gIOHibernateState: 
 .asciz "_gIOHibernateState is not implemented." 

.align 6
 .globl _get_useraddr 
 _get_useraddr: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__get_useraddr 
 blx _Debugger 
 ps_ptr__get_useraddr: 
 .long panicString__get_useraddr 
 panicString__get_useraddr: 
 .asciz "_get_useraddr is not implemented." 

.align 6
 .globl _handle_pending_TLB_flushes 
 _handle_pending_TLB_flushes: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__handle_pending_TLB_flushes 
 blx _Debugger 
 ps_ptr__handle_pending_TLB_flushes: 
 .long panicString__handle_pending_TLB_flushes 
 panicString__handle_pending_TLB_flushes: 
 .asciz "_handle_pending_TLB_flushes is not implemented." 

.align 6
 .globl _hibernate_page_bitmap_count 
 _hibernate_page_bitmap_count: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__hibernate_page_bitmap_count 
 blx _Debugger 
 ps_ptr__hibernate_page_bitmap_count: 
 .long panicString__hibernate_page_bitmap_count 
 panicString__hibernate_page_bitmap_count: 
 .asciz "_hibernate_page_bitmap_count is not implemented." 

.align 6
 .globl _hibernate_page_bitmap_pin 
 _hibernate_page_bitmap_pin: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__hibernate_page_bitmap_pin 
 blx _Debugger 
 ps_ptr__hibernate_page_bitmap_pin: 
 .long panicString__hibernate_page_bitmap_pin 
 panicString__hibernate_page_bitmap_pin: 
 .asciz "_hibernate_page_bitmap_pin is not implemented." 

.align 6
 .globl _hibernate_page_bitset 
 _hibernate_page_bitset: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__hibernate_page_bitset 
 blx _Debugger 
 ps_ptr__hibernate_page_bitset: 
 .long panicString__hibernate_page_bitset 
 panicString__hibernate_page_bitset: 
 .asciz "_hibernate_page_bitset is not implemented." 

.align 6
 .globl _hibernate_page_bittst 
 _hibernate_page_bittst: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__hibernate_page_bittst 
 blx _Debugger 
 ps_ptr__hibernate_page_bittst: 
 .long panicString__hibernate_page_bittst 
 panicString__hibernate_page_bittst: 
 .asciz "_hibernate_page_bittst is not implemented." 

.align 6
 .globl _hibernate_sum_page 
 _hibernate_sum_page: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__hibernate_sum_page 
 blx _Debugger 
 ps_ptr__hibernate_sum_page: 
 .long panicString__hibernate_sum_page 
 panicString__hibernate_sum_page: 
 .asciz "_hibernate_sum_page is not implemented." 

.align 6
 .globl _hw_lock_byte_init 
 _hw_lock_byte_init: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__hw_lock_byte_init 
 blx _Debugger 
 ps_ptr__hw_lock_byte_init: 
 .long panicString__hw_lock_byte_init 
 panicString__hw_lock_byte_init: 
 .asciz "_hw_lock_byte_init is not implemented." 

.align 6
 .globl _hw_lock_byte_lock 
 _hw_lock_byte_lock: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__hw_lock_byte_lock 
 blx _Debugger 
 ps_ptr__hw_lock_byte_lock: 
 .long panicString__hw_lock_byte_lock 
 panicString__hw_lock_byte_lock: 
 .asciz "_hw_lock_byte_lock is not implemented." 

.align 6
 .globl _hw_lock_byte_unlock 
 _hw_lock_byte_unlock: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__hw_lock_byte_unlock 
 blx _Debugger 
 ps_ptr__hw_lock_byte_unlock: 
 .long panicString__hw_lock_byte_unlock 
 panicString__hw_lock_byte_unlock: 
 .asciz "_hw_lock_byte_unlock is not implemented." 

.align 6
 .globl _kdp_machine_get_breakinsn 
 _kdp_machine_get_breakinsn: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__kdp_machine_get_breakinsn 
 blx _Debugger 
 ps_ptr__kdp_machine_get_breakinsn: 
 .long panicString__kdp_machine_get_breakinsn 
 panicString__kdp_machine_get_breakinsn: 
 .asciz "_kdp_machine_get_breakinsn is not implemented." 

.align 6
 .globl _lck_rw_grab_shared 
 _lck_rw_grab_shared: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__lck_rw_grab_shared 
 blx _Debugger 
 ps_ptr__lck_rw_grab_shared: 
 .long panicString__lck_rw_grab_shared 
 panicString__lck_rw_grab_shared: 
 .asciz "_lck_rw_grab_shared is not implemented." 

.align 6
 .globl _lockstat_probe 
 _lockstat_probe: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__lockstat_probe 
 blx _Debugger 
 ps_ptr__lockstat_probe: 
 .long panicString__lockstat_probe 
 panicString__lockstat_probe: 
 .asciz "_lockstat_probe is not implemented." 

.align 6
 .globl _lockstat_probemap 
 _lockstat_probemap: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__lockstat_probemap 
 blx _Debugger 
 ps_ptr__lockstat_probemap: 
 .long panicString__lockstat_probemap 
 panicString__lockstat_probemap: 
 .asciz "_lockstat_probemap is not implemented." 

.align 6
 .globl _Halt_system
 _Halt_system: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__Halt_system
 blx _Debugger 
 ps_ptr__Halt_system: 
 .long panicString__Halt_system 
 panicString__Halt_system: 
 .asciz "_Halt_system is not implemented." 

.align 6
 .globl _machine_callstack 
 _machine_callstack: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__machine_callstack 
 blx _Debugger 
 ps_ptr__machine_callstack: 
 .long panicString__machine_callstack 
 panicString__machine_callstack: 
 .asciz "_machine_callstack is not implemented." 

.align 6
 .globl _machine_exception 
 _machine_exception: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__machine_exception 
 blx _Debugger 
 ps_ptr__machine_exception: 
 .long panicString__machine_exception 
 panicString__machine_exception: 
 .asciz "_machine_exception is not implemented." 

.align 6
 .globl _machine_processor_shutdown 
 _machine_processor_shutdown: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__machine_processor_shutdown 
 blx _Debugger 
 ps_ptr__machine_processor_shutdown: 
 .long panicString__machine_processor_shutdown 
 panicString__machine_processor_shutdown: 
 .asciz "_machine_processor_shutdown is not implemented." 

.align 6
 .globl _machine_task_get_state 
 _machine_task_get_state: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__machine_task_get_state 
 blx _Debugger 
 ps_ptr__machine_task_get_state: 
 .long panicString__machine_task_get_state 
 panicString__machine_task_get_state: 
 .asciz "_machine_task_get_state is not implemented." 

.align 6
 .globl _machine_task_set_state 
 _machine_task_set_state: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__machine_task_set_state 
 blx _Debugger 
 ps_ptr__machine_task_set_state: 
 .long panicString__machine_task_set_state 
 panicString__machine_task_set_state: 
 .asciz "_machine_task_set_state is not implemented." 

.align 6
 .globl _machine_thread_dup 
 _machine_thread_dup: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__machine_thread_dup 
 blx _Debugger 
 ps_ptr__machine_thread_dup: 
 .long panicString__machine_thread_dup 
 panicString__machine_thread_dup: 
 .asciz "_machine_thread_dup is not implemented." 

.align 6
 .globl _machine_thread_get_state 
 _machine_thread_get_state: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__machine_thread_get_state 
 blx _Debugger 
 ps_ptr__machine_thread_get_state: 
 .long panicString__machine_thread_get_state 
 panicString__machine_thread_get_state: 
 .asciz "_machine_thread_get_state is not implemented." 

.align 6
 .globl _machine_timeout_suspended 
 _machine_timeout_suspended: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__machine_timeout_suspended 
 blx _Debugger 
 ps_ptr__machine_timeout_suspended: 
 .long panicString__machine_timeout_suspended 
 panicString__machine_timeout_suspended: 
 .asciz "_machine_timeout_suspended is not implemented." 

.align 6
 .globl _machine_trace_thread 
 _machine_trace_thread: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__machine_trace_thread 
 blx _Debugger 
 ps_ptr__machine_trace_thread: 
 .long panicString__machine_trace_thread 
 panicString__machine_trace_thread: 
 .asciz "_machine_trace_thread is not implemented." 

.align 6
 .globl _machine_trace_thread64 
 _machine_trace_thread64: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__machine_trace_thread64 
 blx _Debugger 
 ps_ptr__machine_trace_thread64: 
 .long panicString__machine_trace_thread64 
 panicString__machine_trace_thread64: 
 .asciz "_machine_trace_thread64 is not implemented." 

.align 6
 .globl _ml_at_interrupt_context 
 _ml_at_interrupt_context: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__ml_at_interrupt_context 
 blx _Debugger 
 ps_ptr__ml_at_interrupt_context: 
 .long panicString__ml_at_interrupt_context 
 panicString__ml_at_interrupt_context: 
 .asciz "_ml_at_interrupt_context is not implemented." 

.align 6
 .globl _ml_cpu_get_info 
 _ml_cpu_get_info: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__ml_cpu_get_info 
 blx _Debugger 
 ps_ptr__ml_cpu_get_info: 
 .long panicString__ml_cpu_get_info 
 panicString__ml_cpu_get_info: 
 .asciz "_ml_cpu_get_info is not implemented." 

.align 6
 .globl _ml_delay_should_spin 
 _ml_delay_should_spin: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__ml_delay_should_spin 
 blx _Debugger 
 ps_ptr__ml_delay_should_spin: 
 .long panicString__ml_delay_should_spin 
 panicString__ml_delay_should_spin: 
 .asciz "_ml_delay_should_spin is not implemented." 

.align 6
 .globl _ml_interrupt_prewarm 
 _ml_interrupt_prewarm: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__ml_interrupt_prewarm 
 blx _Debugger 
 ps_ptr__ml_interrupt_prewarm: 
 .long panicString__ml_interrupt_prewarm 
 panicString__ml_interrupt_prewarm: 
 .asciz "_ml_interrupt_prewarm is not implemented." 

.align 6
 .globl _ml_thread_is64bit 
 _ml_thread_is64bit: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__ml_thread_is64bit 
 blx _Debugger 
 ps_ptr__ml_thread_is64bit: 
 .long panicString__ml_thread_is64bit 
 panicString__ml_thread_is64bit: 
 .asciz "_ml_thread_is64bit is not implemented." 

.align 6
 .globl _nx_enabled 
 _nx_enabled: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__nx_enabled 
 blx _Debugger 
 ps_ptr__nx_enabled: 
 .long panicString__nx_enabled 
 panicString__nx_enabled: 
 .asciz "_nx_enabled is not implemented." 

.align 6
 .globl _panicDialogDesired 
 _panicDialogDesired: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__panicDialogDesired 
 blx _Debugger 
 ps_ptr__panicDialogDesired: 
 .long panicString__panicDialogDesired 
 panicString__panicDialogDesired: 
 .asciz "_panicDialogDesired is not implemented." 

.align 6
 .globl _panic_display_pal_info 
 _panic_display_pal_info: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__panic_display_pal_info 
 blx _Debugger 
 ps_ptr__panic_display_pal_info: 
 .long panicString__panic_display_pal_info 
 panicString__panic_display_pal_info: 
 .asciz "_panic_display_pal_info is not implemented." 

.align 6
 .globl _pmap_adjust_unnest_parameters 
 _pmap_adjust_unnest_parameters: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__pmap_adjust_unnest_parameters 
 blx _Debugger 
 ps_ptr__pmap_adjust_unnest_parameters: 
 .long panicString__pmap_adjust_unnest_parameters 
 panicString__pmap_adjust_unnest_parameters: 
 .asciz "_pmap_adjust_unnest_parameters is not implemented." 

.align 6
 .globl _pmap_attribute 
 _pmap_attribute: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__pmap_attribute 
 blx _Debugger 
 ps_ptr__pmap_attribute: 
 .long panicString__pmap_attribute 
 panicString__pmap_attribute: 
 .asciz "_pmap_attribute is not implemented." 

.align 6
 .globl _pmap_attribute_cache_sync 
 _pmap_attribute_cache_sync: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__pmap_attribute_cache_sync 
 blx _Debugger 
 ps_ptr__pmap_attribute_cache_sync: 
 .long panicString__pmap_attribute_cache_sync 
 panicString__pmap_attribute_cache_sync: 
 .asciz "_pmap_attribute_cache_sync is not implemented." 

.align 6
 .globl _pmap_cache_attributes 
 _pmap_cache_attributes: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__pmap_cache_attributes 
 blx _Debugger 
 ps_ptr__pmap_cache_attributes: 
 .long panicString__pmap_cache_attributes 
 panicString__pmap_cache_attributes: 
 .asciz "_pmap_cache_attributes is not implemented." 

.align 6
 .globl _pmap_clear_modify 
 _pmap_clear_modify: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__pmap_clear_modify 
 blx _Debugger 
 ps_ptr__pmap_clear_modify: 
 .long panicString__pmap_clear_modify 
 panicString__pmap_clear_modify: 
 .asciz "_pmap_clear_modify is not implemented." 

.align 6
 .globl _pmap_clear_reference 
 _pmap_clear_reference: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__pmap_clear_reference 
 blx _Debugger 
 ps_ptr__pmap_clear_reference: 
 .long panicString__pmap_clear_reference 
 panicString__pmap_clear_reference: 
 .asciz "_pmap_clear_reference is not implemented." 

.align 6
 .globl _pmap_copy 
 _pmap_copy: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__pmap_copy 
 blx _Debugger 
 ps_ptr__pmap_copy: 
 .long panicString__pmap_copy 
 panicString__pmap_copy: 
 .asciz "_pmap_copy is not implemented." 

.align 6
 .globl _pmap_copy_part_page 
 _pmap_copy_part_page: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__pmap_copy_part_page 
 blx _Debugger 
 ps_ptr__pmap_copy_part_page: 
 .long panicString__pmap_copy_part_page 
 panicString__pmap_copy_part_page: 
 .asciz "_pmap_copy_part_page is not implemented." 

.align 6
 .globl _pmap_disable_NX 
 _pmap_disable_NX: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__pmap_disable_NX 
 blx _Debugger 
 ps_ptr__pmap_disable_NX: 
 .long panicString__pmap_disable_NX 
 panicString__pmap_disable_NX: 
 .asciz "_pmap_disable_NX is not implemented." 

.align 6
 .globl _pmap_get_refmod 
 _pmap_get_refmod: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__pmap_get_refmod 
 blx _Debugger 
 ps_ptr__pmap_get_refmod: 
 .long panicString__pmap_get_refmod 
 panicString__pmap_get_refmod: 
 .asciz "_pmap_get_refmod is not implemented." 

.align 6
 .globl _pmap_is_modified 
 _pmap_is_modified: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__pmap_is_modified 
 blx _Debugger 
 ps_ptr__pmap_is_modified: 
 .long panicString__pmap_is_modified 
 panicString__pmap_is_modified: 
 .asciz "_pmap_is_modified is not implemented." 

.align 6
 .globl _pmap_is_referenced 
 _pmap_is_referenced: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__pmap_is_referenced 
 blx _Debugger 
 ps_ptr__pmap_is_referenced: 
 .long panicString__pmap_is_referenced 
 panicString__pmap_is_referenced: 
 .asciz "_pmap_is_referenced is not implemented." 

.align 6
 .globl _pmap_map_block 
 _pmap_map_block: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__pmap_map_block 
 blx _Debugger 
 ps_ptr__pmap_map_block: 
 .long panicString__pmap_map_block 
 panicString__pmap_map_block: 
 .asciz "_pmap_map_block is not implemented." 

.align 6
 .globl _pmap_mem_regions 
 _pmap_mem_regions: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__pmap_mem_regions 
 blx _Debugger 
 ps_ptr__pmap_mem_regions: 
 .long panicString__pmap_mem_regions 
 panicString__pmap_mem_regions: 
 .asciz "_pmap_mem_regions is not implemented." 

.align 6
 .globl _pmap_mem_regions_count 
 _pmap_mem_regions_count: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__pmap_mem_regions_count 
 blx _Debugger 
 ps_ptr__pmap_mem_regions_count: 
 .long panicString__pmap_mem_regions_count 
 panicString__pmap_mem_regions_count: 
 .asciz "_pmap_mem_regions_count is not implemented." 

.align 6
 .globl _pmap_resident_max 
 _pmap_resident_max: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__pmap_resident_max 
 blx _Debugger 
 ps_ptr__pmap_resident_max: 
 .long panicString__pmap_resident_max 
 panicString__pmap_resident_max: 
 .asciz "_pmap_resident_max is not implemented." 

.align 6
 .globl _pmap_set_cache_attributes 
 _pmap_set_cache_attributes: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__pmap_set_cache_attributes 
 blx _Debugger 
 ps_ptr__pmap_set_cache_attributes: 
 .long panicString__pmap_set_cache_attributes 
 panicString__pmap_set_cache_attributes: 
 .asciz "_pmap_set_cache_attributes is not implemented." 

.align 6
 .globl _pmap_sync_page_attributes_phys 
 _pmap_sync_page_attributes_phys: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__pmap_sync_page_attributes_phys 
 blx _Debugger 
 ps_ptr__pmap_sync_page_attributes_phys: 
 .long panicString__pmap_sync_page_attributes_phys 
 panicString__pmap_sync_page_attributes_phys: 
 .asciz "_pmap_sync_page_attributes_phys is not implemented." 

.align 6
 .globl _pmap_unnest 
 _pmap_unnest: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__pmap_unnest 
 blx _Debugger 
 ps_ptr__pmap_unnest: 
 .long panicString__pmap_unnest 
 panicString__pmap_unnest: 
 .asciz "_pmap_unnest is not implemented." 

.align 6
 .globl _pt_fake_zone_info 
 _pt_fake_zone_info: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__pt_fake_zone_info 
 blx _Debugger 
 ps_ptr__pt_fake_zone_info: 
 .long panicString__pt_fake_zone_info 
 panicString__pt_fake_zone_info: 
 .asciz "_pt_fake_zone_info is not implemented." 

.align 6
 .globl _pt_fake_zone_init 
 _pt_fake_zone_init: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__pt_fake_zone_init 
 blx _Debugger 
 ps_ptr__pt_fake_zone_init: 
 .long panicString__pt_fake_zone_init 
 panicString__pt_fake_zone_init: 
 .asciz "_pt_fake_zone_init is not implemented." 

.align 6
 .globl _real_ncpus 
 _real_ncpus: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__real_ncpus 
 blx _Debugger 
 ps_ptr__real_ncpus: 
 .long panicString__real_ncpus 
 panicString__real_ncpus: 
 .asciz "_real_ncpus is not implemented." 

.align 6
 .globl _save_kdebug_enable 
 _save_kdebug_enable: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__save_kdebug_enable 
 blx _Debugger 
 ps_ptr__save_kdebug_enable: 
 .long panicString__save_kdebug_enable 
 panicString__save_kdebug_enable: 
 .asciz "_save_kdebug_enable is not implemented." 

.align 6
 .globl _saved_state64 
 _saved_state64: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__saved_state64 
 blx _Debugger 
 ps_ptr__saved_state64: 
 .long panicString__saved_state64 
 panicString__saved_state64: 
 .asciz "_saved_state64 is not implemented." 

.align 6
 .globl _segHIBB 
 _segHIBB: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__segHIBB 
 blx _Debugger 
 ps_ptr__segHIBB: 
 .long panicString__segHIBB 
 panicString__segHIBB: 
 .asciz "_segHIBB is not implemented." 

.align 6
 .globl _segPRELINKB 
 _segPRELINKB: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__segPRELINKB 
 blx _Debugger 
 ps_ptr__segPRELINKB: 
 .long panicString__segPRELINKB 
 panicString__segPRELINKB: 
 .asciz "_segPRELINKB is not implemented." 

.align 6
 .globl _segSizeHIB 
 _segSizeHIB: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__segSizeHIB 
 blx _Debugger 
 ps_ptr__segSizeHIB: 
 .long panicString__segSizeHIB 
 panicString__segSizeHIB: 
 .asciz "_segSizeHIB is not implemented." 

.align 6
 .globl _segSizePRELINK 
 _segSizePRELINK: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__segSizePRELINK 
 blx _Debugger 
 ps_ptr__segSizePRELINK: 
 .long panicString__segSizePRELINK 
 panicString__segSizePRELINK: 
 .asciz "_segSizePRELINK is not implemented." 

.align 6
 .globl _sendsig 
 _sendsig: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__sendsig 
 blx _Debugger 
 ps_ptr__sendsig: 
 .long panicString__sendsig 
 panicString__sendsig: 
 .asciz "_sendsig is not implemented." 

.align 6
 .globl _serial_getc 
 _serial_getc: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__serial_getc 
 blx _Debugger 
 ps_ptr__serial_getc: 
 .long panicString__serial_getc 
 panicString__serial_getc: 
 .asciz "_serial_getc is not implemented." 

.align 6
 .globl _serial_init 
 _serial_init: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__serial_init 
 blx _Debugger 
 ps_ptr__serial_init: 
 .long panicString__serial_init 
 panicString__serial_init: 
 .asciz "_serial_init is not implemented." 

.align 6
 .globl _serial_putc 
 _serial_putc: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__serial_putc 
 blx _Debugger 
 ps_ptr__serial_putc: 
 .long panicString__serial_putc 
 panicString__serial_putc: 
 .asciz "_serial_putc is not implemented." 

.align 6
 .globl _sigreturn 
 _sigreturn: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__sigreturn 
 blx _Debugger 
 ps_ptr__sigreturn: 
 .long panicString__sigreturn 
 panicString__sigreturn: 
 .asciz "_sigreturn is not implemented." 

.align 6
 .globl _thread_set_parent 
 _thread_set_parent: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__thread_set_parent 
 blx _Debugger 
 ps_ptr__thread_set_parent: 
 .long panicString__thread_set_parent 
 panicString__thread_set_parent: 
 .asciz "_thread_set_parent is not implemented." 

.align 6
 .globl _thread_set_child 
 _thread_set_child: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__thread_set_child
 blx _Debugger 
 ps_ptr__thread_set_child: 
 .long panicString__thread_set_child
 panicString__thread_set_child: 
 .asciz "_thread_set_child is not implemented." 

.align 6
 .globl _thread_set_wq_state32 
 _thread_set_wq_state32: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__thread_set_wq_state32 
 blx _Debugger 
 ps_ptr__thread_set_wq_state32: 
 .long panicString__thread_set_wq_state32 
 panicString__thread_set_wq_state32: 
 .asciz "_thread_set_wq_state32 is not implemented." 

.align 6
 .globl _thread_setsinglestep 
 _thread_setsinglestep: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__thread_setsinglestep 
 blx _Debugger 
 ps_ptr__thread_setsinglestep: 
 .long panicString__thread_setsinglestep 
 panicString__thread_setsinglestep: 
 .asciz "_thread_setsinglestep is not implemented." 

.align 6
 .globl _thread_syscall_return 
 _thread_syscall_return: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__thread_syscall_return 
 blx _Debugger 
 ps_ptr__thread_syscall_return: 
 .long panicString__thread_syscall_return 
 panicString__thread_syscall_return: 
 .asciz "_thread_syscall_return is not implemented." 

.align 6
 .globl _thread_userstackdefault 
 _thread_userstackdefault: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__thread_userstackdefault 
 blx _Debugger 
 ps_ptr__thread_userstackdefault: 
 .long panicString__thread_userstackdefault 
 panicString__thread_userstackdefault: 
 .asciz "_thread_userstackdefault is not implemented." 

.align 6
 .globl _unix_syscall_return 
 _unix_syscall_return: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__unix_syscall_return 
 blx _Debugger 
 ps_ptr__unix_syscall_return: 
 .long panicString__unix_syscall_return 
 panicString__unix_syscall_return: 
 .asciz "_unix_syscall_return is not implemented." 

.align 6
 .globl _Call_continuation 
 _Call_continuation: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__Call_continuation 
 blx _panic 
 ps_ptr__Call_continuation: 
 .long panicString__Call_continuation 
 panicString__Call_continuation: 
 .asciz "_Call_continuation is not implemented." 

.align 6
 .globl _OSAddAtomic64 
 _OSAddAtomic64: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__OSAddAtomic64 
 blx _panic 
 ps_ptr__OSAddAtomic64: 
 .long panicString__OSAddAtomic64 
 panicString__OSAddAtomic64: 
 .asciz "_OSAddAtomic64 is not implemented." 

.align 6
 .globl _OSAddAtomicLong 
 _OSAddAtomicLong: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__OSAddAtomicLong 
 blx _panic 
 ps_ptr__OSAddAtomicLong: 
 .long panicString__OSAddAtomicLong 
 panicString__OSAddAtomicLong: 
 .asciz "_OSAddAtomicLong is not implemented." 

.align 6
 .globl _OSCompareAndSwap64 
 _OSCompareAndSwap64: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__OSCompareAndSwap64 
 blx _panic 
 ps_ptr__OSCompareAndSwap64: 
 .long panicString__OSCompareAndSwap64 
 panicString__OSCompareAndSwap64: 
 .asciz "_OSCompareAndSwap64 is not implemented." 

.align 6
 .globl _OSCompareAndSwapPtr 
 _OSCompareAndSwapPtr: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__OSCompareAndSwapPtr 
 blx _panic 
 ps_ptr__OSCompareAndSwapPtr: 
 .long panicString__OSCompareAndSwapPtr 
 panicString__OSCompareAndSwapPtr: 
 .asciz "_OSCompareAndSwapPtr is not implemented." 

.align 6
 .globl _PE_init_SocSupport_stub 
 _PE_init_SocSupport_stub: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__PE_init_SocSupport_stub 
 blx _panic 
 ps_ptr__PE_init_SocSupport_stub: 
 .long panicString__PE_init_SocSupport_stub 
 panicString__PE_init_SocSupport_stub: 
 .asciz "_PE_init_SocSupport_stub is not implemented." 

.align 6
 .globl _Switch_context 
 _Switch_context: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__Switch_context 
 blx _panic 
 ps_ptr__Switch_context: 
 .long panicString__Switch_context 
 panicString__Switch_context: 
 .asciz "_Switch_context is not implemented." 

.align 6
 .globl __start 
 __start: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr___start 
 blx _panic 
 ps_ptr___start: 
 .long panicString___start 
 panicString___start: 
 .asciz "__start is not implemented." 

.align 6
 .globl _copyin 
 _copyin: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__copyin 
 blx _panic 
 ps_ptr__copyin: 
 .long panicString__copyin 
 panicString__copyin: 
 .asciz "_copyin is not implemented." 

.align 6
 .globl _copyinmsg 
 _copyinmsg: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__copyinmsg 
 blx _panic 
 ps_ptr__copyinmsg: 
 .long panicString__copyinmsg 
 panicString__copyinmsg: 
 .asciz "_copyinmsg is not implemented." 

.align 6
 .globl _copyinstr 
 _copyinstr: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__copyinstr 
 blx _panic 
 ps_ptr__copyinstr: 
 .long panicString__copyinstr 
 panicString__copyinstr: 
 .asciz "_copyinstr is not implemented." 

.align 6
 .globl _copyout 
 _copyout: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__copyout 
 blx _panic 
 ps_ptr__copyout: 
 .long panicString__copyout 
 panicString__copyout: 
 .asciz "_copyout is not implemented." 

.align 6
 .globl _copyoutmsg 
 _copyoutmsg: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__copyoutmsg 
 blx _panic 
 ps_ptr__copyoutmsg: 
 .long panicString__copyoutmsg 
 panicString__copyoutmsg: 
 .asciz "_copyoutmsg is not implemented." 

.align 6
 .globl _copyoutstr 
 _copyoutstr: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__copyoutstr 
 blx _panic 
 ps_ptr__copyoutstr: 
 .long panicString__copyoutstr 
 panicString__copyoutstr: 
 .asciz "_copyoutstr is not implemented." 

.align 6
 .globl _copypv 
 _copypv: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__copypv 
 blx _panic 
 ps_ptr__copypv: 
 .long panicString__copypv 
 panicString__copypv: 
 .asciz "_copypv is not implemented." 

.align 6
 .globl _copystr 
 _copystr: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__copystr 
 blx _panic 
 ps_ptr__copystr: 
 .long panicString__copystr 
 panicString__copystr: 
 .asciz "_copystr is not implemented." 

.align 6
 .globl _flush_dcache 
 _flush_dcache: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__flush_dcache 
 blx _panic 
 ps_ptr__flush_dcache: 
 .long panicString__flush_dcache 
 panicString__flush_dcache: 
 .asciz "_flush_dcache is not implemented." 

.align 6
 .globl _invalidate_icache 
 _invalidate_icache: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__invalidate_icache 
 blx _panic 
 ps_ptr__invalidate_icache: 
 .long panicString__invalidate_icache 
 panicString__invalidate_icache: 
 .asciz "_invalidate_icache is not implemented." 

.align 6
 .globl _lck_mtx_assert 
 _lck_mtx_assert: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__lck_mtx_assert 
 blx _panic 
 ps_ptr__lck_mtx_assert: 
 .long panicString__lck_mtx_assert 
 panicString__lck_mtx_assert: 
 .asciz "_lck_mtx_assert is not implemented." 

.align 6
 .globl _lck_mtx_lock 
 _lck_mtx_lock: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__lck_mtx_lock 
 blx _panic 
 ps_ptr__lck_mtx_lock: 
 .long panicString__lck_mtx_lock 
 panicString__lck_mtx_lock: 
 .asciz "_lck_mtx_lock is not implemented." 

.align 6
 .globl _lck_mtx_try_lock 
 _lck_mtx_try_lock: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__lck_mtx_try_lock 
 blx _panic 
 ps_ptr__lck_mtx_try_lock: 
 .long panicString__lck_mtx_try_lock 
 panicString__lck_mtx_try_lock: 
 .asciz "_lck_mtx_try_lock is not implemented." 

.align 6
 .globl _lck_rw_done 
 _lck_rw_done: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__lck_rw_done 
 blx _panic 
 ps_ptr__lck_rw_done: 
 .long panicString__lck_rw_done 
 panicString__lck_rw_done: 
 .asciz "_lck_rw_done is not implemented." 

.align 6
 .globl _lck_rw_lock_exclusive 
 _lck_rw_lock_exclusive: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__lck_rw_lock_exclusive 
 blx _panic 
 ps_ptr__lck_rw_lock_exclusive: 
 .long panicString__lck_rw_lock_exclusive 
 panicString__lck_rw_lock_exclusive: 
 .asciz "_lck_rw_lock_exclusive is not implemented." 

.align 6
 .globl _lck_rw_lock_exclusive_to_shared 
 _lck_rw_lock_exclusive_to_shared: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__lck_rw_lock_exclusive_to_shared 
 blx _panic 
 ps_ptr__lck_rw_lock_exclusive_to_shared: 
 .long panicString__lck_rw_lock_exclusive_to_shared 
 panicString__lck_rw_lock_exclusive_to_shared: 
 .asciz "_lck_rw_lock_exclusive_to_shared is not implemented." 

.align 6
 .globl _lck_rw_try_lock_exclusive 
 _lck_rw_try_lock_exclusive: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__lck_rw_try_lock_exclusive 
 blx _panic 
 ps_ptr__lck_rw_try_lock_exclusive: 
 .long panicString__lck_rw_try_lock_exclusive 
 panicString__lck_rw_try_lock_exclusive: 
 .asciz "_lck_rw_try_lock_exclusive is not implemented." 

.align 6
 .globl _lck_rw_try_lock_shared 
 _lck_rw_try_lock_shared: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__lck_rw_try_lock_shared 
 blx _panic 
 ps_ptr__lck_rw_try_lock_shared: 
 .long panicString__lck_rw_try_lock_shared 
 panicString__lck_rw_try_lock_shared: 
 .asciz "_lck_rw_try_lock_shared is not implemented." 

.align 6
 .globl _lock_done 
 _lock_done: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__lock_done 
 blx _panic 
 ps_ptr__lock_done: 
 .long panicString__lock_done 
 panicString__lock_done: 
 .asciz "_lock_done is not implemented." 

.align 6
 .globl _lock_write 
 _lock_write: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__lock_write 
 blx _panic 
 ps_ptr__lock_write: 
 .long panicString__lock_write 
 panicString__lock_write: 
 .asciz "_lock_write is not implemented." 

.align 6
 .globl _lock_write_to_read 
 _lock_write_to_read: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__lock_write_to_read 
 blx _panic 
 ps_ptr__lock_write_to_read: 
 .long panicString__lock_write_to_read 
 panicString__lock_write_to_read: 
 .asciz "_lock_write_to_read is not implemented." 

.align 6
 .globl _machine_load_context 
 _machine_load_context: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__machine_load_context 
 blx _panic 
 ps_ptr__machine_load_context: 
 .long panicString__machine_load_context 
 panicString__machine_load_context: 
 .asciz "_machine_load_context is not implemented." 

.align 6
 .globl _machine_thread_set_state 
 _machine_thread_set_state: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__machine_thread_set_state 
 blx _panic 
 ps_ptr__machine_thread_set_state: 
 .long panicString__machine_thread_set_state 
 panicString__machine_thread_set_state: 
 .asciz "_machine_thread_set_state is not implemented." 

.align 6
 .globl _ml_set_interrupts_enabled 
 _ml_set_interrupts_enabled: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__ml_set_interrupts_enabled 
 blx _panic 
 ps_ptr__ml_set_interrupts_enabled: 
 .long panicString__ml_set_interrupts_enabled 
 panicString__ml_set_interrupts_enabled: 
 .asciz "_ml_set_interrupts_enabled is not implemented." 

.align 6
 .globl _pmap_change_wiring 
 _pmap_change_wiring: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__pmap_change_wiring 
 blx _panic 
 ps_ptr__pmap_change_wiring: 
 .long panicString__pmap_change_wiring 
 panicString__pmap_change_wiring: 
 .asciz "_pmap_change_wiring is not implemented." 

.align 6
 .globl _pmap_create 
 _pmap_create: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__pmap_create 
 blx _panic 
 ps_ptr__pmap_create: 
 .long panicString__pmap_create 
 panicString__pmap_create: 
 .asciz "_pmap_create is not implemented." 

.align 6
 .globl _pmap_create_sharedpage 
 _pmap_create_sharedpage: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__pmap_create_sharedpage 
 blx _panic 
 ps_ptr__pmap_create_sharedpage: 
 .long panicString__pmap_create_sharedpage 
 panicString__pmap_create_sharedpage: 
 .asciz "_pmap_create_sharedpage is not implemented." 

.align 6
 .globl _pmap_enter 
 _pmap_enter: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__pmap_enter 
 blx _panic 
 ps_ptr__pmap_enter: 
 .long panicString__pmap_enter 
 panicString__pmap_enter: 
 .asciz "_pmap_enter is not implemented." 

.align 6
 .globl _pmap_enter_options 
 _pmap_enter_options: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__pmap_enter_options 
 blx _panic 
 ps_ptr__pmap_enter_options: 
 .long panicString__pmap_enter_options 
 panicString__pmap_enter_options: 
 .asciz "_pmap_enter_options is not implemented." 

.align 6
 .globl _pmap_extract 
 _pmap_extract: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__pmap_extract 
 blx _panic 
 ps_ptr__pmap_extract: 
 .long panicString__pmap_extract 
 panicString__pmap_extract: 
 .asciz "_pmap_extract is not implemented." 

.align 6
 .globl _pmap_find_phys 
 _pmap_find_phys: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__pmap_find_phys 
 blx _panic 
 ps_ptr__pmap_find_phys: 
 .long panicString__pmap_find_phys 
 panicString__pmap_find_phys: 
 .asciz "_pmap_find_phys is not implemented." 

.align 6
 .globl _pmap_map_bd 
 _pmap_map_bd: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__pmap_map_bd 
 blx _panic 
 ps_ptr__pmap_map_bd: 
 .long panicString__pmap_map_bd 
 panicString__pmap_map_bd: 
 .asciz "_pmap_map_bd is not implemented." 

.align 6
 .globl _pmap_page_protect 
 _pmap_page_protect: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__pmap_page_protect 
 blx _panic 
 ps_ptr__pmap_page_protect: 
 .long panicString__pmap_page_protect 
 panicString__pmap_page_protect: 
 .asciz "_pmap_page_protect is not implemented." 

.align 6
 .globl _pmap_pre_expand 
 _pmap_pre_expand: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__pmap_pre_expand 
 blx _panic 
 ps_ptr__pmap_pre_expand: 
 .long panicString__pmap_pre_expand 
 panicString__pmap_pre_expand: 
 .asciz "_pmap_pre_expand is not implemented." 

.align 6
 .globl _pmap_switch 
 _pmap_switch: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__pmap_switch 
 blx _panic 
 ps_ptr__pmap_switch: 
 .long panicString__pmap_switch 
 panicString__pmap_switch: 
 .asciz "_pmap_switch is not implemented." 

.align 6
 .globl _ram_begin 
 _ram_begin: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__ram_begin 
 blx _panic 
 ps_ptr__ram_begin: 
 .long panicString__ram_begin 
 panicString__ram_begin: 
 .asciz "_ram_begin is not implemented." 

.align 6
 .globl _thread_adjuserstack 
 _thread_adjuserstack: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__thread_adjuserstack 
 blx _panic 
 ps_ptr__thread_adjuserstack: 
 .long panicString__thread_adjuserstack 
 panicString__thread_adjuserstack: 
 .asciz "_thread_adjuserstack is not implemented." 

.align 6
 .globl _thread_bootstrap_return 
 _thread_bootstrap_return: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__thread_bootstrap_return 
 blx _panic 
 ps_ptr__thread_bootstrap_return: 
 .long panicString__thread_bootstrap_return 
 panicString__thread_bootstrap_return: 
 .asciz "_thread_bootstrap_return is not implemented." 

.align 6
 .globl _thread_entrypoint 
 _thread_entrypoint: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__thread_entrypoint 
 blx _panic 
 ps_ptr__thread_entrypoint: 
 .long panicString__thread_entrypoint 
 panicString__thread_entrypoint: 
 .asciz "_thread_entrypoint is not implemented." 

.align 6
 .globl _thread_exception_return 
 _thread_exception_return: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__thread_exception_return 
 blx _panic 
 ps_ptr__thread_exception_return: 
 .long panicString__thread_exception_return 
 panicString__thread_exception_return: 
 .asciz "_thread_exception_return is not implemented." 

.align 6
 .globl _thread_setentrypoint 
 _thread_setentrypoint: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__thread_setentrypoint 
 blx _panic 
 ps_ptr__thread_setentrypoint: 
 .long panicString__thread_setentrypoint 
 panicString__thread_setentrypoint: 
 .asciz "_thread_setentrypoint is not implemented." 

.align 6
 .globl _thread_setuserstack 
 _thread_setuserstack: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__thread_setuserstack 
 blx _panic 
 ps_ptr__thread_setuserstack: 
 .long panicString__thread_setuserstack 
 panicString__thread_setuserstack: 
 .asciz "_thread_setuserstack is not implemented." 

.align 6
 .globl _thread_userstack 
 _thread_userstack: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__thread_userstack 
 blx _panic 
 ps_ptr__thread_userstack: 
 .long panicString__thread_userstack 
 panicString__thread_userstack: 
 .asciz "_thread_userstack is not implemented." 

.align 6
 .globl _vfp_context_save 
 _vfp_context_save: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__vfp_context_save 
 blx _panic 
 ps_ptr__vfp_context_save: 
 .long panicString__vfp_context_save 
 panicString__vfp_context_save: 
 .asciz "_vfp_context_save is not implemented." 

.align 6
 .globl _vfp_enable_exception 
 _vfp_enable_exception: 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 nop 
 ldr x0, ps_ptr__vfp_enable_exception 
 blx _panic 
 ps_ptr__vfp_enable_exception: 
 .long panicString__vfp_enable_exception 
 panicString__vfp_enable_exception: 
 .asciz "_vfp_enable_exception is not implemented."
