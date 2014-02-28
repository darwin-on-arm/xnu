SRC_DIR := mach/arm

C_SRC_FILES := \
	vm_map_compat.c

C_SRCS += $(addprefix $(SRC_DIR)/, $(C_SRC_FILES))
