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

#include <string>
#include <sstream>

#include <ADDriver.h>
#include <asynNDArrayDriver.h>
#include <asynOctetSyncIO.h>

#include <iocsh.h>
#include <epicsExit.h>
#include <epicsString.h>
#include <epicsThread.h>

#include "sls/Detector.h"
#include "sls/detectorData.h"

#include <epicsExport.h>

#define MAX_FILENAME_LEN 256

static const char *driverName = "slsDetectorDriver";

#define SDDetectorTypeString    "SD_DETECTOR_TYPE"
#define SDSettingString         "SD_SETTING"
#define SDDelayTimeString       "SD_DELAY_TIME"
#define SDThresholdString       "SD_THRESHOLD"
#define SDEnergyString          "SD_ENERGY"
#define SDTrimbitsString        "SD_TRIMBITS"
#define SDUseDataCallbackString "SD_USE_DATA_CALLBACK"
#define SDBitDepthString        "SD_BIT_DEPTH"
#define SDNumGatesString        "SD_NUM_GATES"
#define SDNumCyclesString       "SD_NUM_CYCLES"
#define SDNumFramesString       "SD_NUM_FRAMES"
#define SDTimingModeString      "SD_TMODE"
#define SDTriggerSoftwareString "SD_TRIGGER_SOFTWARE"
#define SDRecvModeString        "SD_RECV_MODE"
#define SDRecvStreamString      "SD_RECV_STREAM"
#define SDRecvStatusString      "SD_RECV_STATUS"
#define SDRecvMissedString      "SD_RECV_MISSED"
#define SDHighVoltageString     "SD_HIGH_VOLTAGE"
#define SDCommandString         "SD_COMMAND"
#define SDCounterMaskString     "SD_COUNTER_MASK"
#define SDGate1DelayString      "SD_GATE1_DELAY"
#define SDGate2DelayString      "SD_GATE2_DELAY"
#define SDGate3DelayString      "SD_GATE3_DELAY"
#define SDGate1WidthString      "SD_GATE1_WIDTH"
#define SDGate2WidthString      "SD_GATE2_WIDTH"
#define SDGate3WidthString      "SD_GATE3_WIDTH"
#define SDSetupFileString       "SD_SETUP_FILE"
#define SDLoadSetupString       "SD_LOAD_SETUP"

#define SetDetectorParam(INDEX,TYPE,RESULT) \
    for (size_t ADDR=0; ADDR<RESULT.size(); ADDR++) \
        status |= set##TYPE##Param(ADDR, INDEX, RESULT[ADDR]);

#define SetDetectorStringParam(INDEX,RESULT) \
    for (size_t ADDR=0; ADDR<RESULT.size(); ADDR++) \
        status |= setStringParam(ADDR, INDEX, RESULT[ADDR].c_str());

/** Driver for sls array detectors using over TCP/IP socket */
class slsDetectorDriver : public ADDriver {
public:
    slsDetectorDriver(const char *portName, const char *configFileName,
                    int detectorId, int numModules,
                    int maxBuffers, size_t maxMemory,
                    int priority, int stackSize);

    /* These are the methods that we override from ADDriver */
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    virtual asynStatus writeUInt32Digital(asynUser *pasynUser, epicsUInt32 value, epicsUInt32 mask);
    virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
    virtual asynStatus writeOctet(asynUser *pasynUser, const char *value,
                                    size_t nChars, size_t *nActual);
    virtual asynStatus readEnum(asynUser *pasynUser, char *strings[], int values[], int severities[],
                               size_t nElements, size_t *nIn);

    virtual void report(FILE *fp, int details);

    void dataCallback(detectorData *pData); /* These should be private but are called from C so must be public */
    void acquisitionTask();
    void shutdown();

 protected:
    int SDDetectorType;
    #define FIRST_SD_PARAM SDDetectorType
    int SDSetting;
    int SDDelayTime;
    int SDThreshold;
    int SDTrimbits;
    int SDEnergy;
    int SDUseDataCallback;
    int SDBitDepth;
    int SDNumGates;
    int SDNumCycles;
    int SDNumFrames;
    int SDTimingMode;
    int SDTriggerSoftware;
    int SDRecvMode;
    int SDRecvStream;
    int SDRecvStatus;
    int SDRecvMissed;
    int SDHighVoltage;
    int SDCommand;
    int SDCounterMask;
    int SDGate1Delay;
    int SDGate2Delay;
    int SDGate3Delay;
    int SDGate1Width;
    int SDGate2Width;
    int SDGate3Width;
    int SDSetupFile;
    int SDLoadSetup;
    #define LAST_SD_PARAM SDLoadSetup

