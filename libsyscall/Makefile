# Project name and version
export RC_ProjectName := Libsyscall
export RC_ProjectSourceVersion := 2050

# Build tools used
CC := $(shell xcrun -find clang)
MIG := $(shell xcrun -find mig)
LIBTOOL := $(shell xcrun -find libtool)

# User-specified variables
RC_ARCHS ?= armv7
RC_PLATFORM ?= MacOSX
DESTDIR ?= /

# Path to xnu kernel source
KERNEL_SOURCE ?= ../

# Local project variables
export SRCROOT := $(CURDIR)

ifndef SDKROOT
	export SDKROOT := $(shell xcrun -show-sdk-path)
endif

ifndef DEVELOPER_DIR
	export DEVELOPER_DIR := $(shell xcode-select -print-path)
endif

export ARCHS := $(RC_ARCHS)
export PLATFORM := $(RC_PLATFORM)

BUILDDIR := Build
BUILDROOT := $(BUILDDIR)/$(ARCHS)
BUILDOBJS := $(BUILDROOT)/Objects
GENROOT := $(BUILDROOT)/GeneratedSources

export OBJROOT := $(SRCROOT)
export DSTROOT := /tmp/$(RC_ProjectName).dst
export DERIVED_SOURCES_DIR := $(GENROOT)

# Our library name
LIB := libsystem_kernel

# C compiler flags
CFLAGS :=

CFLAGS_ARGS := \
	-arch $(ARCHS) \
	-Os \
	-fstrict-aliasing \
	-fpascal-strings \
	-fmessage-length=0 \
	-std=gnu99 \
	-fdollars-in-identifiers \
	-no-cpp-precomp \
	-fno-common \
	-fno-stack-protector \
	-pipe

CFLAGS_DEFS := \
	-DLIBSYSCALL_INTERFACE \
	-D__DARWIN_VERS_1050=1 \
	-DSYSCALL_PRE1050 \
	-DCF_OPEN_SOURCE \
	-DCF_EXCLUDE_CSTD_HEADERS \
	-DDEBUG \
	-D_FORTIFY_SOURCE=0
#	-DTARGET_OS_EMBEDDED=1

CFLAGS_WARNS := \
	-Wno-trigraphs \
	-Wno-missing-field-initializers \
	-Wno-missing-prototypes \
	-Wreturn-type -Wformat \
	-Wno-missing-braces \
	-Wparentheses -Wswitch \
	-Wno-unused-function \
	-Wno-unused-label \
	-Wno-unused-parameter \
	-Wunused-variable \
	-Wunused-value \
	-Wno-empty-body \
	-Wno-uninitialized \
	-Wno-unknown-pragmas \
	-Wno-shadow \
	-Wno-four-char-constants \
	-Wno-conversion \
	-Wno-constant-conversion \
	-Wno-int-conversion \
	-Wno-enum-conversion \
	-Wno-shorten-64-to-32 \
	-Wpointer-sign \
	-Wnewline-eof \
	-Wmost

CFLAGS_INCS := \
	-I$(GENROOT) \
	-I$(SDKROOT)/System/Library/Frameworks/System.framework/Versions/B/PrivateHeaders

# Mach Interface Generator flags
MIGFLAGS += $(CFLAGS_INCS)

# Libtool flags
LIBTOOL_FLAGS += -arch_only $(ARCHS) -syslibroot $(SDKROOT) -L$(BUILDROOT) 

# Subdirectories in this project
MODULES := \
	custom \
	mach \
	mach/arm \
	mach/servers \
	wrappers \
	wrappers/unix03 \
	wrappers/legacy \
	wrappers/cancelable

# Populate our CFLAGS
CFLAGS += $(CFLAGS_ARGS)
CFLAGS += $(CFLAGS_DEFS)
CFLAGS += $(patsubst %,-I%/,$(MODULES))
CFLAGS += $(CFLAGS_INCS)
CFLAGS += $(CFLAGS_WARNS)

# Misc. source code variables
S_SRCS :=
C_SRCS :=
MIG_USR_SRCS :=
MIG_SRV_SRCS :=

# Pull in information from throughout the project
include $(patsubst %,%/module.mk,$(MODULES))

# Compiled objects to resolve from source code
OBJS := \
	$(patsubst %.defs,%Server.o, $(filter %.defs,$(MIG_SRV_SRCS))) \
	$(patsubst %.defs,%User.o, $(filter %.defs,$(MIG_USR_SRCS))) \
	$(patsubst %.defs,%User.o, $(filter %.defs,$(MIG_SRV_SRCS))) \
	$(patsubst %.s,%.o, $(filter %.s,$(S_SRCS))) \
	$(patsubst %.c,%.o, $(filter %.c,$(C_SRCS)))

