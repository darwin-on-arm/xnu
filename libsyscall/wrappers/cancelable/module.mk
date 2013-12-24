SRC_DIR := wrappers/cancelable

C_SRC_FILES := \
	fcntl.c \
	fcntl-cancel.c \
	select.c \
	select-cancel.c \
	sigsuspend.c \
	sigsuspend-cancel.c

C_SRCS += $(addprefix $(SRC_DIR)/, $(C_SRC_FILES))