 private:
    /* These are the methods that are new to this class */
    std::chrono::nanoseconds secToNsec(double seconds);
    double nsecToSec(std::chrono::nanoseconds nanoseconds);

    /* Our data */
    sls::Detector *pDetector;
    epicsEventId startEventId;
    int tempDacIndex;
};

#define NUM_SD_PARAMS (&LAST_SD_PARAM - &FIRST_SD_PARAM + 1)

std::chrono::nanoseconds slsDetectorDriver::secToNsec(double seconds)
{
    std::chrono::duration<double> value(seconds);
    return std::chrono::duration_cast<std::chrono::nanoseconds>(value);
}

double slsDetectorDriver::nsecToSec(std::chrono::nanoseconds nanoseconds)
{
    return std::chrono::duration<double>(nanoseconds).count();
}

static void c_shutdown(void* arg) {
    slsDetectorDriver *p = (slsDetectorDriver*)arg;
    p->shutdown();
}

void dataCallbackC(detectorData *pData, uint64_t n, uint32_t s, void *pArg)
{
    if (pData == NULL)
       return;

    if (pArg  != NULL) {
        slsDetectorDriver *pDetectorDriver = (slsDetectorDriver*)pArg;
        pDetectorDriver->dataCallback(pData);
    }
}

void acquisitionTaskC(void *drvPvt)
{
    slsDetectorDriver *pDetectorDriver = (slsDetectorDriver*)drvPvt;
    pDetectorDriver->acquisitionTask();
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
            /* Release the lock while we wait for an event that says acquire has started, then lock again */
            asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
                "%s:%s: waiting for acquire to start\n", driverName, functionName);
            this->unlock();
            status = epicsEventWait(this->startEventId);
            this->lock();
            getIntegerParam(ADAcquire, &acquire);
        }

        /* Poll detector temperature if exist */
        if (tempDacIndex != -1) {
            try {
            SetDetectorParam(ADTemperatureActual, Double, pDetector->getTemperature(slsDetectorDefs::dacIndex(tempDacIndex)));
            callParamCallbacks();
            } catch (const std::exception &e) {
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: Detector::getTemperature: %s\n",
                driverName, functionName, e.what());
            }
        }

        setShutter(1);
        setIntegerParam(ADStatus, slsDetectorDefs::RUNNING);
        setIntegerParam(SDRecvMissed, 0);
        callParamCallbacks();

        /* Start acquisition, this is a blocking function */
        this->unlock();
        try {
        status = asynSuccess;
        pDetector->acquire();
        } catch (const std::exception &e) {
        status = asynError;
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s: Detector::acquire error: %s\n",
            driverName, functionName, e.what());
        }
        /* In case of exception, stop */
        if (status) {
            this->lock();
            setShutter(0);
            setIntegerParam(ADStatus, slsDetectorDefs::ERROR);
            setIntegerParam(ADAcquire, 0);
            callParamCallbacks();
            continue;
        }
        /* Update detector status */
        try {
        auto detStatus = pDetector->getDetectorStatus();
        auto recvStatus = pDetector->getReceiverStatus();
        auto fileNumber = pDetector->getAcquisitionIndex();
        SetDetectorParam(ADStatus, Integer, detStatus);
        SetDetectorParam(SDRecvStatus, Integer, recvStatus);
        SetDetectorParam(NDFileNumber, Integer, fileNumber);
        // missed packets for each module (summed up for all UDP ports)
        int missedPackets = 0;
        for (auto m: pDetector->getNumMissingPackets())
            missedPackets += sls::sum(m);
        setIntegerParam(SDRecvMissed, missedPackets);
        } catch (const std::exception &e) {
        status = asynError;
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s: %s\n",
            driverName, functionName, e.what());
        }

        this->lock();
        /* Compose last saved file name
         * Not using FileTemplate because it is builtin slsDetector library. */
        getStringParam(NDFilePath, sizeof(filePath), filePath);
        getStringParam(NDFileName, sizeof(fileName), fileName);
        getIntegerParam(NDFileNumber, &fileNumber);
        epicsSnprintf(fullFileName, MAX_FILENAME_LEN, "%s%s_%d", filePath, fileName, fileNumber-1);
        setStringParam(NDFullFileName, fullFileName);

        getIntegerParam(ADImageMode, &imageMode);
        if (imageMode == ADImageSingle || imageMode == ADImageMultiple) {
            setShutter(0);
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
    NDDataType_t dtype = NDUInt16;
    void *pBuffer = NULL;
    int imageCounter;
    int arrayCallbacks;
    epicsTimeStamp timeStamp;
    epicsInt32 colorMode = NDColorModeMono;
    const char *functionName = "dataCallback";

    if (pData == NULL || pData->data == NULL) return;

    this ->lock();

    dims[0] = pData->nx;
    dims[1] = pData->ny;

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
        case 8:
            dtype = NDUInt64;
            break;
        default:
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                  "%s:%s: unsupported dynamic range %d\n",
                  driverName, functionName, pData->dynamicRange);
            return;
    }
    pBuffer = pData->data;

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
    updateTimeStamp(&pImage->epicsTS);

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