# Compilation rules for various source code types
%.o: %.c
	$(eval obj_uuid := $(shell uuidgen -r))
	$(CC) -x c $(CFLAGS) -c $< -o $(BUILDOBJS)/$(notdir $*-$(obj_uuid).o)
	@echo "$(BUILDOBJS)/$(notdir $*-$(obj_uuid).o)" >> $(BUILDOBJS)/Objects.list

%.o: %.s
	$(eval obj_uuid := $(shell uuidgen -r))
	$(CC) -x assembler-with-cpp $(CFLAGS) -c $< -o $(BUILDOBJS)/$(notdir $*-$(obj_uuid).o)
	@echo "$(BUILDOBJS)/$(notdir $*-$(obj_uuid).o)" >> $(BUILDOBJS)/Objects.list

%User.o: %.defs
	$(eval obj_uuid := $(shell uuidgen -r))
	$(MIG) $(MIGFLAGS) -arch $(ARCHS) -cc $(CC) -user $(GENROOT)/$(notdir $*User.c) -server /dev/null -header $(GENROOT)/$(notdir $*.h) -sheader /dev/null -dheader /dev/null $*.defs
	$(CC) -x c $(CFLAGS) -c $(GENROOT)/$(notdir $*User.c) -o $(BUILDOBJS)/$(notdir $*User-$(obj_uuid).o)
	@echo "$(BUILDOBJS)/$(notdir $*User-$(obj_uuid).o)" >> $(BUILDOBJS)/Objects.list
	rm -f $(notdir $**.h) $(notdir $**.d)

%Server.o: %.defs
	$(eval obj_uuid := $(shell uuidgen -r))
	$(MIG) $(MIGFLAGS) -arch $(ARCHS) -cc $(CC) -server $(GENROOT)/$(notdir $*Server.c) -user $(GENROOT)/$(notdir $*User.c) -header $(GENROOT)/$(notdir $*.h) -sheader $(GENROOT)/$(notdir $*Server.h) -dheader /dev/null $*.defs
	$(CC) -x c $(CFLAGS) -c $(GENROOT)/$(notdir $*Server.c) -o $(BUILDOBJS)/$(notdir $*Server-$(obj_uuid).o)
	@echo "$(BUILDOBJS)/$(notdir $*Server-$(obj_uuid).o)" >> $(BUILDOBJS)/Objects.list
	rm -f $(notdir $**.h) $(notdir $**.d)

# Main project rules
.PHONY: all install clean
.SECONDARY: $(LIB)

all: setup $(LIB)

setup:
	rm -rf $(BUILDROOT)
	mkdir -p $(GENROOT)
	mkdir -p $(BUILDOBJS)/sys
	$(DEVELOPER_DIR)/Makefiles/bin/version.pl $(RC_ProjectName) > $(GENROOT)/$(RC_ProjectName)_version.c
	$(CC) $(CFLAGS) -c $(GENROOT)/$(RC_ProjectName)_version.c -o $(BUILDROOT)/$(RC_ProjectName)_version.o
	xcodescripts/create-syscalls.pl $(KERNEL_SOURCE)/bsd/kern/syscalls.master custom/ Platforms/ $(PLATFORM) $(BUILDOBJS)/sys
	xcodescripts/compile-syscalls.pl $(BUILDOBJS)/sys/stubs.list $(BUILDROOT)/syscalls.a
	xcodescripts/mach_install_mig.sh

$(LIB): $(OBJS)
	$(LIBTOOL) $(LIBTOOL_FLAGS) -static -filelist $(BUILDOBJS)/Objects.list $(BUILDROOT)/syscalls.a $(BUILDROOT)/Libsyscall_version.o -o $(BUILDROOT)/$@.a
	$(CC) -arch $(ARCHS) -dynamiclib -isysroot $(SDKROOT) -L$(BUILDROOT) -F$(BUILDROOT) -install_name /usr/lib/system/$(LIB).dylib -nostdlib -mmacosx-version-min=10.8 -dead_strip -umbrella System -all_load -lsystem_kernel -single_module -compatibility_version 1 -o $(BUILDROOT)/$(LIB).dylib

install: all
	install -d $(DESTDIR)/usr/lib/system
	install -d $(DESTDIR)/usr/include/servers
	install -d $(DESTDIR)/usr/include/mach/arm
	rsync -avPr $(DSTROOT)/ $(DESTDIR)/
	install -m 755 $(BUILDROOT)/$(LIB).dylib $(DESTDIR)/usr/lib/system/$(LIB).dylib

clean:
	rm -rf $(BUILDDIR) $(DSTROOT)
