SRC_DIR := wrappers/unix03

C_SRC_FILES := \
	chmod.c \
	fchmod.c \
	getrlimit.c \
	mmap.c \
	setrlimit.c

C_SRCS += $(addprefix $(SRC_DIR)/, $(C_SRC_FILES))
