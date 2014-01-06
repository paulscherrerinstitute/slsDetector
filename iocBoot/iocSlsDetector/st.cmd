< envPaths

dbLoadDatabase("$(TOP)/dbd/slsDetectorApp.dbd")

slsDetectorApp_registerRecordDeviceDriver(pdbbase)


epicsEnvSet("PREFIX","13SD1:")
epicsEnvSet("PORT",   "SD1")
epicsEnvSet("QSIZE",  "20")
epicsEnvSet("XSIZE",  "1280")
epicsEnvSet("YSIZE",  "1")
epicsEnvSet("NCHANS", "1360")
# slsDetectorConfig (
#               portName,       # The name of the asyn port driver to be created.
#               configFileName, # The configuration file to the detector.
#               detectorId,     # The detector index number running on the same system.
#               useReceiver,    # Wether to use builtin receiver. Set this to 1 to launch builtin receiver.
#                                 Only valid for detectors GOTTHARD, JUNGFRAU, EIGER.
#               maxBuffers,     # The maximum number of NDArray buffers that the NDArrayPool for this driver is 
#                                 allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
#               maxMemory)      # The maximum amount of memory that the NDArrayPool for this driver is 
#                                 allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
slsDetectorConfig("SD1", "mcs6x18.config", 0, 0, -1,-1)
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/ADBase.template",   "P=$(PREFIX),R=cam1:,PORT=$(PORT),ADDR=0,TIMEOUT=1")
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDFile.template",   "P=$(PREFIX),R=cam1:,PORT=$(PORT),ADDR=0,TIMEOUT=1")
dbLoadRecords("$(TOP)/slsDetectorApp/Db/slsDetector.template",        "P=$(PREFIX),R=cam1:,PORT=$(PORT),ADDR=0,TIMEOUT=1")

# Create a standard arrays plugin
NDStdArraysConfigure("Image1", 3, 0, "$(PORT)", 0)
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDPluginBase.template","P=$(PREFIX),R=image1:,PORT=Image1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),NDARRAY_ADDR=0")
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDStdArrays.template", "P=$(PREFIX),R=image1:,PORT=Image1,ADDR=0,TIMEOUT=1,TYPE=Float64,FTVL=DOUBLE,NELEMENTS=10000")


# Load all other plugins using commonPlugins.cmd
< $(AREA_DETECTOR)/iocBoot/commonPlugins.cmd

set_requestfile_path("$(TOP)/slsDetectorApp/Db")

iocInit()

# save things every thirty seconds
create_monitor_set("auto_settings.req", 30,"P=$(PREFIX),D=cam1:")
