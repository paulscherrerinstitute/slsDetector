/* slsDetectorDriver.cpp
 *
 * This is a driver for slsDetector classes.
 *
 * Author: Xiaoqiang Wang
 *         Paul Scherrer Institut
 *
 * Created:  June 8, 2012
 *
 */
 
#include <ADDriver.h>
#include <asynNDArrayDriver.h>
#include <asynOctetSyncIO.h>

#include <iocsh.h>
#include <epicsExit.h>
#include <epicsThread.h>

#include "multiSlsDetector.h"

#include <epicsExport.h>

#define MAX_FILENAME_LEN 256

static const char *driverName = "slsDetectorDriver";

#define SDSettingString         "SD_SETTING"
#define SDTriggerDelayString    "SD_TRIGGER_DELAY"
#define SDThresholdString       "SD_THRESHOLD"
#define SDEnergyString          "SD_ENERGY"
#define SDOnlineString          "SD_ONLINE"
#define SDFlatFieldPathString   "SD_FLATFIELD_PATH"
#define SDFlatFieldFileString   "SD_FLATFIELD_FILE"
#define SDUseFlatFieldString    "SD_USE_FLATFIELD"
#define SDUseCountRateString    "SD_USE_COUNTRATE"
#define SDUsePixelMaskString    "SD_USE_PIXELMASK"
#define SDUseAngularConvString  "SD_USE_ANGULAR_CONV"
#define SDBitDepthString        "SD_BIT_DEPTH"
#define SDNumGatesString        "SD_NUM_GATES"
#define SDNumCyclesString       "SD_NUM_CYCLES"
#define SDNumFramesString       "SD_NUM_FRAMES"
#define SDTimingModeString      "SD_TMODE"
#define SDSetupFileString       "SD_SETUP_FILE"
#define SDLoadSetupString       "SD_LOAD_SETUP"
#define SDSaveSetupString       "SD_SAVE_SETUP"


/** Driver for Dectris Pilatus pixel array detectors using their camserver server over TCP/IP socket */
class slsDetectorDriver : public ADDriver {
public:
    slsDetectorDriver(const char *portName, const char *configFileName,
                    int maxBuffers, size_t maxMemory,
                    int priority, int stackSize);

    /* These are the methods that we override from ADDriver */
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
    virtual asynStatus writeOctet(asynUser *pasynUser, const char *value, 
                                    size_t nChars, size_t *nActual);
    virtual void report(FILE *fp, int details); 

    void dataCallback(detectorData *pData); /* This should be private but is called from C so must be public */
    void acquisitionTask(); 
    void shutdown(); 
 
 protected:
    int SDSetting;
    #define FIRST_SD_PARAM SDSetting
    int SDTriggerDelay; 
    int SDThreshold;
    int SDEnergy;
    int SDOnline;
    int SDFlatFieldPath;
    int SDFlatFieldFile;
    int SDUseFlatField;
    int SDUseCountRate;
    int SDUsePixelMask;
    int SDUseAngularConv;
    int SDBitDepth; 
    int SDNumGates; 
    int SDNumCycles; 
    int SDNumFrames; 
    int SDTimingMode; 
    int SDSetupFile; 
    int SDLoadSetup; 
    int SDSaveSetup; 
    #define LAST_SD_PARAM SDSaveSetup 

 private:                                       
    /* These are the methods that are new to this class */

    /* Our data */
    multiSlsDetector *pDetector; 
    epicsEventId startEventId;
};

#define NUM_SD_PARAMS (&LAST_SD_PARAM - &FIRST_SD_PARAM + 1)

static void c_shutdown(void* arg) {
    slsDetectorDriver *p = (slsDetectorDriver*)arg;
    p->shutdown(); 
}

int dataCallbackC(detectorData *pData, void *pArg) 
{
   if (pData == NULL)
       return 0; 
  
    if (pArg  != NULL) {
        slsDetectorDriver *pDetector = (slsDetectorDriver*)pArg; 
        pDetector->dataCallback(pData); 
    }
   return 0; 
}

void acquisitionTaskC(void *drvPvt)
{
    slsDetectorDriver *pDetector = (slsDetectorDriver*)drvPvt; 
    pDetector->acquisitionTask(); 
}

