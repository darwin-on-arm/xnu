export MakeInc_cmd=${SRCROOT}/makedefs/MakeInc.cmd
export MakeInc_def=${SRCROOT}/makedefs/MakeInc.def
export MakeInc_rule=${SRCROOT}/makedefs/MakeInc.rule
export MakeInc_dir=${SRCROOT}/makedefs/MakeInc.dir


include $(MakeInc_cmd)
include $(MakeInc_def)

DATAFILES = \
	_structs.h	endian.h	param.h		types.h \
	_limits.h	_types.h	limits.h	setjmp.h \
	_param.h	arch.h		locks.h		signal.h \
	_mcontext.h

KERNELFILES = \
	_structs.h	endian.h	param.h		types.h \
	_limits.h	_types.h	limits.h	setjmp.h \
	_param.h	arch.h		locks.h		signal.h \
	_mcontext.h


INSTALL_MD_LIST = ${DATAFILES}
INSTALL_MD_LCL_LIST = ${DATAFILES} disklabel.h

INSTALL_MD_DIR = arm

EXPORT_MD_LIST = ${KERNELFILES}

EXPORT_MD_DIR = arm

include $(MakeInc_rule)
include $(MakeInc_dir)


