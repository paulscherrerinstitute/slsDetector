TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

INC += multiSlsDetector.h  sls_detector_defs.h sls_detector_funcs.h slsDetectorUsers.h slsDetectorUtils.h slsDetectorActions.h postProcessing.h
INC += fileIO.h detectorData.h angularConversion.h slsDetectorBase.h

# These libraries are built from slsDetectorSoftware
ifeq (linux-x86_64, $(findstring linux-x86_64, $(T_A)))
LIB_INSTALLS_Linux += ../os/linux-x86_64/libSlsDetector.a

else ifeq (linux-x86, $(findstring linux-x86, $(T_A)))
LIB_INSTALLS_Linux += ../os/linux-x86/libSlsDetector.a

endif

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

