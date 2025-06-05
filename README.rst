EPICS areaDetector driver for slsDetector
=========================================

slsDetector refers to a group of detectors produced by SLS detectors group in PSI. 
They range from strip detectors to pixel array detectors. 
It is however a common interest of the group to use a common platform for data acquisition. 

The architecture includes a socket server running on the detector controller(an embeded Linux), 
and a client side C++ library to communicate with the server. This driver is using such client side library, namely libSlsDetector, to control detector parameters as well as retrieve data. This driver inherits from ADDriver.


Implementation of standard driver parameters from ADBase.template and NDFile.template
-------------------------------------------------------------------------------------

========================  =========================   ============
Parameter index variable  EPICS record name           Description
========================  =========================   ============
ADManufacturer            $(P)$(R)Manufacturer        Detector manuafacturer, either PSI or Dectris
ADModel                   $(P)$(R)Model               Detector type, Mythen, Eiger
ADMaxSizeX                $(P)$(R)SizeX               Detector maximum size in X
ADMaxSizeY                $(P)$(R)SizeY               Detector maximum size in Y
ADAcquireTime             $(P)$(R)AcquireTime         Exposure time measured in in seconds
ADAcquirePeriod           $(P)$(R)AcquirePeriod       Exposure period in case of multiple images measured in seconds
ADTemperatureActual       $(P)$(R)TemperatureActual   Detector fpga temperature
NDFileNumber              $(P)$(R)FileNumber          File number
NDFilePath                $(P)$(R)FilePath            File path
NDFileName                $(P)$(R)FileName            File base name
NDFullFileName            $(P)$(R)FullFileName        Composed from format "%s%s_%d", FilePath, FileName, FileNumber
NDAutoSave                $(P)$(R)AutoSave            Write flag (0=No, 1=Yes) controlling whether a file is automatically saved each time acquisition completes.
========================  =========================   ============

slsDetector specific parameters
-------------------------------

========================  ============== ====== ====================  =================================  ============
Parameter index variable  asyn interface Access drvInfo string        EPICS record name                  Description
========================  ============== ====== ====================  =================================  ============
DetectorType              asynInt32      r/o    SD_DETECTOR_TYPE      $(P)$(R)DetectorType_RBV           Detector type enum
SDSetting                 asynInt32      r/w    SD_SETTING            $(P)$(R)Setting                    Detector settings
SDBitDepth                asynInt32      r/w    SD_BIT_DEPTH          $(P)$(R)BitDepth                   Dynamic range
SDTimingMode              asynInt32      r/w    SD_TMODE              $(P)$(R)TimingMode                 External signal communication mode, triggering, gating
SDTriggerSoftware         asynInt32      r/w    SD_TRIGGER_SOFTWARE   $(P)$(R)TriggerSoftware            Send software trigger
SDDelayTime               asynFloat64    r/w    SD_DELAY_TIME         $(P)$(R)DelayTime                  Delay in seconds between external trigger and the start of image acquisition
SDRecvMode                asynInt32      r/w    SD_RECV_MODE          $(P)$(R)ReceiverMode               Receiver data callback frequency
SDRecvStream              asynInt32      r/w    SD_RECV_STREAM        $(P)$(R)ReceiverStream             Enable/disable receiver stream
SDRecvStatus              asynInt32      r/o    SD_RECV_STATUS        $(P)$(R)ReceiverState_RBV          Receiver status
SDRecvMissed              asynInt32      r/o    SD_RECV_MISSED        $(P)$(R)ReceiverMissedPackets_RBV  Number of packets missed
SDHighVoltage             asynInt32      r/w    SD_HIGH_VOLTAGE       $(P)$(R)HighVoltage                Detector high voltage
SDNumCycles               asynInt32      r/w    SD_NCYCLES            $(P)$(R)NumCycles                  Number of triggeres
SDNumFrames               asynInt32      r/w    SD_NFRAMES            $(P)$(R)NumFrames                  Number of frames to acquire for each trigger
SDSetupFile               asynOctet      r/w    SD_SETUP_FILE         $(P)$(R)SetupFile                  Detector setup from file
SDLoadSetup               asynInt32      r/w    SD_LOAD_SETUP         $(P)$(R)LoadSetup                  Load detector setup from file
SDCommand                 asynOctet      r/w    SD_COMMAND            $(P)$(R)Command                    Direct command to detector
SDUseDataCallback         asynInt32      r/w    SD_USE_DATA_CALLBACK  $(R)$(R)UseDataCallback            Enable disable client data callback
========================  ============== ====== ====================  =================================  ============

