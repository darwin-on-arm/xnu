SRC_DIR := wrappers/legacy

C_SRC_FILES := \
	accept.c \
	connect.c \
	getaudit.c \
	getsockname.c \
	lchown.c \
	mprotect.c \
	munmap.c \
	recvfrom.c \
	select.c \
	sendmsg.c \
	setattrlist.c \
	socketpair.c \
	bind.c \
	getattrlist.c \
	getpeername.c \
	kill.c \
	listen.c \
	msync.c \
	open.c \
	recvmsg.c \
	select-pre1050.c \
	sendto.c \
	sigsuspend.c

C_SRCS += $(addprefix $(SRC_DIR)/, $(C_SRC_FILES))
