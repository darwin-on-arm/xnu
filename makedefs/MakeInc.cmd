#
# Commands for the build environment
#

# Host OS type
UNAME_S := $(shell uname -s)

##
# Verbosity
##
ifeq ($(RC_XBS),YES)
VERBOSE = YES
else
VERBOSE = NO
endif
ifeq ($(VERBOSE),YES)
_v =
_vstdout =
else
_v = @
_vstdout = > /dev/null
endif

##
# SDK paths
##
SDKROOT ?= /
HOST_SDKROOT ?= /
HOST_SPARSE_SDKROOT ?= /

# Standard UNIX/BSD tools
RM = /bin/rm -f
CP = /bin/cp
MV = /bin/mv
LN = /bin/ln -fs
CAT = /bin/cat
MKDIR = /bin/mkdir -p
FIND = /usr/bin/find
XARGS = /usr/bin/xargs
INSTALL = /usr/bin/install
TAR = /usr/bin/gnutar
BASENAME = /usr/bin/basename
TR = /usr/bin/tr

ifeq ($(UNAME_S),Darwin)

	# Host-specific verbosity
	ifeq ($(VERBOSE),YES)
		XCRUN = /usr/bin/xcrun -verbose -log
	else
		XCRUN = /usr/bin/xcrun
	endif

	# SDKROOT may be passed as a shorthand like "iphoneos.internal". We
	# must resolve these to a full path and override SDKROOT.
	ifeq ($(SDKROOT_RESOLVED),)
		ifeq ($(SDKROOT),/)
			export SDKROOT_RESOLVED	:= /
		else
			export SDKROOT_RESOLVED := $(shell xcodebuild -sdk $(SDKROOT) -version Path | head -1)
		endif
	endif

	# Override SDKROOT if it was set by the user
	override SDKROOT = $(SDKROOT_RESOLVED)

	# Find out our platform target
	ifeq ($(PLATFORM),)
		export PLATFORM := $(shell xcodebuild -sdk $(SDKROOT) -version PlatformPath | head -1 | sed 's,^.*/\([^/]*\)\.platform$$,\1,')
		ifeq ($(PLATFORM),)
			export PLATFORM := MacOSX
		endif
	endif

	ifeq ($(PLATFORM),iPhoneOS)
		DEVELOPER_DIR ?= $(shell xcode-select -print-path)
		export HOST_SPARSE_SDKROOT := $(DEVELOPER_DIR)/SDKs/iPhoneHostSideTools.sparse.sdk
	endif

	##
	# CC/CXX get defined by make(1) by default, so we can't check them
	# against the empty string to see if they haven't been set
	##
	ifeq ($(origin CC),default)
		ifneq ($(findstring iPhone,$(PLATFORM)),)
			export CC := $(shell $(XCRUN) -sdk $(SDKROOT) -find clang)
		else
			export CC := $(shell $(XCRUN) -sdk $(SDKROOT) -find clang)
		endif
	endif
	ifeq ($(origin CXX),default)
		ifneq ($(findstring iPhone,$(PLATFORM)),)
			export CXX := $(shell $(XCRUN) -sdk $(SDKROOT) -find clang++)
		else
			export CXX := $(shell $(XCRUN) -sdk $(SDKROOT) -find clang++)
		endif
	endif

	##
	# Misc. tools for native/cross building
	##
	ifeq ($(MIG),)
		export MIG := $(shell $(XCRUN) -sdk $(SDKROOT) -find mig)
	endif
	ifeq ($(MIGCC),)
		export MIGCC := $(CC)
	endif
	ifeq ($(STRIP),)
		export STRIP := $(shell $(XCRUN) -sdk $(SDKROOT) -find strip)
	endif
	ifeq ($(LIPO),)
		export LIPO := $(shell $(XCRUN) -sdk $(SDKROOT) -find lipo)
	endif
	ifeq ($(LIBTOOL),)
		export LIBTOOL := $(shell $(XCRUN) -sdk $(SDKROOT) -find libtool)
	endif
	ifeq ($(NM),)
		export NM := $(shell $(XCRUN) -sdk $(SDKROOT) -find nm)
	endif
	ifeq ($(UNIFDEF),)
		export UNIFDEF := $(shell $(XCRUN) -sdk $(SDKROOT) -find unifdef)
	endif
	ifeq ($(DSYMUTIL),)
		export DSYMUTIL := $(shell $(XCRUN) -sdk $(SDKROOT) -find dsymutil)
	endif
	ifeq ($(CTFCONVERT),)
		export CTFCONVERT := $(shell $(XCRUN) -sdk $(SDKROOT) -find ctfconvert)
	endif
	ifeq ($(CTFMERGE),)
		export CTFMERGE :=  $(shell $(XCRUN) -sdk $(SDKROOT) -find ctfmerge)
	endif
	ifeq ($(CTFSCRUB),)
		export CTFSCRUB := $(shell $(XCRUN) -sdk $(SDKROOT) -find ctfdump) -r
	endif
	ifeq ($(NMEDIT),)
		export NMEDIT := $(shell $(XCRUN) -sdk $(SDKROOT) -find nmedit)
	endif

	# Platform-specific tools
	ifneq ($(findstring iPhone,$(PRODUCT)),)
		#ifeq ($(EMBEDDED_DEVICE_MAP),)
		#	export EMBEDDED_DEVICE_MAP := $(shell $(XCRUN) -sdk $(SDKROOT) -find embedded_device_map || echo /usr/bin/true)
		#endif
		ifeq ($(IPHONEOS_OPTIMIZE),)
			export IPHONEOS_OPTIMIZE := $(shell $(XCRUN) -sdk $(SDKROOT) -find iphoneos-optimize || echo /usr/bin/true)
		endif
	endif

	CTFINSERT = $(XCRUN) -sdk $(SDKROOT) ctf_insert

	# Scripts or tools we build ourselves
	SEG_HACK := $(OBJROOT)/SETUP/setsegname/setsegname
	KEXT_CREATE_SYMBOL_SET := $(OBJROOT)/SETUP/kextsymboltool/kextsymboltool
	DECOMMENT := $(OBJROOT)/SETUP/decomment/decomment
	NEWVERS = $(SRCROOT)/config/newvers.pl
	MD := $(OBJROOT)/SETUP/md/md

	###
	# Commands to generate host binaries. HOST_CC intentionally not
	# $(CC), which controls the target compiler
	###
	ifeq ($(HOST_CC),)
		export HOST_CC := $(shell $(XCRUN) -sdk $(HOST_SDKROOT) -find cc)
	endif
	ifeq ($(HOST_FLEX),)
		export HOST_FLEX := $(shell $(XCRUN) -sdk $(HOST_SDKROOT) -find flex)
	endif
	ifeq ($(HOST_BISON),)
		export HOST_BISON := $(shell $(XCRUN) -sdk $(HOST_SDKROOT) -find bison)
	endif
	ifeq ($(HOST_CODESIGN),)
		export HOST_CODESIGN := $(shell $(XCRUN) -sdk $(HOST_SDKROOT) -find codesign)
	endif	

	##
	# Command to build libkmod.a/libkmodc++.a, which are
	# linked into kext binaries, and should be built as if
	# they followed system-wide policies
	##
	ifeq ($(LIBKMOD_CC),)
		export LIBKMOD_CC := $(shell $(XCRUN) -sdk $(SDKROOT) -find clang)
	endif

