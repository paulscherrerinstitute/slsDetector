EPICS areaDetector driver for slsDetector
=========================================

slsDetector refers to a group of detectors produced by SLS detectors group in PSI. 
They range from strip detectors to pixel array detectors. 
It is however a common interest of the group to use a common platform for data acquisition. 

The architecture includes a socket server running on the detector controller(an embeded Linux), 
and a client side C++ library to communicate with the server. This driver is using such client side library, namely libSlsDetector, to control detector parameters as well as retrieve data. This driver inherits from ADDriver.


Implementation of standard driver parameters from ADBase.template and NDFile.template
-------------------------------------------------------------------------------------

========================  =====================   ============
Parameter index variable  EPICS record name       Description
========================  =====================   ============
ADManufacturer            $(P)$(R)Manufacturer    Detector manuafacturer, either PSI or Dectris
ADModel                   $(P)$(R)Model           Detector type, Mythen, Eiger
ADMaxSizeX                $(P)$(R)SizeX           Detector maximum size in X
ADMaxSizeY                $(P)$(R)SizeY           Detector maximum size in Y
ADMinX                    $(P)$(R)MinX            First pixel to read in the X direction.
ADMinX                    $(P)$(R)MinY            First pixel to read in theY direction.
ADSizeX                   $(P)$(R)SizeX           DetectorDetector size in X
ADSizeY                   $(P)$(R)SizeY           DetectorDetector size in Y Depending on the hardware, the actual setting maybe rounded to the closest module.
ADAcquireTime             $(P)$(R)AcquireTime     Exposure time measured in in seconds
ADAcquirePeriod           $(P)$(R)AcquirePeriod   Exposure period in case of multiple images measured in seconds
NDFileNumber              $(P)$(R)FileNumber      File number
NDFilePath                $(P)$(R)FilePath        File path
NDFileName                $(P)$(R)FileName        File base name
NDFullFileName            $(P)$(R)FullFileName    Composed from format "%s%s_%d", FilePath, FileName, FileNumber
NDAutoSave                $(P)$(R)AutoSave        Write flag (0=No, 1=Yes) controlling whether a file is automatically saved each time acquisition completes.
========================  =====================   ============

slsDetector specific parameters
-------------------------------

========================  ============== ====== ===================  =======================   ============
Parameter index variable  asyn interface Access drvInfo string       EPICS record name         Description
========================  ============== ====== ===================  =======================   ============
SDSetting                 asynInt32      r/w    SD_SETTING           $(P)$(R)Setting           Detector settings
SDThreshold               asynInt32      r/w    SD_THRESHOLD         $(P)$(R)ThresholdEnergy   Threshold energy in eV
SDEnergy                  asynInt32      r/w    SD_ENERGY            $(P)$(R)BeamEnergy        Beam energy in eV. Threshold energy will change to be half of the beam energy.
SDOnline                  asynInt32      r/w    SD_ONLINE            $(P)$(R)Online            Detector online control
SDFlatFieldPath           asynOctet      r/w    SD_FLATFIELD_PATH    $(P)$(R)FlatFieldPath     File path and name of a file to correct for the flat field.
SDFlatFieldFile           asynOctet      r/w    SD_FLATFIELD_FILE    $(P)$(R)FlatFieldFile
SDUseFlatField            asynInt32      r/w    SD_USE_FLATFIELD     $(P)$(R)UseFlatField      Enable flat field correction
SDUseCountRate            asynInt32      r/w    SD_USE_COUNTRATE     $(P)$(R)UseCountRate      Enable count rate correction
SDUsePixelMask            asynInt32      r/w    SD_USE_PIXELMASK     $(P)$(R)UsePixelMask      Enable pixel mask correction
SDUseAngularConv          asynInt32      r/w    SD_USE_ANGULAR_CONV  $(P)$(R)UseAngularConv    Enable angular conversion       
SDBitDepth                asynInt32      r/w    SD_BIT_DEPTH         $(P)$(R)BitDepth          Dynamic range
SDTimingMode              asynInt32      r/w    SD_TMODE             $(P)$(R)TimingMode        External signal communication mode, triggering, gating 
SDDelayTime               asynFloat64    r/w    SD_DELAY_TIME        $(P)$(R)DelayTime         Delay in seconds between external trigger and the start of image acquisition
SDNumGates                asynInt32      r/w    SD_NGATES            $(P)$(R)NumGates          Number of gates if timing mode is gating        
SDNumCycles               asynInt32      r/w    SD_NCYCLES           $(P)$(R)NumCycles         Number of triggeres     
SDNumFrames               asynInt32      r/w    SD_NFRAMES           $(P)$(R)NumFrames         Number of frames to acquire for each trigger    
SDSetupFile               asynOctet      r/w    SD_SETUP_FILE        $(P)$(R)SetupFile         Detector setup from file        
SDLoadSetup               asynInt32      r/w    SD_LOAD_SETUP        $(P)$(R)LoadSetup         Load detector setup from file   
SDSaveSetup               asynInt32      r/w    SD_SAVE_SETUP        $(P)$(R)SaveSetup         Save detector setup to file     
========================  ============== ====== ===================  =======================   ============

Configuration
-------------

::

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
    slsDetectorConfig("SD1", "cfg/mcs1x21.config", 0, 0, -1, -1)


Release Notes
-------------

* 1.1 - 10.12.2013

  1. libSlsDetector updated to r706.
  2. Avoid calling getDetectorStatus from two threads simultaneously.

* 1.0 - 18.09.2013

  1. Milestone release. It has been tested for MYTHEN and GOTTHARD detectors.detectors