void slsDetectorDriver::shutdown()
{
    if (pDetector)
        delete pDetector; 
}

void slsDetectorDriver::acquisitionTask()
{
    int status = asynSuccess; 
    int acquire; 
    int detStatus; 
    static const char *functionName = "acquisitionTask";
    this->lock(); 

    while (1) {
        /* Is acquisition active? */
        getIntegerParam(ADAcquire, &acquire);
        
        /* If we are not acquiring then wait for a semaphore that is given when acquisition is started */
        if (!acquire) 
        {
            setIntegerParam(ADStatus, 0);
            callParamCallbacks();
            /* Release the lock while we wait for an event that says acquire has started, then lock again */
            asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
                "%s:%s: waiting for acquire to start\n", driverName, functionName);
            this->unlock();
            status = epicsEventWait(this->startEventId);
            this->lock();
            getIntegerParam(ADAcquire, &acquire);
        }

        /* Start acquisition,  this is a blocking function */
        this->unlock(); 
        pDetector->startMeasurement();
        this->lock(); 

        /* Wait for acquisition to complete, but allow acquire stop events to be handled */
        while (1) {
            /* Release the lock while waiting */
            this->unlock(); 
            epicsThreadSleep(0.2); 
            detStatus = pDetector->getRunStatus(); 
            this->lock(); 
            if (detStatus == 0 || detStatus == 1 || detStatus == 3) break; 
            /* Update detector status */
            setIntegerParam(ADStatus, detStatus);
            setIntegerParam(NDFileNumber,    pDetector->getFileIndex()); 
            callParamCallbacks(); 
        }
        /* Update detector status */
        setIntegerParam(ADStatus, pDetector->getRunStatus());
        setIntegerParam(NDFileNumber,    pDetector->getFileIndex()); 

        setIntegerParam(ADAcquire,  0); 
        callParamCallbacks(); 
    }
}

void slsDetectorDriver::dataCallback(detectorData *pData)
{
    NDArray *pImage; 
    int ndims = 2, dims[2];
    int totalBytes; 
    int imageCounter;
    int arrayCallbacks;
    epicsTimeStamp timeStamp; 

    if (pData == NULL || pData->values == NULL || pData->npoints <= 0) return; 

    this ->lock(); 

    dims[0] = pData->npoints; 
    dims[1] = pData->npy; 
    totalBytes = dims[0]*dims[1]*4; 
    if (dims[1] == 1) ndims = 1; 

    /* Get the current time */
    epicsTimeGetCurrent(&timeStamp); 

    /* Allocate a new image buffer */
    pImage = this->pNDArrayPool->alloc(ndims, dims, NDFloat32, totalBytes, NULL); 
    memcpy(pImage->pData,  pData->values, totalBytes); 
    pImage->ndims = ndims; 
    pImage->dims[0].size = dims[0]; 
    pImage->dims[0].offset = 0; 
    pImage->dims[0].binning = 1; 
    pImage->dims[1].size = dims[1]; 
    pImage->dims[1].offset = 0; 
    pImage->dims[1].binning = 1; 

    /* Increase image counter */
    getIntegerParam(NDArrayCounter, &imageCounter);
    imageCounter++;
    setIntegerParam(NDArrayCounter, imageCounter);

    /* Set the uniqueId and time stamp */
    pImage->uniqueId = imageCounter; 
    pImage->timeStamp = timeStamp.secPastEpoch + timeStamp.nsec / 1e9; 

    /* Get any attributes that have been defined for this driver */        
    this->getAttributes(pImage->pAttributeList);

    getIntegerParam(NDArrayCallbacks, &arrayCallbacks);
    if (arrayCallbacks) {
        /* Call the NDArray callback */
        /* Must release the lock here, or we can get into a deadlock, because we can
         * block on the plugin lock, and the plugin can be calling us */
        this->unlock();
        doCallbacksGenericPointer(pImage, NDArrayData, 0);
        this->lock();
    }

    /* We save the most recent good image buffer so it can be used in the
     * readADImage function.  Now release it. */
    if (this->pArrays[0]) this->pArrays[0]->release();
    this->pArrays[0] = pImage;

    /* Update any changed parameters */
    callParamCallbacks();

    this->unlock();
}

