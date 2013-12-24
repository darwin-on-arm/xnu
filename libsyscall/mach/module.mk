SRC_DIR := mach

MIG_USR_SRC_FILES := \
	clock.defs \
	clock_reply.defs \
	host_priv.defs \
	ledger.defs \
	mach_host.defs \
	mach_vm.defs \
	processor.defs \
	task.defs \
	vm_map.defs \
	clock_priv.defs \
	host_security.defs \
	lock_set.defs \
	mach_port.defs \
	notify.defs \
	processor_set.defs \
	thread_act.defs

MIG_SRV_SRC_FILES := \
	exc.defs

S_SRC_FILES := \
	mach_traps.s

C_SRC_FILES := \
	clock_sleep.c \
	exc_catcher.c \
	fprintf_stderr.c \
	mach_init.c \
	mach_port.c \
	mig_deallocate.c \
	mig_strncpy.c \
	port_obj.c \
	string.c \
	dylib_link.c \
	exc_catcher_state.c \
	mach_error.c \
	mach_legacy.c  \
	mach_vm.c \
	mig_reply_port.c \
	ms_thread_switch.c \
	semaphore.c \
	error_codes.c \
	exc_catcher_state_identity.c \
	mach_error_string.c  mach_msg.c \
	mig_allocate.c \
	mig_reply_setup.c \
	panic.c \
	slot_name.c

SRC_FILES := $(MIG_SRC_FILES) $(C_SRC_FILES)

%User.o: %.defs
	$(MIG) $(MIGFLAGS) -arch $(RC_ARCHS) -cc $(CC) -MD "$(CFLAGS)" -user $*User.c -server /dev/null -header $*.h -sheader /dev/null -dheader /dev/null $*.defs
	$(CC) $(CFLAGS) -c $*User.c -o $*User.o
	rm -f $(notdir $**.h) $(notdir $**.d)

%Server.o: %.defs
	$(MIG) $(MIGFLAGS) -arch $(RC_ARCHS) -cc $(CC) -MD "$(CFLAGS)" -server $*Server.c -user /dev/null -header /dev/null -sheader $*Server.h -dheader /dev/null $*.defs
	$(CC) $(CFLAGS) -c $*Server.c -o $*Server.o
	rm -f $(notdir $**.h) $(notdir $**.d)

S_SRCS += $(addprefix $(SRC_DIR)/, $(S_SRC_FILES))
C_SRCS += $(addprefix $(SRC_DIR)/, $(C_SRC_FILES))
MIG_USR_SRCS += $(addprefix $(SRC_DIR)/, $(MIG_USR_SRC_FILES))
MIG_SRV_SRCS += $(addprefix $(SRC_DIR)/, $(MIG_SRV_SRC_FILES))