Eiger and Mythen3 specific

========================  ============== ====== ===================  =======================   ============
Parameter index variable  asyn interface Access drvInfo string       EPICS record name         Description
========================  ============== ====== ===================  =======================   ============
SDThreshold               asynInt32      r/w    SD_THRESHOLD         $(P)$(R)ThresholdEnergy   Threshold energy in eV
SDEnergy                  asynInt32      r/w    SD_ENERGY            $(P)$(R)BeamEnergy        Beam energy in eV. Threshold energy will change to be half of the beam energy.
SDTrimbits                asynInt32      r/w    SD_TRIMBITS          $(P)$(R)Trimbits          Whether loading trimbits from settings
========================  ============== ====== ===================  =======================   ============

Mythen3 specific

=========================  ================= ====== =======================  ===============================  ============
Parameter index variable   asyn interface    Access drvInfo string           EPICS record name                Description
=========================  ================= ====== =======================  ===============================  ============
SDNumGates                 asynInt32         r/w    SD_NGATES                $(P)$(R)NumGates                 Number of gates if timing mode is gating
SDCounterMask              asynUInt32Digital r/w    SD_COUNTER_MASK          $(P)$(R)CounterMask              Mask of counters used
SDGate\ *n*\ Delay         asynFloat64       r/w    SD_GATE\ *n*\ _DELAY     $(P)$(R)Gate\ *n*\ Delay         Gate\ *n* delay in seconds
SDGate\ *n*\ Width         asynFloat64       r/w    SD_GATE\ *n*\ _WIDTH     $(P)$(R)Gate\ *n*\ Width         Gate\ *n* width in seconds
SDCounter\ *n*\ Threshold  asynInt32         r/w    SD_CNT\ *n*\ _THRESHOLD  $(P)$(R)Counter\ *n*\ Threshold  Counter\ *n* threshold in eV
=========================  ================= ====== =======================  ===============================  ============


Configuration
-------------

::

    # slsDetectorConfig (
    #               portName,       # The name of the asyn port driver to be created.
    #               configFileName, # The configuration file to the detector.
    #               detectorId,     # The detector index number running on the same system.
    #               numModules,     # The number of modules for a multi-module detector.
    #               maxBuffers,     # The maximum number of NDArray buffers that the NDArrayPool for this driver is 
    #                                 allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
    #               maxMemory)      # The maximum amount of memory that the NDArrayPool for this driver is 
    #                                 allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
    slsDetectorConfig("SD1", "cfg/mcs1x21.config", 0, 1, -1, -1)


Release Notes
-------------
* 9.2.0 - 05.06.2025
  1. slsDetector library updated to 9.2.0
  2. Mark detector disconnected when config fails.

* 9.0.0 - 27.11.2024
  1. slsDetector library updated to 9.0.0

* 8.0.2 - 17.10.2024
  1. slsDetector library updated to 8.0.2 built on RHEL8.

* 5.0.0 - 30.11.2020
  1. slsDetector library updated to 5.0.1

* 4.1.2 - 13.05.2020
  1. libSlsDetector updated to 4.1.1 with zmq statically builtin.

* 1.2 - 08.08.2014
  
  1. libSlsDetector updated.

* 1.1 - 10.12.2013

  1. libSlsDetector updated to r706.
  2. Avoid calling getDetectorStatus from two threads simultaneously.

* 1.0 - 18.09.2013

  1. Milestone release. It has been tested for MYTHEN and GOTTHARD detectors.detectors
