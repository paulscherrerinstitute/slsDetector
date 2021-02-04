TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

PROD_NAME = jungfrauApp
PROD_IOC += $(PROD_NAME)

# <name>.dbd will be created from <name>Include.dbd
DBD += $(PROD_NAME).dbd
$(PROD_NAME)_DBD += slsDetectorSupport.dbd

USR_CXXFLAGS += -D_GLIBCXX_USE_CXX11_ABI=0 

$(PROD_NAME)_SRCS += jungfrauApp_registerRecordDeviceDriver.cpp jungfrauAppMain.cpp

# Add locally compiled object code
$(PROD_NAME)_LIBS += slsDetectorDriver SlsDetector

# Add system library
$(PROD_NAME)_SYS_LIBS += pthread zmq

include $(ADCORE)/ADApp/commonDriverMakefile

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

