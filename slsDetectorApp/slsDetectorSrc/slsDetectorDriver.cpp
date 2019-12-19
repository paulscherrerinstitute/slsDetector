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

#include <stdlib.h>

#include <ADDriver.h>
#include <asynNDArrayDriver.h>
#include <asynOctetSyncIO.h>

#include <iocsh.h>
#include <epicsExit.h>
#include <epicsString.h>
#include <epicsThread.h>

#include "slsDetectorUsers.h"
#include "detectorData.h"

#include <epicsExport.h>

#define MAX_FILENAME_LEN 256
#define MAX_COMMAND_ARG 20

static const char *driverName = "slsDetectorDriver";

#define SDSettingString         "SD_SETTING"
#define SDDelayTimeString       "SD_DELAY_TIME"
#define SDThresholdString       "SD_THRESHOLD"
#define SDEnergyString          "SD_ENERGY"
#define SDOnlineString          "SD_ONLINE"
#define SDFlatFieldPathString   "SD_FLATFIELD_PATH"
#define SDFlatFieldFileString   "SD_FLATFIELD_FILE"
#define SDUseFlatFieldString    "SD_USE_FLATFIELD"
#define SDUseCountRateString    "SD_USE_COUNTRATE"
#define SDUsePixelMaskString    "SD_USE_PIXELMASK"
#define SDUseAngularConvString  "SD_USE_ANGULAR_CONV"
#define SDUseDataCallbackString "SD_USE_DATA_CALLBACK"
#define SDBitDepthString        "SD_BIT_DEPTH"
#define SDNumGatesString        "SD_NUM_GATES"
#define SDNumCyclesString       "SD_NUM_CYCLES"
#define SDNumFramesString       "SD_NUM_FRAMES"
#define SDTimingModeString      "SD_TMODE"
#define SDRecvModeString        "SD_RECV_MODE"
#define SDRecvStreamString      "SD_RECV_STREAM"
#define SDHighVoltageString     "SD_HIGH_VOLTAGE"
#define SDCommandString         "SD_COMMAND"
#define SDReplyString           "SD_REPLY"
#define SDSetupFileString       "SD_SETUP_FILE"
#define SDLoadSetupString       "SD_LOAD_SETUP"
#define SDSaveSetupString       "SD_SAVE_SETUP"


/** Driver for sls array detectors using over TCP/IP socket */
class slsDetectorDriver : public ADDriver {
public:
    slsDetectorDriver(const char *portName, const char *configFileName, int detectorId,
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
    int SDDelayTime;
    int SDThreshold;
    int SDEnergy;
    int SDOnline;
    int SDFlatFieldPath;
    int SDFlatFieldFile;
    int SDUseFlatField;
    int SDUseCountRate;
    int SDUsePixelMask;
    int SDUseAngularConv;
    int SDUseDataCallback;
    int SDBitDepth;
    int SDNumGates;
    int SDNumCycles;
    int SDNumFrames;
    int SDTimingMode;
    int SDRecvMode;
    int SDRecvStream;
    int SDHighVoltage;
    int SDCommand;
    int SDReply;
    int SDSetupFile;
    int SDLoadSetup;
    int SDSaveSetup;
    #define LAST_SD_PARAM SDSaveSetup

 private:
    /* These are the methods that are new to this class */

    /* Our data */
    slsDetectorUsers *pDetector;
    epicsEventId startEventId;
};

#define NUM_SD_PARAMS (&LAST_SD_PARAM - &FIRST_SD_PARAM + 1)

static void c_shutdown(void* arg) {
    slsDetectorDriver *p = (slsDetectorDriver*)arg;
    p->shutdown();
}

/* Dummy implementation */
int dummyFinishedCallbackC(double progress, int status, void *pArg)
{
    return 0;
}

int dataCallbackC(detectorData *pData, int n, int s, void *pArg)
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
    int imageMode;
    char filePath[MAX_FILENAME_LEN];
    char fileName[MAX_FILENAME_LEN];
    int  fileNumber;
    char fullFileName[MAX_FILENAME_LEN];
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

        /* Poll detector temperature */
        setDoubleParam(ADTemperatureActual, pDetector->getADC("temp_fpga")/1000.);
        callParamCallbacks();

        /* Start acquisition,  this is a blocking function */
        this->unlock();
        pDetector->startMeasurement();
        this->lock();

        /* Update detector status */
        setIntegerParam(ADStatus, pDetector->getDetectorStatus());
        fileNumber = pDetector->getFileIndex();
        setIntegerParam(NDFileNumber, fileNumber);

        /* Compose last saved file name
         * Not using FileTemplate because it is builtin slsDetector library
         * */
        getStringParam(NDFilePath, sizeof(filePath), filePath);
        getStringParam(NDFileName, sizeof(fileName), fileName);
        getIntegerParam(NDFileNumber, &fileNumber);
        epicsSnprintf(fullFileName, MAX_FILENAME_LEN, "%s%s_%d", filePath, fileName, fileNumber-1);
        setStringParam(NDFullFileName, fullFileName);

        getIntegerParam(ADImageMode, &imageMode);
        if (imageMode == ADImageSingle || imageMode == ADImageMultiple) {
            setIntegerParam(ADAcquire,  0);
            callParamCallbacks();
        }
    }
}

void slsDetectorDriver::dataCallback(detectorData *pData)
{
    NDArray *pImage;
    int ndims = 2;
    size_t dims[2];
    int totalBytes = 0;
    NDDataType_t dtype = NDFloat64;
    void *pBuffer = NULL;
    int imageCounter;
    int arrayCallbacks;
    epicsTimeStamp timeStamp;
    epicsInt32 colorMode = NDColorModeMono;
    const char *functionName = "dataCallback";

    if (pData == NULL || (pData->values == NULL && pData->cvalues == NULL) || pData->npoints <= 0) return;

    this ->lock();

    dims[0] = pData->npoints;
    dims[1] = pData->npy;
    if (pData->values) {
        totalBytes = dims[0]*dims[1]*8;
        dtype = NDFloat64;
        pBuffer = pData->values;
    }
    else if (pData->cvalues){
        totalBytes = pData->databytes;
        switch (pData->dynamicRange / 8) {
            case 1:
                dtype = NDUInt8;
                break;
            case 2:
                dtype = NDUInt16;
                break;
            case 4:
                dtype = NDUInt32;
                break;
            default:
                asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                      "%s:%s: unsupported dynamic range %d\n",
                      driverName, functionName, pData->dynamicRange);
                return;
        }
        pBuffer = pData->cvalues;
    }
    if (dims[1] == 1) ndims = 1;

    /* Get the current time */
    epicsTimeGetCurrent(&timeStamp);

    /* Allocate a new image buffer */
    pImage = this->pNDArrayPool->alloc(ndims, dims, dtype, totalBytes, NULL);
    memcpy(pImage->pData,  pBuffer, totalBytes);
    pImage->dataType = dtype;
    pImage->ndims = ndims;
    pImage->dims[0].size = dims[0];
    pImage->dims[0].offset = 0;
    pImage->dims[0].binning = 1;
    pImage->dims[1].size = dims[1];
    pImage->dims[1].offset = 0;
    pImage->dims[1].binning = 1;

    pImage->pAttributeList->add("ColorMode", "Color Mode", NDAttrInt32, &colorMode);

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

    /* Reject any call to the detector if it is running */
    int acquire;
    getIntegerParam(ADAcquire, &acquire);
    if (acquire == 1) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
        "%s:%s: detector is busy\n", driverName, functionName);
        return asynError;
    }

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
        if (value && strlen(value) != 0) {
            pDetector->setFlatFieldCorrectionDir(value);
        }
        status |= setStringParam(SDFlatFieldPath,
                    pDetector->getFlatFieldCorrectionDir().c_str());
    } else if (function == SDFlatFieldFile) {
        if (value && strlen(value) != 0) {
            pDetector->setFlatFieldCorrectionFile(value);
        }
        status |= setStringParam(SDFlatFieldPath,
                    pDetector->getFlatFieldCorrectionFile().c_str());
    } else if (function == SDCommand) {
        if (value && strlen(value) != 0) {
            char *output = epicsStrDup(value);
            char *str, *token, *saveptr;
            char *argv[MAX_COMMAND_ARG];
            int argc = 0;
            int pos = -1;
            size_t n = 0;
            for(str=output; ;str=NULL) {
                token = epicsStrtok_r(str, " ", &saveptr);
                if (token == NULL)
                    break;
                if (argc >= MAX_COMMAND_ARG) {
                    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s:%s: Command has more than %d parameters: %s\n", driverName, functionName, MAX_COMMAND_ARG, value);
                    break;
                }
                /* extract module number (0:) from the second argument */
                if (argc == 1 && (n = strspn(token, "1234567890")) && token[n] == ':') {
                    pos = atoi(token);
                    token += n + 1;
                }
                argv[argc++] = token;
            }
            std::string reply;
            if (argc >= 2) {
                char **params = argv + 1;
                int num = argc - 1;
                if (epicsStrCaseCmp(argv[0], "get") == 0)
                    reply = pDetector->getCommand(num, params, pos);
                else if (epicsStrCaseCmp(argv[0], "put") == 0)
                    reply = pDetector->putCommand(num, params, pos);
                else
                    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s:%s: Command type is neither \"put\" nor \"get\": %s\n", driverName, functionName, argv[0]);
            } else
                asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: Command expects more than 2 parameters: got %d\n", driverName, functionName, argc);

            free(output);
            status |= setStringParam(SDReply, reply.c_str());
        }
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
    static int threshold = -1;
    int retVal;
    static const char *functionName = "writeInt32";

    /* Reject any call to the detector if it is running */
    int acquire;
    getIntegerParam(ADAcquire, &acquire);
    if (function != ADAcquire && acquire == 1) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
        "%s:%s: detector is busy\n", driverName, functionName);
        return asynError;
    }

    /* Set the parameter and readback in the parameter library.
     * This may be overwritten when we read back the
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
        retVal = pDetector->setFileIndex(value);
        status |= setIntegerParam(NDFileNumber, retVal);
    } else if (function == SDSetting) {
        retVal = pDetector->setSettings(value);
        /* map undefined and uninitialized settings from 200 to 14 */
	if (retVal >= 200) {
            retVal -= 186;
	}
        status |= setIntegerParam(SDSetting, retVal);
        /* setSettings override current threshhold, recover it with user's value */
        if (threshold != -1) {
            pDetector->setThresholdEnergy(threshold);
        }
        status |= setIntegerParam(SDThreshold, pDetector->getThresholdEnergy());
    } else if (function == SDThreshold) {
        /* note down user's set value and recover it when settings change */
        threshold = value;
        retVal = pDetector->setThresholdEnergy(value);
        status |= setIntegerParam(SDThreshold, pDetector->getThresholdEnergy());
    } else if (function == SDEnergy) {
        /* Threshold energy is automatically set to half of the beam energy */
        retVal = pDetector->setThresholdEnergy(value / 2);
        status |= setIntegerParam(SDThreshold, retVal);
        status |= setIntegerParam(SDEnergy, value);
    } else if (function  == SDOnline) {
        retVal = pDetector->setOnline(value);
        status |= setIntegerParam(SDOnline, retVal);
    } else if (function == SDUseFlatField) {
        retVal = pDetector->enableFlatFieldCorrection(value);
        status |= setIntegerParam(SDUseFlatField, retVal);
    } else if (function == SDUseCountRate) {
        retVal = pDetector->enableCountRateCorrection(value);
        status |= setIntegerParam(SDUseCountRate, retVal);
    } else if (function == SDUsePixelMask) {
        retVal = pDetector->enablePixelMaskCorrection(value);
        status |= setIntegerParam(SDUsePixelMask, retVal);
    } else if (function == SDUseAngularConv) {
        retVal = pDetector->enableAngularConversion(value);
        status |= setIntegerParam(SDUseAngularConv,retVal);
    } else if (function == SDUseDataCallback) {
        if (pDetector->enableDataStreamingToClient(-1) != value) {
            retVal = pDetector->enableDataStreamingToClient(value);
            status |= setIntegerParam(SDUseDataCallback, retVal);
            status |= setIntegerParam(SDRecvStream, pDetector->enableDataStreamingFromReceiver(-1));
        }
    } else if (function == SDBitDepth) {
        retVal = pDetector->setBitDepth(value);
        status |= setIntegerParam(SDBitDepth, retVal);
    } else if (function == SDNumGates) {
        retVal = pDetector->setNumberOfGates(value);
        status |= setIntegerParam(SDNumGates, retVal);
    } else if (function == SDNumCycles) {
        retVal = pDetector->setNumberOfCycles(value);
        status |= setIntegerParam(SDNumCycles, retVal);
    } else if (function == SDNumFrames) {
        retVal = pDetector->setNumberOfFrames(value);
        status |= setIntegerParam(SDNumFrames, retVal);
    } else if (function == SDTimingMode) {
        retVal = pDetector->setTimingMode(value);
        status |= setIntegerParam(SDTimingMode, retVal);
    } else if (function == SDLoadSetup) {
        getStringParam(SDSetupFile, sizeof(filePath), filePath);
        if (pDetector->retrieveDetectorSetup(filePath) != 0)
            status |= asynError;
        setIntegerParam(SDLoadSetup, 0);
    } else if (function == SDSaveSetup) {
        getStringParam(SDSetupFile, sizeof(filePath), filePath);
        if (pDetector->dumpDetectorSetup(filePath) != 0)
            status |= asynError;
        setIntegerParam(SDSaveSetup, 0);
    } else if (function == ADAcquire) {
        if (value) {
            int runStatus = pDetector->getDetectorStatus();
            if (runStatus != 0  && runStatus != 3 && runStatus != 6) {
                /* Detector not ready */
                setIntegerParam(ADAcquire, 0);
                asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s:Acquire: detector not ready status=%s\n", driverName, functionName, pDetector->runStatusType(runStatus).c_str());
            }
            else
                /* Send an event to wake up the acquisition task.  */
                epicsEventSignal(this->startEventId);
        } else {
            /* Stop measurement.
             * If in continuous mode, let the current acquisition fininish, because 
             * abort has a high chance of hangup as of slsDetector library v4.1.0 */
            int imageMode;
            getIntegerParam(ADImageMode, &imageMode);
            if (imageMode != ADImageContinuous)
                pDetector->stopMeasurement();
        }
    } else if (function ==  NDAutoSave) {
        int autoSave = pDetector->enableWriteToFile(value);
        status |= setIntegerParam(NDAutoSave, autoSave);
    } else if (function == SDRecvMode) {
        int recvMode = pDetector->setReceiverMode(value);
        status |= setIntegerParam(SDRecvMode, recvMode);
    } else if (function == SDRecvStream) {
        if (pDetector->enableDataStreamingFromReceiver(-1) != value) {
            int recvStream = pDetector->enableDataStreamingFromReceiver(value);
            status |= setIntegerParam(SDRecvStream, recvStream);
        }
    } else if (function == SDHighVoltage) {
        pDetector->setHighVoltage(value);
        status |= setIntegerParam(SDHighVoltage,    pDetector->setHighVoltage(-1));
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

    /* Reject any call to the detector if it is running */
    int acquire;
    getIntegerParam(ADAcquire, &acquire);
    if (acquire == 1) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
        "%s:%s: detector is busy\n", driverName, functionName);
        return asynError;
    }

    /* Set the parameter in the parameter library. */
    status = (asynStatus) setDoubleParam(addr, function, value);

    if (function == ADAcquireTime) {
        pDetector->setExposureTime(value, true);
        status |= setDoubleParam(ADAcquireTime,    pDetector->setExposureTime(-1, true));
    } else if (function == ADAcquirePeriod) {
        pDetector->setExposurePeriod(value, true);
        status |= setDoubleParam(ADAcquirePeriod,  pDetector->setExposurePeriod(-1, true));
    } else if (function == SDDelayTime) {
        pDetector->setDelayAfterTrigger(value, true);
        status |= setDoubleParam(SDDelayTime,   pDetector->setDelayAfterTrigger(-1, true));
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

extern "C" int slsDetectorConfig(const char *portName, const char *configFileName, int detectorId,
                                    int maxBuffers, size_t maxMemory,
                                    int priority, int stackSize)
{
    new slsDetectorDriver(portName, configFileName, detectorId,
            maxBuffers, maxMemory, priority, stackSize);
    return(asynSuccess);
}

/** Constructor for slsDetectorDriver driver; most parameters are simply passed to ADDriver::ADDriver.
  * After calling the base class constructor this method creates a thread to collect the detector data,
  * and sets reasonable default values for the parameters defined in this class, asynNDArrayDriver, and ADDriver.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] configFileName The configuration file to the detector.
  * \param[in] detectorId The detector index number running on the same system.
  * \param[in] maxBuffers The maximum number of NDArray buffers that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  */
slsDetectorDriver::slsDetectorDriver(const char *portName, const char *configFileName, int detectorId,
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
    createParam(SDDelayTimeString,      asynParamFloat64,&SDDelayTime);
    createParam(SDThresholdString,      asynParamInt32,  &SDThreshold);
    createParam(SDEnergyString,         asynParamInt32,  &SDEnergy);
    createParam(SDOnlineString,         asynParamInt32,  &SDOnline);
    createParam(SDFlatFieldPathString,  asynParamOctet,  &SDFlatFieldPath);
    createParam(SDFlatFieldFileString,  asynParamOctet,  &SDFlatFieldFile);
    createParam(SDUseFlatFieldString,   asynParamInt32,  &SDUseFlatField);
    createParam(SDUseCountRateString,   asynParamInt32,  &SDUseCountRate);
    createParam(SDUsePixelMaskString,   asynParamInt32,  &SDUsePixelMask);
    createParam(SDUseAngularConvString, asynParamInt32,  &SDUseAngularConv);
    createParam(SDUseDataCallbackString,asynParamInt32,  &SDUseDataCallback);
    createParam(SDBitDepthString,       asynParamInt32,  &SDBitDepth);
    createParam(SDNumGatesString,       asynParamInt32,  &SDNumGates);
    createParam(SDNumCyclesString,      asynParamInt32,  &SDNumCycles);
    createParam(SDNumFramesString,      asynParamInt32,  &SDNumFrames);
    createParam(SDTimingModeString,     asynParamInt32,  &SDTimingMode);
    createParam(SDRecvModeString,       asynParamInt32,  &SDRecvMode);
    createParam(SDRecvStreamString,     asynParamInt32,  &SDRecvStream);
    createParam(SDHighVoltageString,    asynParamInt32,  &SDHighVoltage);
    createParam(SDCommandString,        asynParamOctet,  &SDCommand);
    createParam(SDReplyString,          asynParamOctet,  &SDReply);
    createParam(SDSetupFileString,      asynParamOctet,  &SDSetupFile);
    createParam(SDLoadSetupString,      asynParamInt32,  &SDLoadSetup);
    createParam(SDSaveSetupString,      asynParamInt32,  &SDSaveSetup);

    /* Connect to camserver */
    pDetector = new slsDetectorUsers(detectorId);
    if (pDetector->readConfigurationFile(configFileName) != 0) {
        status = asynError;
        printf("%s:%s: ERROR: slsDetectorDriver::readConfigurationFile %s failed, status=%d\n",
            driverName, functionName, configFileName, status);
    }

    /* Set some default values for parameters */
    status |= setIntegerParam(SDOnline, pDetector->setOnline(1));

    status =  setStringParam (ADManufacturer, pDetector->getDetectorDeveloper().c_str());
    status |= setStringParam (ADModel,        pDetector->getDetectorType().c_str());

    int sensorSizeX,  sensorSizeY;
    pDetector->getMaximumDetectorSize(sensorSizeX, sensorSizeY);
    status |= setIntegerParam(ADMaxSizeX, sensorSizeX);
    status |= setIntegerParam(ADMaxSizeY, sensorSizeY);

    int minX,  minY, sizeX, sizeY;
    pDetector->getDetectorSize(minX, minY, sizeX, sizeY);
    status |= setIntegerParam(ADMinX,  minX);
    status |= setIntegerParam(ADMinY,  minY);
    status |= setIntegerParam(ADSizeX, sizeX);
    status |= setIntegerParam(ADSizeY, sizeY);

    status |= setIntegerParam(NDArraySize, 0);
    status |= setIntegerParam(NDDataType,  NDFloat64);

    status |= setIntegerParam(ADImageMode, ADImageSingle);

    /* NOTE: these char type waveform record could not be initialized in iocInit
     * Instead use autosave to restore their values.
     * It is left here only for references.
     * */
    status |= setStringParam(NDFilePath,       pDetector->getFilePath().c_str());
    status |= setStringParam(NDFileName,       pDetector->getFileName().c_str());

    status |= setStringParam(SDFlatFieldPath,  pDetector->getFlatFieldCorrectionDir().c_str());
    status |= setStringParam(SDFlatFieldPath,  pDetector->getFlatFieldCorrectionFile().c_str());

    status |= setIntegerParam(SDUseFlatField,  pDetector->enableFlatFieldCorrection());
    status |= setIntegerParam(SDUseCountRate,  pDetector->enableCountRateCorrection());
    status |= setIntegerParam(SDUsePixelMask,  pDetector->enablePixelMaskCorrection());
    status |= setIntegerParam(SDUseAngularConv,  pDetector->enableAngularConversion());

    status |= setDoubleParam(ADTemperatureActual, pDetector->getADC("temp_fpga")/1000.);
    status |= setIntegerParam(SDHighVoltage,    pDetector->setHighVoltage(-1));
    status |= setIntegerParam(ADStatus,        pDetector->getDetectorStatus());

    callParamCallbacks();

    if (status) {
        printf("%s: unable to read camera parameters\n", functionName);
        return;
    }

    /* Register data callback function */
    pDetector->registerDataCallback(dataCallbackC,  (void *)this);
    /* Register acquisition finsihed callback function
     * A dummy callback is used only to trick slsDetector library to send us every nth frame
     * */
    pDetector->registerAcquisitionFinishedCallback(dummyFinishedCallbackC,  (void *)this);

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
static const iocshArg slsDetectorConfigArg2 = {"detector index", iocshArgInt};
static const iocshArg slsDetectorConfigArg3 = {"maxBuffers", iocshArgInt};
static const iocshArg slsDetectorConfigArg4 = {"maxMemory", iocshArgInt};
static const iocshArg slsDetectorConfigArg5 = {"priority", iocshArgInt};
static const iocshArg slsDetectorConfigArg6 = {"stackSize", iocshArgInt};
static const iocshArg * const slsDetectorConfigArgs[] =  {&slsDetectorConfigArg0,
                                                              &slsDetectorConfigArg1,
                                                              &slsDetectorConfigArg2,
                                                              &slsDetectorConfigArg3,
                                                              &slsDetectorConfigArg4,
                                                              &slsDetectorConfigArg5,
                                                              &slsDetectorConfigArg6};
static const iocshFuncDef configSlsDetector = {"slsDetectorConfig", 7, slsDetectorConfigArgs};
static void configSlsDetectorCallFunc(const iocshArgBuf *args)
{
    slsDetectorConfig(args[0].sval, args[1].sval, args[2].ival,
            args[3].ival, args[4].ival,  args[5].ival, args[6].ival);
}


static void slsDetectorRegister(void)
{

    iocshRegister(&configSlsDetector, configSlsDetectorCallFunc);
}

extern "C" {
epicsExportRegistrar(slsDetectorRegister);
}

