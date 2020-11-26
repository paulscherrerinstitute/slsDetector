set_requestfile_path("$($(MODULE)_DIR)db")

# slsDetectorConfig (
#               portName,       # The name of the asyn port driver to be created.
#               configFileName, # The configuration file to the detector.
#               detectorId,     # The detector index number running on the same system.
#               numModules,     # The number of modules.
#               maxBuffers,     # The maximum number of NDArray buffers that the NDArrayPool for this driver is 
#                                 allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
#               maxMemory)      # The maximum amount of memory that the NDArrayPool for this driver is 
#                                 allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
slsDetectorConfig("$(PORT=SD$(N=1))", "$(CONFIG)", $(ID=0), $(MODULES=1))
dbLoadRecords("slsDetector.template","P=$(PREFIX),R=$(RECORD=cam1:),PORT=$(PORT=SD$(N=1)),ADDR=0,TIMEOUT=1")

set_pass0_restoreFile("slsDetector_settings$(N=1).sav")
set_pass1_restoreFile("slsDetector_settings$(N=1).sav")

afterInit create_monitor_set,"slsDetector_settings.req",30,"P=$(PREFIX),R=$(RECORD=cam1:)","slsDetector_settings$(N=1).sav"
