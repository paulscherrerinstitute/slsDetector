include /ioc/tools/driver.makefile
PROJECT=slsDetector
EXCLUDE_VERSIONS=3.13
BUILDCLASSES=Linux
CROSS_COMPILER_TARGET_ARCHS=$(EPICS_HOST_ARCH)

SOURCES+=../slsDetectorApp/slsDetectorSrc/slsDetectorDriver.cpp
DBDS+=../slsDetectorApp/slsDetectorSrc/slsDetectorSupport.dbd

# For external built library
#SLS_DET_SOFTWARE = /afs/psi.ch/user/w/wang_x1/software/slsDetectorSoftware
#SLS_DET_SOFTWARE = /afs/psi.ch/project/mythen/newMythenSoftware/slsDetectorSoftware
#USR_INCLUDES+=-I$(SLS_DET_SOFTWARE)/slsDetector \
#			  -I$(SLS_DET_SOFTWARE)/slsDetectorAnalysis \
#USR_CFLAGS+=-DVERBOSE -g
#USR_LDFLAGS +=  -L$(SLS_DET_SOFTWARE) -Wl,-rpath $(SLS_DET_SOFTWARE)

# For builtin library
USR_INCLUDES += -I../../slsDetectorApp/slsDetectorSupport
USR_LDFLAGS += -L../../slsDetectorApp/slsDetectorSupport/os/linux-x86

USR_LIBS += SlsDetector pthread
