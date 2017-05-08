include /ioc/tools/driver.makefile
PROJECT=slsDetector
EXCLUDE_VERSIONS=3.13 3.14.8
BUILDCLASSES=Linux
CROSS_COMPILER_TARGET_ARCHS=$(EPICS_HOST_ARCH)

LIBVERSION=v2.3.1

SOURCES+=../slsDetectorApp/slsDetectorSrc/slsDetectorDriver.cpp
DBDS+=../slsDetectorApp/slsDetectorSrc/slsDetectorSupport.dbd

# convert SL6-x86_64 to linux-x86_64
ARCH=linux-$(word 2,$(subst -, , $(EPICS_HOST_ARCH)))
USR_INCLUDES += -I../../slsDetectorApp/slsDetectorSupport
USR_LDFLAGS += -L../../slsDetectorApp/slsDetectorSupport/os/$(ARCH)

USR_LIBS += SlsDetector pthread

TEMPLATES += ../slsDetectorApp/Db/slsDetector.template
TEMPLATES += ../slsDetectorApp/Db/slsDetector_settings.req

