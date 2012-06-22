/*******************************************************************

Date:       $Date: 2012/06/22 12:04:14 $
Revision:   $Rev$
Author:     $Author: wang_x1 $
URL:        $URL$
ID:         $Id: multiSlsDetector.h,v 1.1 2012/06/22 12:04:14 wang_x1 Exp $

********************************************************************/



#ifndef MULTI_SLS_DETECTOR_H
#define MULTI_SLS_DETECTOR_H

#include "slsDetectorUtils.h"

class slsDetector;

//#include "sls_detector_defs.h"



//using namespace std;



/**
 * 
 *
@libdoc The multiSlsDetector class is used to operate several slsDetectors in parallel.
 *
 * @short This is the base class for multi detector system functionalities
 * @author Anna Bergamaschi
 * @version 0.1alpha

 */

class multiSlsDetector  : public slsDetectorUtils {


  
  typedef  struct sharedMultiSlsDetector {




    /** already existing flag. If the detector does not yet exist (alreadyExisting=0) the sharedMemory will be created, otherwise it will simly be linked */
    int alreadyExisting;


    /** last process id accessing the shared memory */
    pid_t lastPID;


    /** online flag - is set if the detector is connected, unset if socket connection is not possible  */
    int onlineFlag;
    
  
    /** stopped flag - is set if an acquisition error occurs or the detector is stopped manually. Is reset to 0 at the start of the acquisition */
    int stoppedFlag;
    
    
    /** Number of detectors operated at once */
    int numberOfDetectors;

    /** Ids of the detectors to be operated at once */
    int detectorIds[MAXDET];


    /** Detectors offset in the X direction (in number of channels)*/
    int offsetX[MAXDET];
    /** Detectors offsets  in the Y direction (in number of channels) */
    int offsetY[MAXDET];

    /** position of the master detector */
    int masterPosition;
    
    /** type of synchronization between detectors */
    synchronizationMode syncMode;
      
    /**  size of the data that are transfered from all detectors */
    int dataBytes;
  
    /**  total number of channels for all detectors */
    int numberOfChannels;
  
    /**  total number of channels for all detectors */
    int maxNumberOfChannels;
  


    /** timer values */
    int64_t timerValue[MAX_TIMERS]; // needed?!?!?!?

    /** detector settings (standard, fast, etc.) */
    detectorSettings currentSettings; // needed?!?!?!?
    /** detector threshold (eV) */
    int currentThresholdEV; // needed?!?!?!?

 
    /** indicator for the acquisition progress - set to 0 at the beginning of the acquisition and incremented every time that the data are written to file */   
    int progressIndex;	
    /** total number of frames to be acquired */   
    int totalProgress;	   
 


   /** current index of the output file */   
    int fileIndex;
    /** path of the output files */  
    char filePath[MAX_STR_LENGTH];
    /** name root of the output files */  
    char fileName[MAX_STR_LENGTH];
    

    /** corrections  to be applied to the data \see ::correctionFlags */
    int correctionMask;
    /** threaded processing flag (i.e. if data are processed and written to file in a separate thread)  */
    int threadedProcessing;
    /** dead time (in ns) for rate corrections */
    float tDead;



    /** directory where the flat field files are stored */
    char flatFieldDir[MAX_STR_LENGTH];
    /** file used for flat field corrections */
    char flatFieldFile[MAX_STR_LENGTH];


    /** file with the bad channels */
    char badChanFile[MAX_STR_LENGTH];


    /** angular conversion file */
    char angConvFile[MAX_STR_LENGTH];







     /** array of angular conversion constants for each module \see ::angleConversionConstant */
    //angleConversionConstant angOff[MAXMODS];
    /** angular direction (1 if it corresponds to the encoder direction i.e. channel 0 is 0, maxchan is positive high angle, 0 otherwise  */
    int angDirection;
     /** beamline fine offset (of the order of mdeg, might be adjusted for each measurements)  */
    float fineOffset;
     /** beamline offset (might be a few degrees beacuse of encoder offset - normally it is kept fixed for a long period of time)  */
    float globalOffset;
    /** bin size for data merging */
    float binSize;


     /** number of positions at which the detector should acquire  */
    int numberOfPositions;
     /** list of encoder positions at which the detector should acquire */
    float detPositions[MAXPOS];






    /** Scans and scripts */

    int actionMask;
    
    //int actionMode[MAX_ACTIONS];
    mystring actionScript[MAX_ACTIONS];
    mystring actionParameter[MAX_ACTIONS];
    

    int scanMode[MAX_SCAN_LEVELS];
    mystring scanScript[MAX_SCAN_LEVELS];
    mystring scanParameter[MAX_SCAN_LEVELS];
    int nScanSteps[MAX_SCAN_LEVELS];
    mysteps scanSteps[MAX_SCAN_LEVELS];
    int scanPrecision[MAX_SCAN_LEVELS];
    
    

  };
