/** Called when asyn clients call pasynUInt32Digital->write().
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write.
  * \param[in] mask Mask value to use when writinging the value. */
asynStatus slsDetectorDriver::writeUInt32Digital(asynUser *pasynUser, epicsUInt32 value, epicsUInt32 mask)
{
    int function = pasynUser->reason;
    int addr = 0;
    epicsUInt32 oldValue;
    int status = asynSuccess;
    std::vector<int> positions = {};
    const char *functionName = "writeUInt32Digital";

    status = getAddress(pasynUser, &addr);
    if (status != asynSuccess) return((asynStatus)status);
    if (addr != 0) positions.push_back(addr); // when addr=0, write all modules

    /* Reject any call to the detector if it is running */
    int acquire;
    getIntegerParam(ADAcquire, &acquire);
    if (acquire == 1) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
        "%s:%s: detector is busy\n", driverName, functionName);
        return asynError;
    }

    /* Set the parameter in the parameter library. */
    getUIntDigitalParam(addr, function, &oldValue, mask);
    status |= (asynStatus)setUIntDigitalParam(addr, function, value, mask);
    try {
    if (function == SDCounterMask) {
        pDetector->setCounterMask(value & mask, positions);
    } else {
        /* If this is not a parameter we have handled call the base class */
        if (function < FIRST_SD_PARAM)
            status = ADDriver::writeUInt32Digital(pasynUser, value, mask);
    }
    } catch (const std::exception &e) {
        status = asynError;
        setUIntDigitalParam(addr, function, oldValue, mask);
        printf("%s:%s: error: %s\n",
            driverName, functionName, e.what());
    }
    /* Update any changed parameters */
    if (addr == 0)
        for (int i=0; i<this->maxAddr; i++)
            callParamCallbacks(i);
    else
        callParamCallbacks(addr);

    if (status)
        asynPrint(pasynUser, ASYN_TRACE_ERROR,
              "%s:%s: error, status=%d function=%d, value=%u, mask=%u\n",
              driverName, functionName, status, function, value, mask);
    else
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s: function=%d, value=%d, mask=%u\n",
              driverName, functionName, function, value, mask);

    return((asynStatus)status);
}

