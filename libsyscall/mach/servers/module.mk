SRC_DIR := mach/servers

MIG_USR_SRC_FILES := \
	netname.defs

MIG_USR_SRCS += $(addprefix $(SRC_DIR)/, $(MIG_USR_SRC_FILES))