 public:

 

 using slsDetectorUtils::flatFieldCorrect;
 using slsDetectorUtils::rateCorrect;
 using slsDetectorUtils::setBadChannelCorrection;
 using slsDetectorUtils::readAngularConversion;
 using slsDetectorUtils::writeAngularConversion;

/* 
      @short Structure allocated in shared memory to store detector settings and be accessed in parallel by several applications (take care of possible conflicts!)
      
  */


/** (default) constructor 
    \param  id is the detector index which is needed to define the shared memory id. Different physical detectors should have different IDs in order to work independently
    
    
*/
  multiSlsDetector(int id=0);
  //slsDetector(string  const fname);
  /** destructor */ 
  virtual ~multiSlsDetector();
  
  
  /** frees the shared memory occpied by the sharedMultiSlsDetector structure */
  int freeSharedMemory() ;
  
  /** allocates the shared memory occpied for the sharedMultiSlsDetector structure */
  int initSharedMemory(int) ;
  
  /** adds the detector with ID id in postion pos
   \param id of the detector to be added (should already exist!)
   \param pos position where it should be added (normally at the end of the list (default to -1)
   \return the actual number of detectors or -1 if it failed*/
  int addSlsDetector(int id, int pos=-1);

  /** adds the detector with ID id in postion pos
   \param name of the detector to be added (should already exist in shared memory or at least be online) 
   \param pos position where it should be added (normally at the end of the list (default to -1)
   \return the actual number of detectors or -1 if it failed*/
  int addSlsDetector(const char *name, int pos=-1);

  int addSlsDetector(detectorType type, int pos=-1);

  /**removes the detector in position pos from the multidetector
     \param pos position of the detector to be removed from the multidetector system (defaults to -1 i.e. last detector)
     \returns the actual number of detectors
  */
  int removeSlsDetector(int pos=-1);

  /**removes the detector in position pos from the multidetector
     \param name is the name of the detector
     \returns the actual number of detectors
  */
  int removeSlsDetector(char *name);




  string setHostname(const char*, int pos=-1);


  string getHostname(int pos=-1);
  using slsDetectorBase::getDetectorType;

  string getDetectorType(){return sgetDetectorsType();};

  detectorType getDetectorsType(int pos=-1);
  detectorType setDetectorsType(detectorType type=GET_DETECTOR_TYPE, int pos=-1){addSlsDetector(type, pos); return getDetectorsType(pos);}; 

  string sgetDetectorsType(int pos=-1);
  string ssetDetectorsType(detectorType type=GET_DETECTOR_TYPE, int pos=-1){return getDetectorType(setDetectorsType(type, pos));}; //
  string ssetDetectorsType(string s, int pos=-1);//{return getDetectorType(setDetectorsType(getDetectorType(s),pos));}; // should decode detector type


  /** adds a detector by id in position pos
   \param ival detector id to be added
   \param pos position to add it (-1 fails)
   \returns detector ID or -1 if detector in position i is empty
  */
  int setDetectorId(int ival, int pos=-1);



