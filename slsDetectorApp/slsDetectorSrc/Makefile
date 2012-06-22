TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================


LIBRARY_IOC_Linux += slsDetectorDriver
LIB_SRCS += slsDetectorDriver.cpp
LIB_LIBS += SlsDetector

PROD_IOC_Linux += slsDetectorApp

# <name>.dbd will be created from <name>Include.dbd
DBD += slsDetectorApp.dbd
DBD += slsDetectorSupport.dbd

# <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
PROD_SRCS += slsDetectorApp_registerRecordDeviceDriver.cpp slsDetectorAppMain.cpp

# Add locally compiled object code
PROD_LIBS += slsDetectorDriver SlsDetector 

include $(AREA_DETECTOR)/ADApp/commonDriverMakefile

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