/** Called when asyn clients call pasynInt32->write().
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. */
asynStatus slsDetectorDriver::writeOctet(asynUser *pasynUser, const char *value,
                                            size_t nChars, size_t *nActual)
{
    int function = pasynUser->reason;
    int status = asynSuccess;
    const char *functionName = "writeOctet";

    /* Set the parameter in the parameter library. */
    status |= (asynStatus)setStringParam(function, (char *)value);

    if (function == NDFilePath) {
        pDetector->setFilePath(value); 
        status |= setStringParam(NDFilePath,
                pDetector->getFilePath().c_str()); 
        this->checkPath(); 
    } else if (function == NDFileName) {
        pDetector->setFileName(value); 
        status |= setStringParam(NDFileName,
                    pDetector->getFileName().c_str()); 
    } else if (function == SDFlatFieldPath) {
        pDetector->setFlatFieldCorrectionDir(value); 
        status |= setStringParam(SDFlatFieldPath,
                    pDetector->getFlatFieldCorrectionDir().c_str()); 
    } else if (function == SDFlatFieldFile) {
        pDetector->setFlatFieldCorrectionFile(value); 
        status |= setStringParam(SDFlatFieldPath,
                    pDetector->getFlatFieldCorrectionFile().c_str()); 
    } else {
        /* If this is not a parameter we have handled call the base class */
        if (function < FIRST_SD_PARAM) 
            status = ADDriver::writeOctet(pasynUser, value,nChars, nActual);
    }
 
    /* Update any changed parameters */
    callParamCallbacks();

    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:%s: error, status=%d function=%d, value=%s\n", 
              driverName, functionName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, value=%s\n", 
              driverName, functionName, function, value);

    *nActual = nChars;
    return((asynStatus)status); 
}

/** Called when asyn clients call pasynInt32->write().
  * This function performs actions for some parameters, including ADAcquire, ADBinX, etc.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. */