  /** returns the id of the detector in position i
   \param i position of the detector
  \returns detector ID or -1 if detector in position i is empty*/
  int getDetectorId(int i);


  /** returns the number of detectors in the multidetector structure
      \returns number of detectors */
  int getNumberOfDetectors() {return thisMultiDetector->numberOfDetectors;};

  

  int getMaxMods();
 int getNMods();
 int getChansPerMod(int imod=0);

 angleConversionConstant *getAngularConversionPointer(int imod=0);
  

  int getTotalNumberOfChannels(){return thisMultiDetector->numberOfChannels;};

  int getMaxNumberOfChannels(){return thisMultiDetector->maxNumberOfChannels;};

  float getScanStep(int index, int istep){return thisMultiDetector->scanSteps[index][istep];};
  /** returns the detector offset (in number of channels)
      \param pos position of the detector
      \param ox reference to the offset in x
      \param oy reference to the offset in y
      \returns OK/FAIL if the detector does not exist
  */
  int getDetectorOffset(int pos, int &ox, int &oy);

    /** sets the detector offset (in number of channels)
      \param pos position of the detector
      \param ox offset in x (-1 does not change)
      \param oy offset in y (-1 does not change)
      \returns OK/FAIL if the detector does not exist
  */
  int setDetectorOffset(int pos, int ox=-1, int oy=-1);

  

  /** sets the detector in position i as master of the structure (e.g. it gates the other detectors and therefore must be started as last. <BR> Assumes that signal 0 is gate in, signal 1 is trigger in, signal 2 is gate out
      \param i position of master (-1 gets, -2 unset)
      \return master's position (-1 none)
  */
  int setMaster(int i=-1);
  
  /** 
      Sets/gets the synchronization mode of the various detectors
      \param sync syncronization mode
      \returns current syncronization mode   
  */   
/*   enum synchronizationMode { */
/*     GET_SYNCHRONIZATION_MODE=-1, /\**< the multidetector will return its synchronization mode *\/ */
/*     NONE, /\**< all detectors are independent (no cabling) *\/ */
/*     MASTER_GATES, /\**< the master gates the other detectors *\/ */
/*     MASTER_TRIGGERS, /\**< the master triggers the other detectors *\/ */
/*     SLAVE_STARTS_WHEN_MASTER_STOPS /\**< the slave acquires when the master finishes, to avoid deadtime *\/ */
/*   }; */

  synchronizationMode setSynchronization(synchronizationMode sync=GET_SYNCHRONIZATION_MODE);


  /** sets the onlineFlag
      \param off can be:  GET_ONLINE_FLAG, returns wether the detector is in online or offline state; OFFLINE_FLAG, detector in offline state (i.e. no communication to the detector - using only local structure - no data acquisition possible!); ONLINE_FLAG  detector in online state (i.e. communication to the detector updating the local structure) 
      \returns online/offline status
  */
  int setOnline(int const online=GET_ONLINE_FLAG);  
  /**
      \returns 1 if the detector structure has already be initlialized with the given id and belongs to this multiDetector instance, 0 otherwise */
  int exists();



  /**
    Purely virtual function
    Should be implemented in the specific detector class
    /sa mythenDetector::readConfigurationFile
  */

  int readConfigurationFile(string const fname);  
  /**  
    Purely virtual function
    Should be implemented in the specific detector class
    /sa mythenDetector::writeConfigurationFile
  */
  int writeConfigurationFile(string const fname);


 
  /* I/O */
  


  /* Communication to server */

  // Expert Initialization functions
  
  /**
    get threshold energy
    \param imod module number (-1 all)
    \returns current threshold value for imod in ev (-1 failed)
  */
  int getThresholdEnergy(int imod=-1);  

  /**
    set threshold energy
    \param e_eV threshold in eV
    \param imod module number (-1 all)
    \param isettings ev. change settings
    \returns current threshold value for imod in ev (-1 failed)
  */
  int setThresholdEnergy(int e_eV, int imod=-1, detectorSettings isettings=GET_SETTINGS); 
 