/** Called when asyn clients call pasynInt32->write().
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. */
asynStatus slsDetectorDriver::writeOctet(asynUser *pasynUser, const char *value,
                                            size_t nChars, size_t *nActual)
{
    int function = pasynUser->reason;
    int addr = 0;
    int status = asynSuccess;
    std::vector<int> positions = {};
    const char *functionName = "writeOctet";

    status = getAddress(pasynUser, &addr);
    if (status != asynSuccess) return((asynStatus)status);
    if (addr != 0) positions.push_back(addr); // when addr=0, write all modules

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
    try {
    if (function == NDFilePath) {
        pDetector->setFilePath(value, positions);
        SetDetectorStringParam(NDFilePath, pDetector->getFilePath());
        this->checkPath();
    } else if (function == NDFileName) {
        pDetector->setFileNamePrefix(value, positions);
        SetDetectorStringParam(NDFileName, pDetector->getFileNamePrefix());
    } else if (function == SDCommand) {
        std::istringstream input(value);
        std::vector<std::string> commands;
        for (std::string command; std::getline(input, command, ';');)
            if (!command.empty())
                commands.push_back(command);
        pDetector->loadParameters(commands);
    } else {
        /* If this is not a parameter we have handled call the base class */
        if (function < FIRST_SD_PARAM)
            status = ADDriver::writeOctet(pasynUser, value,nChars, nActual);
    }
    } catch (const std::exception &e) {
        status = asynError;
        printf("%s:%s: error: %s\n",
            driverName, functionName, e.what());
    }
    /* Update any changed parameters */
    if (addr == 0)
        for (int i=0; i<this->maxAddr; i++)
            callParamCallbacks(i);
    else
        callParamCallbacks(addr);

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
    int addr = 0;
    epicsInt32 oldValue;
    std::vector<int> positions = {};
    int status = asynSuccess;
    char filePath[MAX_FILENAME_LEN];
    static const char *functionName = "writeInt32";

    status = getAddress(pasynUser, &addr);
    if (status != asynSuccess) return((asynStatus)status);
    if (addr != 0) positions.push_back(addr); // when addr=0, write all modules

    /* Reject any call to the detector if it is running */
    int acquire;
    getIntegerParam(ADAcquire, &acquire);
    if ((function != ADAcquire && function != SDTriggerSoftware) && acquire == 1) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
        "%s:%s: detector is busy\n", driverName, functionName);
        return asynError;
    }

    /* Set the parameter and readback in the parameter library.
     * This may be overwritten when we read back the
     * status at the end, but that's OK */
    getIntegerParam(addr, function, &oldValue);
    status |= setIntegerParam(addr, function, value);

    try {
    if (function == ADMinX ||
        function == ADMinY ||
        function == ADSizeX ||
        function == ADSizeY ||
        function == ADBinX ||
        function == ADBinY) {
        setIntegerParam(addr, function, oldValue);
    } else if (function == NDFileNumber) {
        pDetector->setAcquisitionIndex(value, positions);
        SetDetectorParam(NDFileNumber, Integer, pDetector->getAcquisitionIndex());
    } else if (function == SDSetting ||
               function == SDThreshold ||
               function == SDEnergy ||
               function == SDTrimbits) {
        if (pDetector->getDetectorType().squash(slsDetectorDefs::GENERIC) == slsDetectorDefs::EIGER) {
            int threshold;
            int setting;
            int trimbits;
            getIntegerParam(SDThreshold, &threshold);
            getIntegerParam(SDSetting, &setting);
            getIntegerParam(SDTrimbits, &trimbits);

            /* Threshold energy is automatically set to half of the beam energy */
            if (function == SDEnergy) threshold = value / 2;

            pDetector->setThresholdEnergy(threshold, slsDetectorDefs::detectorSettings(setting), trimbits, positions);
            SetDetectorParam(SDThreshold, Integer, pDetector->getThresholdEnergy());
            SetDetectorParam(SDSetting, Integer, pDetector->getSettings());
        } else {
            if (function == SDSetting && value != -1) {
                pDetector->setSettings(slsDetectorDefs::detectorSettings(value), positions);
                SetDetectorParam(SDSetting, Integer, pDetector->getSettings());
            }
        }
    } else if (function == SDUseDataCallback) {
        pDetector->registerDataCallback(value ? dataCallbackC : NULL,  (void *)this);
    } else if (function == SDBitDepth) {
        pDetector->setDynamicRange(value);
        SetDetectorParam(SDBitDepth, Integer, pDetector->getDynamicRange());
        if (value <= 8)
            setIntegerParam(addr, NDDataType, NDUInt8);
        else if (value == 16)
            setIntegerParam(addr, NDDataType, NDUInt16);
        else if (value == 32)
            setIntegerParam(addr, NDDataType, NDUInt32);
        else if (value == 64)
            setIntegerParam(addr, NDDataType, NDUInt64);
    } else if (function == SDNumGates) {
        pDetector->setNumberOfGates(value, positions);
        SetDetectorParam(SDNumGates, Integer, pDetector->getNumberOfGates());
    } else if (function == SDNumCycles) {
        pDetector->setNumberOfTriggers(value);
        SetDetectorParam(SDNumCycles, Integer, pDetector->getNumberOfTriggers());
    } else if (function == SDNumFrames) {
        pDetector->setNumberOfFrames(value);
        SetDetectorParam(SDNumFrames, Integer, pDetector->getNumberOfFrames());
    } else if (function == SDTimingMode) {
        pDetector->setTimingMode(slsDetectorDefs::timingMode(value), positions);
        SetDetectorParam(SDTimingMode, Integer, pDetector->getTimingMode());
    } else if (function == SDTriggerSoftware) {
        pDetector->sendSoftwareTrigger(positions);
    } else if (function == SDLoadSetup) {
        getStringParam(SDSetupFile, sizeof(filePath), filePath);
        try {
            pDetector->loadParameters(filePath);
        }
        catch (const std::exception &e) {
            status |= asynError;
            printf("%s:%s: loadParameters %s error: %s\n",
                driverName, functionName, filePath, e.what());
        }
        setIntegerParam(SDLoadSetup, 0);
    } else if (function == ADAcquire) {
        if (value) {
            /* Send an event to wake up the acquisition task.  */
            epicsEventSignal(this->startEventId);
        } else {
            pDetector->stopDetector();
            pDetector->stopReceiver();
            pDetector->clearAcquiringFlag();
            SetDetectorParam(ADStatus, Integer, pDetector->getDetectorStatus());
            SetDetectorParam(SDRecvStatus, Integer, pDetector->getReceiverStatus());
        }
    } else if (function ==  NDAutoSave) {
        pDetector->setFileWrite(value, positions);
        SetDetectorParam(NDAutoSave, Integer, pDetector->getFileWrite());
    } else if (function == SDRecvMode) {
        pDetector->setRxZmqFrequency(value, positions);
        SetDetectorParam(SDRecvMode, Integer, pDetector->getRxZmqFrequency());
    } else if (function == SDRecvStream) {
        pDetector->setRxZmqDataStream(value, positions);
        SetDetectorParam(SDRecvStream, Integer, pDetector->getRxZmqDataStream());
    } else if (function == SDHighVoltage) {
        pDetector->setHighVoltage(value, positions);
        SetDetectorParam(SDHighVoltage, Integer, pDetector->getHighVoltage());
    } else {
        /* If this is not a parameter we have handled call the base class */
        if (function < FIRST_SD_PARAM) status = ADDriver::writeInt32(pasynUser, value);
    }
    } catch (const std::exception &e) {
        status = asynError;
        setIntegerParam(addr, function, oldValue);
        printf("%s:%s: error: %s\n",
            driverName, functionName, e.what());
    }

    /* Update any changed parameters */
    if (addr == 0)
        for (int i=0; i<this->maxAddr; i++)
            callParamCallbacks(i);
    else
        callParamCallbacks(addr);

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
    int addr = 0;
    std::vector<int> positions = {};
    int status = asynSuccess;
    const char* functionName = "writeFloat64";

    status = getAddress(pasynUser, &addr);
    if (status != asynSuccess) return((asynStatus)status);
    if (addr != 0) positions.push_back(addr); // when addr=0, write all modules

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

    try {
    if (function == ADAcquireTime) {
        pDetector->setExptime(secToNsec(value), positions);
        /* Each MYTHEN3 module has 3 gates, and every gate has SDGateWidth, which is equivalent to exposure time.
         * ADAcquireTime setting is propagated to all gates. If the 3 gates have different SDGateWidth,
         * ADAcquireTime readback is set to -1 */
        if (pDetector->getDetectorType().squash(slsDetectorDefs::GENERIC) == slsDetectorDefs::MYTHEN3) {
            auto result = pDetector->getExptimeForAllGates();
            for (size_t i=0; i<result.size(); i++) {
                if (sls::allEqual(result[i]))
                    status |= setDoubleParam(i, ADAcquireTime, nsecToSec(result[i][0]));
                else
                    status |= setDoubleParam(i, ADAcquireTime, -1);
                for (size_t j=0; j<3; j++)
                    status |= setDoubleParam(i, SDGate1Width+j, nsecToSec(result[i][j]));
            }
        } else {
            auto result = pDetector->getExptime();
            for (size_t i=0; i<result.size(); i++)
                status |= setDoubleParam(i, ADAcquireTime, nsecToSec(result[i]));
        }
    } else if (function == ADAcquirePeriod) {
        pDetector->setPeriod(secToNsec(value), positions);
        auto result = pDetector->getPeriod();
        for (size_t i=0; i<result.size(); i++)
            status |= setDoubleParam(i, ADAcquirePeriod, nsecToSec(result[i]));
    } else if (function == SDDelayTime) {
        pDetector->setDelayAfterTrigger(secToNsec(value), positions);
        auto result = pDetector->getDelayAfterTrigger();
        for (size_t i=0; i<result.size(); i++)
            status |= setDoubleParam(i, SDDelayTime, nsecToSec(result[i]));
    } else if (function >= SDGate1Delay && function <= SDGate3Delay) {
        int index = function - SDGate1Delay;
        pDetector->setGateDelay(index, secToNsec(value), positions);
    } else if (function >= SDGate1Width && function <= SDGate3Width) {
        int index = function - SDGate1Width;
        pDetector->setExptime(index, secToNsec(value), positions);
        auto result = pDetector->getExptimeForAllGates();
        for (size_t i=0; i<result.size(); i++) {
            if (sls::allEqual(result[i]))
                status |= setDoubleParam(i, ADAcquireTime, nsecToSec(result[i][0]));
            else
                status |= setDoubleParam(i, ADAcquireTime, -1);
        }
    } else {
        /* If this is not a parameter we have handled call the base class */
        if (function < NUM_SD_PARAMS) status = ADDriver::writeFloat64(pasynUser, value);
    }
    } catch (const std::exception &e) {
        status = asynError;
        printf("%s:%s: error: %s\n",
            driverName, functionName, e.what());
    }

    /* Update any changed parameters */
    if (addr == 0)
        for (int i=0; i<this->maxAddr; i++)
            callParamCallbacks(i);
    else
        callParamCallbacks(addr);

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

