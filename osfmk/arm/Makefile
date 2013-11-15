export MakeInc_cmd=${SRCROOT}/makedefs/MakeInc.cmd
export MakeInc_def=${SRCROOT}/makedefs/MakeInc.def
export MakeInc_rule=${SRCROOT}/makedefs/MakeInc.rule
export MakeInc_dir=${SRCROOT}/makedefs/MakeInc.dir


include $(MakeInc_cmd)
include $(MakeInc_def)

EXPORT_ONLY_FILES = 	\
		    machine_routines.h \
		    locks.h lock.h cpu_number.h \
		cpu_capabilities.h mp.h io_map_entries.h \
		thread.h task.h ast.h ast_types.h \
		pmap.h machlimits.h cpu_affinity.h  asm_help.h cpu_data.h
 
INSTALL_MD_DIR = arm

EXPORT_MD_LIST = ${EXPORT_ONLY_FILES}

EXPORT_MD_DIR = arm

include $(MakeInc_rule)
include $(MakeInc_dir)