  /**
    get detector settings
    \param imod module number (-1 all)
    \returns current settings
  */
  detectorSettings getSettings(int imod=-1);  

  /**
    set detector settings
    \param isettings  settings
    \param imod module number (-1 all)
    \returns current settings

    in this function trimbits and calibration files are searched in the trimDir and calDir directories and the detector is initialized
  */
  detectorSettings setSettings(detectorSettings isettings, int imod=-1);






 
  int64_t getId(idMode mode, int imod=0);
  int digitalTest(digitalTestMode mode, int imod=0);
  int executeTrimming(trimMode mode, int par1, int par2, int imod=-1);
  const char *getSettingsFile();


  int decodeNMod(int i, int &idet, int &imod);


  /** loads the modules settings/trimbits reading from a file -  file name extension is automatically generated! */
  int loadSettingsFile(string fname, int nmod=0);

  /** gets the modules settings/trimbits and writes them to file -  file name extension is automatically generated! */
  int saveSettingsFile(string fname, int nmod=0);



















// Acquisition functions


  /**
    start detector acquisition (master is started as last)
    \returns OK if all detectors are properly started, FAIL otherwise
  */
  int startAcquisition();

  /**
    stop detector acquisition (master firtst)
    \returns OK/FAIL
  */
  int stopAcquisition();
  
  /**
    start readout (without exposure or interrupting exposure) (master first)
    \returns OK/FAIL
  */
  int startReadOut();



  /**
    start detector acquisition and read all data putting them a data queue
    \returns pointer to the front of the data queue
    \sa startAndReadAllNoWait getDataFromDetector dataQueue
  */ 
  int* startAndReadAll();
  
  /**
    start detector acquisition and read out, but does not read data from socket 
   
  */ 
  int startAndReadAllNoWait(); 

  /**
    receives a data frame from the detector socket
    \returns pointer to the data or NULL. If NULL disconnects the socket
    \sa getDataFromDetector
  */ 
  //int* getDataFromDetectorNoWait(); 
  /**
    receives a data frame from the detector socket
    \returns pointer to the data or NULL. If NULL disconnects the socket
    \sa getDataFromDetector
  */ 
  int* getDataFromDetector();

  /**
   asks and  receives a data frame from the detector and puts it in the data queue
    \returns pointer to the data or NULL. 
    \sa getDataFromDetector
  */ 
  int* readFrame(); 

  /**
   asks and  receives all data  from the detector  and puts them in a data queue
    \returns pointer to the front of the queue  or NULL. 
    \sa getDataFromDetector  dataQueue
  */ 
  int* readAll();

  
  /**
   pops the data from the data queue
    \returns pointer to the popped data  or NULL if the queue is empty. 
    \sa  dataQueue
  */ 
  int* popDataQueue();

  /**
   pops the data from thepostprocessed data queue
    \returns pointer to the popped data  or NULL if the queue is empty. 
    \sa  finalDataQueue
  */ 
  detectorData* popFinalDataQueue();




  /**
  resets the raw data queue
    \sa  dataQueue
  */ 
  void resetDataQueue();

  /**
  resets the postprocessed  data queue
    \sa  finalDataQueue
  */ 
  void resetFinalDataQueue();







  int setSpeed(speedVariable sp, int value=-1);


  /** 
      set/get timer value
      \param index timer index
      \param t time in ns or number of...(e.g. frames, gates, probes)
      \returns timer set value in ns or number of...(e.g. frames, gates, probes)
  */
  int64_t setTimer(timerIndex index, int64_t t=-1);
  /** 
      set/get timer value
      \param index timer index
      \param t time in ns or number of...(e.g. frames, gates, probes)
      \returns timer set value in ns or number of...(e.g. frames, gates, probes)
  */
  int64_t getTimeLeft(timerIndex index);

/*   /\**  */
/*       get current timer value */
/*       \param index timer index */
/*       \returns elapsed time value in ns or number of...(e.g. frames, gates, probes) */
/*   *\/ */
/*   int64_t getTimeLeft(timerIndex index); */