asynStatus slsDetectorDriver::readEnum(asynUser *pasynUser, char *strings[], int values[], int severities[],
                               size_t nElements, size_t *nIn)
{
    int function = pasynUser->reason;

    *nIn = 0;
    if (function == SDSetting) {
        try {
        auto settings = pDetector->getSettingsList();
        for (size_t i=0; i<settings.size(); i++) {
            if (strings[*nIn]) free(strings[*nIn]);
            strings[*nIn] = epicsStrDup(sls::ToString(settings[i]).c_str());
            values[*nIn] = settings[i];
            severities[*nIn] = 0;
            (*nIn)++;
        }
        } catch (...) {
            if (strings[0]) free(strings[0]);
            strings[0] = epicsStrDup("N.A.");
            values[0] = -1;
            *nIn = 1;
        }
        return asynSuccess;
    }
    else if (function == SDTimingMode) {
        auto modes = pDetector->getTimingModeList();
        for (size_t i=0; i<modes.size(); i++) {
            if (strings[*nIn]) free(strings[*nIn]);
            strings[*nIn] = epicsStrDup(sls::ToString(modes[i]).c_str());
            values[*nIn] = modes[i];
            severities[*nIn] = 0;
            (*nIn)++;
        }
        return asynSuccess;
    }
    else if (function == SDBitDepth) {
        auto list = pDetector->getDynamicRangeList();
        for (size_t i=0; i<list.size(); i++) {
            if (strings[*nIn]) free(strings[*nIn]);
            strings[*nIn] = epicsStrDup(std::to_string(list[i]).c_str());
            values[*nIn] = list[i];
            severities[*nIn] = 0;
            (*nIn)++;
        }
        return asynSuccess;
    }
    else {
        return asynError;
    }
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
                                    int detectorId, int numModules,
                                    int maxBuffers, size_t maxMemory,
                                    int priority, int stackSize)
{
    new slsDetectorDriver(portName, configFileName,
            detectorId, numModules,
            maxBuffers, maxMemory,
            priority, stackSize);
    return(asynSuccess);
}

/** Constructor for slsDetectorDriver driver; most parameters are simply passed to ADDriver::ADDriver.
  * After calling the base class constructor this method creates a thread to collect the detector data,
  * and sets reasonable default values for the parameters defined in this class, asynNDArrayDriver, and ADDriver.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] configFileName The configuration file to the detector.
  * \param[in] detectorId The detector index number running on the same system.
  * \param[in] numModules The number of modules for a multi-module detector.
  * \param[in] maxBuffers The maximum number of NDArray buffers that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  */
slsDetectorDriver::slsDetectorDriver(const char *portName, const char *configFileName,
                                int detectorId, int numModules,
                                int maxBuffers, size_t maxMemory,
                                int priority, int stackSize)

    : ADDriver(portName, numModules, NUM_SD_PARAMS, maxBuffers, maxMemory,
               asynEnumMask | asynUInt32DigitalMask, asynEnumMask | asynUInt32DigitalMask,
               ASYN_CANBLOCK | ASYN_MULTIDEVICE, 1, /* ASYN_CANBLOCK=1, ASYN_MULTIDEVICE=1, autoConnect=1 */
               priority, stackSize), pDetector(NULL)

{
    int status = asynSuccess;
    const char *functionName = "slsDetectorDriver";

    /* Create the epicsEvents for signaling to the slsDetector task when acquisition starts */
    this->startEventId = epicsEventCreate(epicsEventEmpty);
    if (!this->startEventId) {
        printf("%s:%s epicsEventCreate failure for start event\n",
            driverName, functionName);
        return;
    }

    createParam(SDDetectorTypeString,   asynParamInt32,  &SDDetectorType);
    createParam(SDSettingString,        asynParamInt32,  &SDSetting);
    createParam(SDDelayTimeString,      asynParamFloat64,&SDDelayTime);
    createParam(SDThresholdString,      asynParamInt32,  &SDThreshold);
    createParam(SDEnergyString,         asynParamInt32,  &SDEnergy);
    createParam(SDTrimbitsString,       asynParamInt32,  &SDTrimbits);
    createParam(SDUseDataCallbackString,asynParamInt32,  &SDUseDataCallback);
    createParam(SDBitDepthString,       asynParamInt32,  &SDBitDepth);
    createParam(SDNumGatesString,       asynParamInt32,  &SDNumGates);
    createParam(SDNumCyclesString,      asynParamInt32,  &SDNumCycles);
    createParam(SDNumFramesString,      asynParamInt32,  &SDNumFrames);
    createParam(SDTimingModeString,     asynParamInt32,  &SDTimingMode);
    createParam(SDTriggerSoftwareString,asynParamInt32,  &SDTriggerSoftware);
    createParam(SDRecvModeString,       asynParamInt32,  &SDRecvMode);
    createParam(SDRecvStreamString,     asynParamInt32,  &SDRecvStream);
    createParam(SDRecvStatusString,     asynParamInt32,  &SDRecvStatus);
    createParam(SDRecvMissedString,     asynParamInt32,  &SDRecvMissed);
    createParam(SDHighVoltageString,    asynParamInt32,  &SDHighVoltage);
    createParam(SDCommandString,        asynParamOctet,  &SDCommand);
    createParam(SDCounterMaskString,    asynParamUInt32Digital,  &SDCounterMask);
    createParam(SDGate1DelayString,     asynParamFloat64,&SDGate1Delay);
    createParam(SDGate2DelayString,     asynParamFloat64,&SDGate2Delay);
    createParam(SDGate3DelayString,     asynParamFloat64,&SDGate3Delay);
    createParam(SDGate1WidthString,     asynParamFloat64,&SDGate1Width);
    createParam(SDGate2WidthString,     asynParamFloat64,&SDGate2Width);
    createParam(SDGate3WidthString,     asynParamFloat64,&SDGate3Width);
    createParam(SDSetupFileString,      asynParamOctet,  &SDSetupFile);
    createParam(SDLoadSetupString,      asynParamInt32,  &SDLoadSetup);

    /* Connect to camserver */
    pDetector = new sls::Detector(detectorId);
    try {
        pDetector->loadConfig(configFileName);
    }
    catch (const std::exception &e) {
        status = asynError;
        printf("%s:%s: Detector::loadConfige %s error: %s\n",
            driverName, functionName, configFileName, e.what());
        return;
    }

    if (pDetector->size() > numModules) {
        fprintf(stderr, "%s:%s: more modules (%d) exist than configured (%d).\n",
                driverName, functionName, pDetector->size(), numModules);
        return;
    }

    /* Set some default values for parameters */
    status =  setStringParam (ADManufacturer, "SLS Detector Group");
    status |= setStringParam (ADModel, sls::ToString(pDetector->getDetectorType().squash(slsDetectorDefs::GENERIC)).c_str());
    SetDetectorParam(SDDetectorType,Integer,  pDetector->getDetectorType());
    status |= setStringParam (ADSDKVersion, pDetector->getPackageVersion().c_str());
    auto serialNumbers = pDetector->getSerialNumber();
    for (size_t i=0; i<serialNumbers.size(); i++)
        status |= setStringParam(i, ADSerialNumber, std::to_string(serialNumbers[i]));
    auto firmwareVersions = pDetector->getFirmwareVersion();
    for (size_t i=0; i<firmwareVersions.size(); i++)
        status |= setStringParam(i, ADFirmwareVersion, std::to_string(firmwareVersions[i]));

    auto sensorSize = pDetector->getDetectorSize();
    status |= setIntegerParam(ADMaxSizeX, sensorSize.x);
    status |= setIntegerParam(ADMaxSizeY, sensorSize.y);

    status |= setIntegerParam(ADMinX,  0);
    status |= setIntegerParam(ADMinY,  0);
    status |= setIntegerParam(ADSizeX, sensorSize.x);
    status |= setIntegerParam(ADSizeY, sensorSize.y);

    status |= setIntegerParam(NDArraySize, sensorSize.x * sensorSize.y);
    status |= setIntegerParam(NDDataType,  NDUInt16);

    status |= setIntegerParam(ADImageMode, ADImageSingle);

    auto temps = pDetector->getTemperatureList();
    if (temps.empty())
        tempDacIndex = -1;
    else {
        tempDacIndex = slsDetectorDefs::TEMPERATURE_FPGA;
        if (std::find(temps.begin(), temps.end(), tempDacIndex) == temps.end())
            tempDacIndex = temps[0];
    }
    try {
    SetDetectorStringParam(NDFilePath, pDetector->getFilePath());
    SetDetectorStringParam(NDFileName, pDetector->getFileNamePrefix());

    if (tempDacIndex != -1)
        SetDetectorParam(ADTemperatureActual, Double, pDetector->getTemperature(slsDetectorDefs::dacIndex(tempDacIndex)));
    SetDetectorParam(SDHighVoltage, Integer, pDetector->getHighVoltage());

    status |= setIntegerParam(ADStatus, pDetector->getDetectorStatus().squash(slsDetectorDefs::ERROR));
    SetDetectorParam(SDRecvStatus, Integer, pDetector->getReceiverStatus());
    }
    catch (const std::exception &e) {
        status = asynError;
        printf("%s:%s: error: %s\n",
            driverName, functionName, e.what());
    }
    for (int i=0; i<this->maxAddr; i++)
        callParamCallbacks(i);

    if (status) {
        printf("%s: unable to read camera parameters\n", functionName);
    }

    /* Register data callback function */
    pDetector->registerDataCallback(dataCallbackC,  (void *)this);
    status |= setIntegerParam(SDUseDataCallback, 1);

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
static const iocshArg slsDetectorConfigArg3 = {"numModules", iocshArgInt};
static const iocshArg slsDetectorConfigArg4 = {"maxBuffers", iocshArgInt};
static const iocshArg slsDetectorConfigArg5 = {"maxMemory", iocshArgInt};
static const iocshArg slsDetectorConfigArg6 = {"priority", iocshArgInt};
static const iocshArg slsDetectorConfigArg7 = {"stackSize", iocshArgInt};
static const iocshArg * const slsDetectorConfigArgs[] =  {&slsDetectorConfigArg0,
                                                              &slsDetectorConfigArg1,
                                                              &slsDetectorConfigArg2,
                                                              &slsDetectorConfigArg3,
                                                              &slsDetectorConfigArg4,
                                                              &slsDetectorConfigArg5,
                                                              &slsDetectorConfigArg6,
                                                              &slsDetectorConfigArg7};
static const iocshFuncDef configSlsDetector = {"slsDetectorConfig", 8, slsDetectorConfigArgs};
static void configSlsDetectorCallFunc(const iocshArgBuf *args)
{
    slsDetectorConfig(args[0].sval, args[1].sval, args[2].ival, args[3].ival,
            args[4].ival, args[5].ival,  args[6].ival, args[7].ival);
}


static void slsDetectorRegister(void)
{

    iocshRegister(&configSlsDetector, configSlsDetectorCallFunc);
}

extern "C" {
epicsExportRegistrar(slsDetectorRegister);
}