else ifeq ($(UNAME_S),Linux)

	# We don't use xcrun (yet)
	XCRUN = /bin/true

	# For now, we assume that everything we need is in the rootfs
	ifeq ($(SDKROOT_RESOLVED),)
		ifneq ($(SDKROOT),/)
			export SDKROOT_RESOLVED	:= /
		endif
	endif

	# Override SDKROOT if it was set by the user
	override SDKROOT = $(SDKROOT_RESOLVED)

	# Just assume a MacOSX target
	export PLATFORM := MacOSX

	##
	# CC/CXX get defined by make(1) by default, so we can't check them
	# against the empty string to see if they haven't been set
	##
	ifeq ($(origin CC),default)
		export CC := /usr/bin/clang
	endif
	ifeq ($(origin CXX),default)
		export CXX := /usr/bin/clang++
	endif

	# XXX We need a better way of handling this
	TARGET_PROFILE ?= arm-apple-darwin11
	TOOLCHAIN_PREFIX := $(addsuffix -,$(TARGET_PROFILE))

	##
	# Toolchain tools for cross building
	##
	ifeq ($(LD),)
		export LD := $(addprefix $(TOOLCHAIN_PREFIX),ld)
	endif
	ifeq ($(AS),)
		export AS := $(addprefix $(TOOLCHAIN_PREFIX),as)
	endif
	ifeq ($(STRIP),)
		export STRIP := $(addprefix $(TOOLCHAIN_PREFIX),strip)
	endif
	ifeq ($(LIPO),)
		export LIPO := $(addprefix $(TOOLCHAIN_PREFIX),lipo)
	endif
	ifeq ($(LIBTOOL),)
		export LIBTOOL := $(addprefix $(TOOLCHAIN_PREFIX),libtool)
	endif
	ifeq ($(NM),)
		export NM := $(addprefix $(TOOLCHAIN_PREFIX),nm)
	endif
	ifeq ($(NMEDIT),)
		export NMEDIT := $(addprefix $(TOOLCHAIN_PREFIX),nmedit)
	endif

	# XXX May need this later
	CTFINSERT := /bin/true

	##
	# Misc. tools for building
	##
	ifeq ($(MIG),)
		export MIG := /usr/bin/mig
	endif
	ifeq ($(MIGCC),)
		export MIGCC := $(CC)
	endif
	ifeq ($(UNIFDEF),)
		export UNIFDEF := /usr/bin/unifdef
	endif
	ifeq ($(DSYMUTIL),)
		export DSYMUTIL := /bin/true
	endif
	ifeq ($(CTFCONVERT),)
		export CTFCONVERT := /bin/true
	endif
	ifeq ($(CTFMERGE),)
		export CTFMERGE :=  /bin/true
	endif
	ifeq ($(CTFSCRUB),)
		export CTFSCRUB := /bin/true -r
	endif

	##
	# Other special tools
	##
	SEG_HACK := /usr/bin/setsegname
	KEXT_CREATE_SYMBOL_SET := /usr/bin/kextsymboltool
	DECOMMENT := $(OBJROOT)/SETUP/decomment/decomment
	NEWVERS = $(SRCROOT)/config/newvers.pl
	MD := $(OBJROOT)/SETUP/md/md

	###
	# Commands to generate host binaries.
	###
	ifeq ($(HOST_CC),)
		export HOST_CC := $(CC)
	endif
	ifeq ($(HOST_FLEX),)
		export HOST_FLEX := /usr/bin/flex
	endif
	ifeq ($(HOST_BISON),)
		export HOST_BISON := /usr/bin/bison
	endif
	ifeq ($(HOST_CODESIGN),)
		export HOST_CODESIGN := /bin/true
	endif

	##
	# Command to build libkmod.a/libkmodc++.a, which are
	# linked into kext binaries, and should be built as if
	# they followed system-wide policies
	##
	ifeq ($(LIBKMOD_CC),)
		export LIBKMOD_CC := $(CC)
	endif

else
	echo "Error: Unsupported build host: $(UNAME_S)"
	exit 1
endif

# vim: set ft=make:
