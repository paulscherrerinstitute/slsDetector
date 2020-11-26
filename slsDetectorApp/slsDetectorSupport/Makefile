TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

INC += sls/Detector.h
INC += sls/Result.h
INC += sls/TimeHelper.h
INC += sls/ToString.h
INC += sls/TypeTraits.h
INC += sls/container_utils.h
INC += sls/detectorData.h
INC += sls/network_utils.h
INC += sls/sls_detector_defs.h
INC += sls/sls_detector_exceptions.h
INC += sls/string_utils.h

# These libraries are built from slsDetectorSoftware
ifeq (linux-x86_64, $(findstring linux-x86_64, $(T_A)))
LIB_INSTALLS_Linux += ../os/linux-x86_64/libSlsSupport.so
LIB_INSTALLS_Linux += ../os/linux-x86_64/libSlsDetector.so
endif

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