asynStatus slsDetectorDriver::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    int status = asynSuccess;
    char filePath[MAX_FILENAME_LEN];
    int minX=0, minY=0, sizeX=1, sizeY=1;  
    static const char *functionName = "writeInt32";

    /* Set the parameter and readback in the parameter library.  This may be overwritten when we read back the
     * status at the end, but that's OK */
    status |= setIntegerParam(function, value);

    if (function == ADMinX ||
        function == ADMinY ||
        function == ADSizeX ||
        function == ADSizeY) {
        getIntegerParam(ADMinX, &minX); 
        getIntegerParam(ADMinY, &minY); 
        getIntegerParam(ADSizeX, &sizeX); 
        getIntegerParam(ADSizeY, &sizeY); 
        pDetector->setDetectorSize(minX, minY, sizeX, sizeY); 

        pDetector->getDetectorSize(minX, minY, sizeX, sizeY); 
        status |= setIntegerParam(ADMinX,  minX);
        status |= setIntegerParam(ADMinY,  minY);
        status |= setIntegerParam(ADSizeX, sizeX);
        status |= setIntegerParam(ADSizeY, sizeY);
        status |= setIntegerParam(NDArraySizeX, sizeX);
        status |= setIntegerParam(NDArraySizeY, sizeY);
    } else if (function == NDFileNumber) {
        pDetector->setFileIndex(value); 
        status |= setIntegerParam(NDFileNumber,    pDetector->getFileIndex()); 
    } else if (function == SDSetting) {
        //pDetector->setSettings((slsDetectorDefs::detectorSettings)value); 
        status |= setIntegerParam(SDSetting,       pDetector->setSettings(slsDetectorDefs::GET_SETTINGS)); 
    } else if (function == SDThreshold) {
        pDetector->setThresholdEnergy(value); 
        status |= setIntegerParam(SDThreshold,     pDetector->getThresholdEnergy()); 
    } else if (function == SDEnergy) {
        pDetector->setBeamEnergy(value); 
        status |= setIntegerParam(SDEnergy,        pDetector->getBeamEnergy()); 
        status |= setIntegerParam(SDThreshold,     pDetector->getThresholdEnergy()); 
    } else if (function  == SDOnline) {
        pDetector->setOnline(value); 
        status |= setIntegerParam(SDOnline,        pDetector->setOnline()); 
    } else if (function == SDUseFlatField) {
        pDetector->enableFlatFieldCorrection(value); 
        status |= setIntegerParam(SDUseFlatField,  pDetector->enableFlatFieldCorrection()); 
    } else if (function == SDUseCountRate) {
        pDetector->enableCountRateCorrection(value); 
        status |= setIntegerParam(SDUseCountRate,  pDetector->enableCountRateCorrection()); 
    } else if (function == SDUsePixelMask) {
        pDetector->enablePixelMaskCorrection(value); 
        status |= setIntegerParam(SDUsePixelMask,  pDetector->enablePixelMaskCorrection()); 
    } else if (function == SDUseAngularConv) {
        pDetector->enableAngularConversion(value); 
        status |= setIntegerParam(SDUseAngularConv,pDetector->enableAngularConversion()); 
    } else if (function == SDBitDepth) {
        pDetector->setBitDepth(value); 
        status |= setIntegerParam(SDBitDepth,      pDetector->setBitDepth()); 
    } else if (function == SDNumGates) {
        pDetector->setNumberOfGates(value);
        status |= setIntegerParam(SDNumGates,      pDetector->setNumberOfGates()); 
    } else if (function == SDNumCycles) {
        pDetector->setNumberOfCycles(value); 
        status |= setIntegerParam(SDNumCycles,     pDetector->setNumberOfCycles()); 
    } else if (function == SDNumFrames) {
        pDetector->setNumberOfFrames(value); 
        status |= setIntegerParam(SDNumFrames,     pDetector->setNumberOfFrames()); 
    }else if (function == SDTimingMode) {
        pDetector->setTimingMode(value); 
        status |= setIntegerParam(SDTimingMode,    pDetector->setTimingMode()); 
    } else if (function == SDLoadSetup) {
        getStringParam(SDSetupFile, sizeof(filePath), filePath);
        pDetector->retrieveDetectorSetup(filePath); 
    } else if (function == SDSaveSetup) {
        getStringParam(SDSetupFile, sizeof(filePath), filePath);
        pDetector->dumpDetectorSetup(filePath); 
    } else if (function == ADAcquire) {
        int runStatus = pDetector->getRunStatus(); 
        if (value) {
            if (runStatus != 0  && runStatus  !=  3)
                /* Detector not ready */
                setIntegerParam(ADAcquire, 0); 
            else
                /* Send an event to wake up the acquisition task.  */
                epicsEventSignal(this->startEventId);
        }
        if (!value)
            /* Stop measurement */
            pDetector->stopMeasurement(); 
    } else {
        /* If this is not a parameter we have handled call the base class */
        if (function < FIRST_SD_PARAM) status = ADDriver::writeInt32(pasynUser, value);
    }
   
    /* Update any changed parameters */
    callParamCallbacks();

    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:%s: error, status=%d function=%d, value=%d\n", 
              driverName, functionName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, value=%d\n", 
              driverName, functionName, function, value);
    return((asynStatus)status); 
}

/** Called when asyn clients call pasynFloat64->write().
  * For all  parameters it  sets the value in the parameter library and calls any registered callbacks.
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. */
asynStatus slsDetectorDriver::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
    int function = pasynUser->reason;
    int status = asynSuccess;
    int addr = 0;
    const char* functionName = "writeFloat64";

    status = getAddress(pasynUser, &addr); 
    if (status != asynSuccess) return((asynStatus)status);
    /* Set the parameter in the parameter library. */
    status = (asynStatus) setDoubleParam(addr, function, value);

    if (function == ADAcquireTime) {
        pDetector->setExposureTime((int64_t)(value * 1e+9)); 
        status |= setDoubleParam(ADAcquireTime,    pDetector->setExposureTime()/1e+9);
    } else if (function == ADAcquirePeriod) {
        pDetector->setExposurePeriod((int64_t)(value * 1e+9)); 
        status |= setDoubleParam(ADAcquirePeriod,  pDetector->setExposurePeriod()/1e+9); 
    } else if (function == SDTriggerDelay) {
        pDetector->setDelayAfterTrigger((int64_t)(value * 1e+9)); 
        status |= setDoubleParam(SDTriggerDelay,   pDetector->setDelayAfterTrigger()/1e+9); 
    } else {
        /* If this is not a parameter we have handled call the base class */
        if (function < NUM_SD_PARAMS) status = ADDriver::writeFloat64(pasynUser, value);
    }

    /* Update any changed parameters */
    callParamCallbacks();

    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:%s: error, status=%d function=%d, value=%g\n", 
              driverName, functionName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, value=%g\n", 
              driverName, functionName, function, value);
    return((asynStatus)status); 
}