  // Flags
  /** 
      set/get dynamic range and updates the number of dataBytes
      \param n dynamic range (-1 get)
      \param pos detector position (-1 all detectors)
      \returns current dynamic range
      updates the size of the data expected from the detector
      \sa sharedSlsDetector
  */
  int setDynamicRange(int n, int pos);


 
  /** 
      set roi

      not yet implemented
  */
  int setROI(int nroi=-1, int *xmin=NULL, int *xmax=NULL, int *ymin=NULL, int *ymax=NULL);
  


 
  //Corrections  


  /** 
      set flat field corrections
      \param fname name of the flat field file (or "" if disable)
      \returns 0 if disable (or file could not be read), >0 otherwise
  */
  int setFlatFieldCorrection(string fname=""); 

  /** 
      set flat field corrections
      \param corr if !=NULL the flat field corrections will be filled with corr (NULL usets ff corrections)
      \param ecorr if !=NULL the flat field correction errors will be filled with ecorr (1 otherwise)
      \returns 0 if ff correction disabled, >0 otherwise
  */
  int setFlatFieldCorrection(float *corr, float *ecorr=NULL);

  /** 
      get flat field corrections
      \param corr if !=NULL will be filled with the correction coefficients
      \param ecorr if !=NULL will be filled with the correction coefficients errors
      \returns 0 if ff correction disabled, >0 otherwise
  */
  int getFlatFieldCorrection(float *corr=NULL, float *ecorr=NULL);








  /** 
      set rate correction
      \param t dead time in ns - if 0 disable correction, if >0 set dead time to t, if <0 set deadtime to default dead time for current settings
      \returns 0 if rate correction disabled, >0 otherwise
  */
  int setRateCorrection(float t=0);

  
  /** 
      get rate correction
      \param t reference for dead time
      \returns 0 if rate correction disabled, >0 otherwise
  */
  int getRateCorrection(float &t);

  
  /** 
      get rate correction tau
      \returns 0 if rate correction disabled, otherwise the tau used for the correction
  */
  float getRateCorrectionTau();
  /** 
      get rate correction
      \returns 0 if rate correction disabled, >0 otherwise
  */
  int getRateCorrection();
  
  /** 
      set bad channels correction
      \param fname file with bad channel list ("" disable)
      \returns 0 if bad channel disabled, >0 otherwise
  */
  int setBadChannelCorrection(string fname="");
  

  int setBadChannelCorrection(int nch, int *chs, int ff);



  /** 
      get bad channels correction
      \param bad pointer to array that if bad!=NULL will be filled with the bad channel list
      \returns 0 if bad channel disabled or no bad channels, >0 otherwise
  */
  int getBadChannelCorrection(int *bad=NULL);


 
  /** 
      pure virtual function
      get angular conversion
      \param reference to diffractometer direction
      \param angconv array that will be filled with the angular conversion constants
      \returns 0 if angular conversion disabled, >0 otherwise
      \sa mythenDetector::getAngularConversion
  */
  /////////////////////////////////////////////////// virtual int getAngularConversion(int &direction,  angleConversionConstant *angconv=NULL);
  
  

  int readAngularConversionFile(string fname);

  int writeAngularConversion(string fname);

  //  float* convertAngles(float pos);




  /** 
     decode data from the detector converting them to an array of floats, one for each channle
     \param datain data from the detector
     \returns pointer to a float array with a data per channel
  */
  float* decodeData(int *datain, float *fdata=NULL);

  
  
  
  /** 
     flat field correct data
     \param datain data
     \param errin error on data (if<=0 will default to sqrt(datain)
     \param dataout corrected data
     \param errout error on corrected data
     \param ffcoefficient flat field correction coefficient
     \param fferr erro on ffcoefficient
     \returns 0
  */
  // int flatFieldCorrect(float datain, float errin, float &dataout, float &errout, float ffcoefficient, float fferr);
  
