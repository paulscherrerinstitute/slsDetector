# slsDetectorConfig (
#               portName,       # The name of the asyn port driver to be created.
#               configFileName, # The configuration file to the detector.
#               detectorId,     # The detector index number running on the same system.
#               maxBuffers,     # The maximum number of NDArray buffers that the NDArrayPool for this driver is 
#                                 allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
#               maxMemory)      # The maximum amount of memory that the NDArrayPool for this driver is 
#                                 allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
slsDetectorConfig($(PORT=SD1), $(CONFIG), 0)
dbLoadRecords("slsDetector.template","P=$(PREFIX),R=$(R=cam1:),PORT=$(PORT=SD1),ADDR=$(ADDR=0),TIMEOUT=1")
set_requestfile_path("$($(MODULE)_DIR)db")