/** Report status of the driver.
  * Prints details about the driver if details>0.
  * It then calls the ADDriver::report() method.
  * \param[in] fp File pointed passed by caller where the output is written to.
  * \param[in] details If >0 then driver details are printed.
  */
void slsDetectorDriver::report(FILE *fp, int details)
{
    fprintf(fp, "slsDetectorDriver %s\n", this->portName);
    if (details > 0) {
        int nx, ny, dataType;
        getIntegerParam(ADSizeX, &nx);
        getIntegerParam(ADSizeY, &ny);
        getIntegerParam(NDDataType, &dataType);
        fprintf(fp, "  NX, NY:            %d  %d\n", nx, ny);
        fprintf(fp, "  Data type:         %d\n", dataType);
    }
    /* Invoke the base class method */
    ADDriver::report(fp, details);
}

extern "C" int slsDetectorConfig(const char *portName, const char *configFileName, 
                                    int maxBuffers, size_t maxMemory,
                                    int priority, int stackSize)
{
    new slsDetectorDriver(portName, configFileName, maxBuffers, maxMemory,
                        priority, stackSize);
    return(asynSuccess);
}

/** Constructor for slsDetectorDriver driver; most parameters are simply passed to ADDriver::ADDriver.
  * After calling the base class constructor this method creates a thread to collect the detector data, 
  * and sets reasonable default values for the parameters defined in this class, asynNDArrayDriver, and ADDriver.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] configFileName The configuration file to the detector.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] maxBuffers The maximum number of NDArray buffers that the NDArrayPool for this driver is 
  *            allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is 
  *            allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  */
slsDetectorDriver::slsDetectorDriver(const char *portName, const char *configFileName,
                                int maxBuffers, size_t maxMemory,
                                int priority, int stackSize)

    : ADDriver(portName, 1, NUM_SD_PARAMS, maxBuffers, maxMemory,
               0, 0,             /* No interfaces beyond those set in ADDriver.cpp */
               ASYN_CANBLOCK | ASYN_MULTIDEVICE, 1, /* ASYN_CANBLOCK=1, ASYN_MULTIDEVICE=1, autoConnect=1 */
               priority, stackSize), pDetector(NULL)