  /** 
     flat field correct data
     \param datain data array
     \param errin error array on data (if NULL will default to sqrt(datain)
     \param dataout array of corrected data
     \param errout error on corrected data (if not NULL)
     \returns 0
  */
  int flatFieldCorrect(float* datain, float *errin, float* dataout, float *errout);
 

  
  /** 
     rate correct data
     \param datain data
     \param errin error on data (if<=0 will default to sqrt(datain)
     \param dataout corrected data
     \param errout error on corrected data
     \param tau dead time 9in ns)
     \param t acquisition time (in ns)
     \returns 0
  */
  //  int rateCorrect(float datain, float errin, float &dataout, float &errout, float tau, float t);
  
  /** 
     rate correct data
     \param datain data array
     \param errin error array on data (if NULL will default to sqrt(datain)
     \param dataout array of corrected data
     \param errout error on corrected data (if not NULL)
     \returns 0
  */
  int rateCorrect(float* datain, float *errin, float* dataout, float *errout);

  /** 
      turns off server
  */
  int exitServer();

  /** pure /////////////////////////////////////////////////// virtual function
     function for processing data
     /param delflag if 1 the data are processed, written to file and then deleted. If 0 they are added to the finalDataQueue
     \sa mythenDetector::processData
  */
  ///////////////////////////////////////////////////  virtual void* processData(int delflag=1); // thread function

  
  /////////////////////////////////////////////////// virtual void acquire(int delflag=1);

  /** calcualtes the total number of steps of the acquisition.
      called when number of frames, number of cycles, number of positions and scan steps change
  */
  /////////////////////////////////////// int setTotalProgress(); ////////////// from slsDetectorUtils!

  /** returns the current progress in % */
  ////////////////////////////////float getCurrentProgress();////////////// from slsDetectorUtils!
  

  /**
    set dacs value
    \param val value (in V)
    \param index DAC index
    \param imod module number (if -1 alla modules)
    \returns current DAC value
  */
  float setDAC(float val, dacIndex index, int imod=-1);
  /**
    set dacs value
    \param val value (in V)
    \param index DAC index
    \param imod module number (if -1 alla modules)
    \returns current DAC value
  */
  float getADC(dacIndex index, int imod=0);
    /**
    configure channel
    \param reg channel register
    \param ichan channel number (-1 all)
    \param ichip chip number (-1 all)
    \param imod module number (-1 all)
    \returns current register value
    \sa ::sls_detector_channel
  */
  int setChannel(int64_t reg, int ichan=-1, int ichip=-1, int imod=-1);  
  /** 
      pure virtual function
      get angular conversion
      \param reference to diffractometer direction
      \param angconv array that will be filled with the angular conversion constants
      \returns 0 if angular conversion disabled, >0 otherwise
      \sa mythenDetector::getAngularConversion
  */
  int getAngularConversion(int &direction,  angleConversionConstant *angconv=NULL) ;
  
  

  /**
     get run status
    \returns status mask
  */
  //virtual runStatus  getRunStatus()=0;
  runStatus  getRunStatus();





  /** returns the detector trimbit/settings directory  \sa sharedSlsDetector */
  char* getSettingsDir();
  /** sets the detector trimbit/settings directory  \sa sharedSlsDetector */
  char* setSettingsDir(string s);
 /**
     returns the location of the calibration files
  \sa  sharedSlsDetector
  */
  char* getCalDir();
   /**
      sets the location of the calibration files
  \sa  sharedSlsDetector
  */
  char* setCalDir(string s); 


  char *getNetworkParameter(networkParameter);
  char *setNetworkParameter(networkParameter, std::string);
  int setPort(portType, int);
  int lockServer(int);
    
  string getLastClientIP();


  int configureMAC();

