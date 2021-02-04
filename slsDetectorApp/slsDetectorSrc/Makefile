TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================


LIBRARY_IOC_Linux += slsDetectorDriver
USR_CXXFLAGS += -std=c++11
LIB_SRCS += slsDetectorDriver.cpp
LIB_LIBS += SlsDetector

DBD += slsDetectorSupport.dbd

include $(ADCORE)/ADApp/commonLibraryMakefile

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

