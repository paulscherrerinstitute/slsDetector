< envPaths

dbLoadDatabase("$(TOP)/dbd/jungfrauApp.dbd")

jungfrauApp_registerRecordDeviceDriver(pdbbase)

epicsEnvSet("EPICS_DB_INCLUDE_PATH", "$(ADCORE)/ADApp/Db")
epicsEnvSet("PREFIX","X09LB-SD1:")
epicsEnvSet("PORT",   "SD1")
epicsEnvSet("QSIZE",  "20")
epicsEnvSet("XSIZE",  "1024")
epicsEnvSet("YSIZE",  "512")
epicsEnvSet("NCHANS", "2048")
# slsDetectorConfig (
#               portName,       # The name of the asyn port driver to be created.
#               configFileName, # The configuration file to the detector.
#               detectorId,     # The detector index number running on the same system.
#               numModules)     # The number of modules for a multi-module detector.
slsDetectorConfig("SD1", "bchip039.config", 1)
dbLoadRecords("$(slsDetector)/slsDetectorApp/Db/slsDetector.template","P=$(PREFIX),R=cam1:,PORT=$(PORT),ADDR=0,TIMEOUT=1")

# Create a standard arrays plugin
NDStdArraysConfigure("Image1", 3, 0, "$(PORT)", 0)
dbLoadRecords("$(ADCORE)/ADApp/Db/NDStdArrays.template", "P=$(PREFIX),R=image1:,PORT=Image1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),NDARRAY_ADDR=0,TYPE=Float64,FTVL=DOUBLE,NELEMENTS=524288")

# Load all other plugins using commonPlugins.cmd
< $(AREA_DETECTOR)/iocBoot/commonPlugins.cmd

set_requestfile_path(".")
set_requestfile_path("$(slsDetector)/slsDetectorApp/Db")
set_requestfile_path("$(ADCORE)/ADApp/Db")

set_savefile_path("./autosave")

iocInit()

# save things every thirty seconds
create_monitor_set("auto_settings.req", 30,"P=$(PREFIX),D=cam1:")
