SRC_DIR := wrappers

S_SRC_FILES := \
	__get_cpu_capabilities.s

C_SRC_FILES := \
	init_cpu_capabilities.c \
	kill.c \
	open_dprotected_np.c \
	rename.c \
	unlink.c \
	ioctl.c \
	memcpy.c \
	remove-counter.c \
	rmdir.c \
	_libkernel_init.c \
	_libc_funcptr.c

S_SRCS += $(addprefix $(SRC_DIR)/, $(S_SRC_FILES))
C_SRCS += $(addprefix $(SRC_DIR)/, $(C_SRC_FILES))
