TOP = ..
include $(TOP)/configure/CONFIG

DIRS += slsDetectorSupport
DIRS += slsDetectorSrc

DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *db*))
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *Db*))

include $(TOP)/configure/RULES_DIRS