{
    int status = asynSuccess;
    const char *functionName = "slsDetectorDriver";

    /* Create the epicsEvents for signaling to the slsDetector task when acquisition starts and stops */
    this->startEventId = epicsEventCreate(epicsEventEmpty);
    if (!this->startEventId) {
        printf("%s:%s epicsEventCreate failure for start event\n", 
            driverName, functionName);
        return;
    }

    createParam(SDSettingString,        asynParamInt32,  &SDSetting); 
    createParam(SDTriggerDelayString,   asynParamFloat64,&SDTriggerDelay); 
    createParam(SDThresholdString,      asynParamInt32,  &SDThreshold); 
    createParam(SDEnergyString,         asynParamInt32,  &SDEnergy); 
    createParam(SDOnlineString,         asynParamInt32,  &SDOnline); 
    createParam(SDFlatFieldPathString,  asynParamOctet,  &SDFlatFieldPath); 
    createParam(SDFlatFieldFileString,  asynParamOctet,  &SDFlatFieldFile); 
    createParam(SDUseFlatFieldString,   asynParamInt32,  &SDUseFlatField); 
    createParam(SDUseCountRateString,   asynParamInt32,  &SDUseCountRate); 
    createParam(SDUsePixelMaskString,   asynParamInt32,  &SDUsePixelMask); 
    createParam(SDUseAngularConvString, asynParamInt32,  &SDUseAngularConv); 
    createParam(SDBitDepthString,       asynParamInt32,  &SDBitDepth); 
    createParam(SDNumGatesString,       asynParamInt32,  &SDNumGates); 
    createParam(SDNumCyclesString,      asynParamInt32,  &SDNumCycles); 
    createParam(SDNumFramesString,      asynParamInt32,  &SDNumFrames); 
    createParam(SDTimingModeString,     asynParamInt32,  &SDTimingMode); 
    createParam(SDSetupFileString,      asynParamOctet,  &SDSetupFile); 
    createParam(SDLoadSetupString,      asynParamInt32,  &SDLoadSetup); 
    createParam(SDSaveSetupString,      asynParamInt32,  &SDSaveSetup); 

    /* Connect to camserver */
    pDetector = new multiSlsDetector(); 
    if (pDetector->readConfigurationFile(configFileName)  ==  multiSlsDetector::FAIL) {
        status = asynError; 
        printf("%s:%s: ERROR: slsDetectorDriver::readConfigurationFile %s failed, status=%d\n", 
            driverName, functionName, configFileName, status);
    }

    /* Set some default values for parameters */
    status =  setStringParam (ADManufacturer, pDetector->getDetectorDeveloper().c_str());
    status |= setStringParam (ADModel,        pDetector->getDetectorType().c_str());

    int sensorSizeX,  sensorSizeY; 
    pDetector->getMaximumDetectorSize(sensorSizeX, sensorSizeY);
    status |= setIntegerParam(ADMaxSizeX, sensorSizeX);
    status |= setIntegerParam(ADMaxSizeY, sensorSizeY);

    status |= setIntegerParam(NDArraySize, 0);
    status |= setIntegerParam(NDDataType,  NDFloat32);
    
    status |= setStringParam(NDFilePath,       pDetector->getFilePath().c_str()); 
    status |= setStringParam(NDFileName,       pDetector->getFileName().c_str()); 

    status |= setStringParam(SDFlatFieldPath,  pDetector->getFlatFieldCorrectionDir().c_str()); 
    status |= setStringParam(SDFlatFieldPath,  pDetector->getFlatFieldCorrectionFile().c_str()); 
 
    status |= setIntegerParam(ADStatus,        pDetector->getRunStatus());

    callParamCallbacks();

    if (status) {
        printf("%s: unable to read camera parameters\n", functionName);
        return;
    }

    /* Register data callback function */
    pDetector->registerDataCallback(dataCallbackC,  (void *)this); 
   
    /* Register the shutdown function for epicsAtExit */
    epicsAtExit(c_shutdown, (void*)this); 

    /* Create the thread that runs acquisition */
    status = (epicsThreadCreate("acquisitionTask",
                                epicsThreadPriorityMedium,
                                epicsThreadGetStackSize(epicsThreadStackMedium),
                                (EPICSTHREADFUNC)acquisitionTaskC,
                                this) == NULL);
}

/* Code for iocsh registration */
static const iocshArg slsDetectorConfigArg0 = {"Port name", iocshArgString};
static const iocshArg slsDetectorConfigArg1 = {"config file name", iocshArgString};
static const iocshArg slsDetectorConfigArg2 = {"maxBuffers", iocshArgInt};
static const iocshArg slsDetectorConfigArg3 = {"maxMemory", iocshArgInt};
static const iocshArg slsDetectorConfigArg4 = {"priority", iocshArgInt};
static const iocshArg slsDetectorConfigArg5 = {"stackSize", iocshArgInt};
static const iocshArg * const slsDetectorConfigArgs[] =  {&slsDetectorConfigArg0,
                                                              &slsDetectorConfigArg1,
                                                              &slsDetectorConfigArg2,
                                                              &slsDetectorConfigArg3,
                                                              &slsDetectorConfigArg4,
                                                              &slsDetectorConfigArg5};
static const iocshFuncDef configSlsDetector = {"slsDetectorConfig", 6, slsDetectorConfigArgs};
static void configSlsDetectorCallFunc(const iocshArgBuf *args)
{
    slsDetectorConfig(args[0].sval, args[1].sval, 
            args[2].ival, args[3].ival, args[4].ival,  args[5].ival);
}


static void slsDetectorRegister(void)
{

    iocshRegister(&configSlsDetector, configSlsDetectorCallFunc);
}

extern "C" {
epicsExportRegistrar(slsDetectorRegister);
}

