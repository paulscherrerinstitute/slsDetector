include /ioc/tools/driver.makefile
PROJECT=slsDetector
EXCLUDE_VERSIONS=3.13 3.14
BUILDCLASSES=Linux
ARCH_FILTER=$(EPICS_HOST_ARCH)

LIBVERSION=v6.0.0

SOURCES+=../slsDetectorApp/slsDetectorSrc/slsDetectorDriver.cpp
DBDS+=../slsDetectorApp/slsDetectorSrc/slsDetectorSupport.dbd

# convert SL6-x86_64 to linux-x86_64
ARCH=linux-$(word 2,$(subst -, , $(EPICS_HOST_ARCH)))
USR_INCLUDES += -I../../slsDetectorApp/slsDetectorSupport
USR_CXXFLAGS += -std=c++11
USR_LDFLAGS += -L../../slsDetectorApp/slsDetectorSupport/os/$(ARCH)

USR_LIBS += SlsDetector SlsSupport

SHRLIBS += ../slsDetectorApp/slsDetectorSupport/os/$(ARCH)/libSlsSupport.so.6
SHRLIBS += ../slsDetectorApp/slsDetectorSupport/os/$(ARCH)/libSlsDetector.so.6

TEMPLATES += ../slsDetectorApp/Db/slsDetector.template
TEMPLATES += ../slsDetectorApp/Db/slsDetector_settings.req

