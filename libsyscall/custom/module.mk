SRC_DIR := custom

C_SRC_FILES := \
	errno.c

C_SRCS += $(addprefix $(SRC_DIR)/, $(C_SRC_FILES))