  int setNumberOfModules(int i=-1, dimension d=X);
  int getMaxNumberOfModules(dimension d=X);
  int setDynamicRange(int i=-1);





  int writeRegister(int addr, int val);
  

  int readRegister(int addr);



 int setTrimEn(int nen, int *en=NULL);
 int getTrimEn(int *en=NULL);


 externalSignalFlag setExternalSignalFlags(externalSignalFlag pol=GET_EXTERNAL_SIGNAL_FLAG , int signalindex=0);
 int setReadOutFlags(readOutFlags flag=GET_READOUT_FLAGS);


 externalCommunicationMode setExternalCommunicationMode(externalCommunicationMode pol=GET_EXTERNAL_COMMUNICATION_MODE);

 /**
      Loads dark image or gain image to the detector
      \param index can be DARK_IMAGE or GAIN_IMAGE
      \fname file name to load data from
      \returns OK or FAIL
 */
  int loadImageToDetector(imageType index,string const fname);

   /**
     sets the value of s angular conversion parameter
     \param c can be ANGULAR_DIRECTION, GLOBAL_OFFSET, FINE_OFFSET, BIN_SIZE
     \param v the value to be set
     \returns the actual value
  */

  float setAngularConversionParameter(angleConversionParameter c, float v);

    /**
     
       writes a data file
       \param name of the file to be written
       \param data array of data values
       \param err array of arrors on the data. If NULL no errors will be written
       
       \param ang array of angular values. If NULL data will be in the form chan-val(-err) otherwise ang-val(-err)
       \param dataformat format of the data: can be 'i' integer or 'f' float (default)
       \param nch number of channels to be written to file. if -1 defaults to the number of installed channels of the detector
       \returns OK or FAIL if it could not write the file or data=NULL
       \sa mythenDetector::writeDataFile
 
  */
   int writeDataFile(string fname, float *data, float *err=NULL, float *ang=NULL, char dataformat='f', int nch=-1); 
  

  /**
   
       writes a data file
       \param name of the file to be written
       \param data array of data values
       \returns OK or FAIL if it could not write the file or data=NULL  
       \sa mythenDetector::writeDataFile
  */
   int writeDataFile(string fname, int *data);
  
  /**
   
       reads a data file
       \param name of the file to be read
       \param data array of data values to be filled
       \param err array of arrors on the data. If NULL no errors are expected on the file
       
       \param ang array of angular values. If NULL data are expected in the form chan-val(-err) otherwise ang-val(-err)
       \param dataformat format of the data: can be 'i' integer or 'f' float (default)
       \param nch number of channels to be written to file. if <=0 defaults to the number of installed channels of the detector
       \returns OK or FAIL if it could not read the file or data=NULL
       
       \sa mythenDetector::readDataFile
  */
   int readDataFile(string fname, float *data, float *err=NULL, float *ang=NULL, char dataformat='f'); 


  /**
   
       reads a data file
       \param name of the file to be read
       \param data array of data values
       \returns OK or FAIL if it could not read the file or data=NULL
       \sa mythenDetector::readDataFile
  */
   int readDataFile(string fname, int *data);


   /**
        writes the counter memory block from the detector
        \param startACQ is 1 to start acquisition after reading counter
        \param fname file name to load data from
        \returns OK or FAIL
   */
   int writeCounterBlockFile(string const fname,int startACQ=0);
 

  /**
        Resets counter in detector
        \param startACQ is 1 to start acquisition after resetting counter
        \returns OK or FAIL
   */
   int resetCounterBlock(int startACQ=0);

   int getMoveFlag(int imod);


   slsDetector *getSlsDetector(int pos) {if (pos>=0 && pos< MAXDET) return detectors[pos]; return NULL;};

 protected:
 

  /** Shared memory ID */
  int shmId;

  /** pointers to the slsDetector structures */
  slsDetector *detectors[MAXDET];

  /** Shared memory structure */
  sharedMultiSlsDetector *thisMultiDetector;





};



#endif
