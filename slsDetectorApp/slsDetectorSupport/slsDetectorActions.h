#ifndef SLS_DETECTOR_ACTIONS_H
#define SLS_DETECTOR_ACTIONS_H



#include "slsDetectorBase.h"
#include <string>
#include <fstream>



#define MAX_SCAN_LEVELS 2

using namespace std;

/**

@short class implementing the script and scan utilities of the detectors


*/

class slsDetectorActions : public virtual slsDetectorBase
// : public virtual postProcessing 
{
 public :
  /** default constructor */
  slsDetectorActions(){};
  
  /** virtual destructor */
  virtual ~slsDetectorActions(){};

  /** 
      set action 
      \param iaction can be enum {startScript, scriptBefore, headerBefore, headerAfter,scriptAfter, stopScript, MAX_ACTIONS}
      \param fname for script ("" disable)
      \param par for script 
      \returns 0 if action disabled, >0 otherwise
  */
  int setAction(int iaction, string fname="", string par="");

  /** 
      set action script 
      \param iaction can be enum {startScript, scriptBefore, headerBefore, headerAfter,scriptAfter, stopScript, MAX_ACTIONS}
      \param fname for script ("" disable)
      \returns 0 if action disabled, >0 otherwise
  */
  int setActionScript(int iaction, string fname="");


  /** 
      set action 
      \param iaction can be enum {startScript, scriptBefore, headerBefore, headerAfter,scriptAfter, stopScript, MAX_ACTIONS}
      \param par for script
      \returns 0 if action disabled, >0 otherwise
  */
  int setActionParameter(int iaction, string par="");


  /** 
      returns action script
      \param iaction can be enum {startScript, scriptBefore, headerBefore, headerAfter,scriptAfter, stopScript}
      \returns action script
  */
  string getActionScript(int iaction);

    /** 
	returns action parameter
	\param iaction can be enum {startScript, scriptBefore, headerBefore, headerAfter,scriptAfter, stopScript}
	\returns action parameter
    */
  string getActionParameter(int iaction);
  
  /** 
      returns action mode
      \param iaction can be enum {startScript, scriptBefore, headerBefore, headerAfter,scriptAfter, stopScript}
      \returns action mode
  */
  int getActionMode(int iaction);
  
  
  /** 
      set scan 
      \param index of the scan (0,1)
      \param script fname for script ("" disables, "none" disables and overwrites current, "threshold" threshold scan, "trimbits", trimbits scan)
      \param nvalues number of steps (0 disables, -1 leaves current value)
      \param values pointer to steps (if NULL leaves current values)
      \param par parameter for the scan script ("" leaves unchanged)
      \param precision to write the scan varaible in the scan name (-1 unchanged)
      \returns 0 is scan disabled, >0 otherwise
  */
  int setScan(int index, string script="", int nvalues=-1, float *values=NULL, string par="", int precision=-1);
  
  /** set scan script
      \param index of the scan (0,1)
      \param script fname for script ("" disables, "none" disables and overwrites current, "threshold" threshold scan, "trimbits", trimbits scan)
      \returns 0 is scan disabled, >0 otherwise
  */
  int setScanScript(int index, string script=""); 
  /** set scan script parameter
      \param index of the scan (0,1)
      \param script parameter for scan
      \returns 0 is scan disabled, >0 otherwise
  */
  int setScanParameter(int index, string par="");  
  /** set scan script parameter
      \param index of the scan (0,1)
      \param precision scan varaible precision to be printed in file name
      \returns 0 is scan disabled, >0 otherwise
  */
  int setScanPrecision(int index, int precision=-1);

   
  /** set scan steps
      \param index of the scan (0,1)
      \param nvalues number of steps
      \param values pointer to array of values
      \returns 0 is scan disabled, >0 otherwise
  */
  int setScanSteps(int index, int nvalues=-1, float *values=NULL); 
  
  /** get scan step
      \param index of the scan (0,1)
      \param istep step number
      \returns value of the scan variable
  */
  float getScanStep(int index, int istep){if (index<MAX_SCAN_LEVELS && index>=0 && istep>=0 && istep<MAX_SCAN_STEPS) return scanSteps[index][istep]; else return -1;};
  /** 
      returns scan script
      \param iscan can be (0,1) 
      \returns scan script
  */
  string getScanScript(int iscan);

    /** 
	returns scan parameter
	\param iscan can be (0,1)
	\returns scan parameter
    */
  string getScanParameter(int iscan);

   /** 
	returns scan mode
	\param iscan can be (0,1)
	\returns scan mode
    */
  int getScanMode(int iscan);

   /** 
	returns scan steps
	\param iscan can be (0,1)
	\param v is the pointer to the scan steps
	\returns number of scan steps
    */
  int getScanSteps(int iscan, float *v=NULL);


   /** 
	returns scan precision
	\param iscan can be (0,1)
	\returns scan precision
    */
  int getScanPrecision(int iscan);


  /** calculates the total number of steps for the acquisition 
      \returns total number of steps for the acquisitions
  */
  virtual int setTotalProgress()=0;

  /**
     \returns the action mask
  */
  int getActionMask() {if (actionMask) return *actionMask; return 0;};


  /**
     \param index scan level index
     \returns value of the current scan variable
  */
  float getCurrentScanVariable(int index) {return currentScanVariable[index];};
  //  int getScanPrecision(int index) {return scanPrecision[index];};



  
  /**
    set dacs value
    \param val value (in V)
    \param index DAC index
    \param imod module number (if -1 alla modules)
    \returns current DAC value
  */
  virtual float setDAC(float val, dacIndex index , int imod=-1)=0;


  virtual int setThresholdEnergy(int, int im=-1, detectorSettings isettings=GET_SETTINGS)=0;
  virtual int setChannel(long long, int ich=-1, int ichip=-1, int imod=-1)=0;


  int setStartIndex(int i=-1){if (i>=0) {startIndex=i; lastIndex=startIndex; nowIndex=startIndex;};return startIndex;};
  int setLastIndex(int i=-1){if (i>=0 && i>lastIndex) lastIndex=i; return lastIndex;};
  
  
 protected:


  int executeScan(int level, int istep);
  int executeAction(int level);



  /** action mask */
  int *actionMask;  
  /** array of action scripts */
  mystring *actionScript;	
  /** array of actionparameters */
  mystring *actionParameter; 	      

  /** pointer to number of steps [2] */
  int *nScanSteps;		      
  /** pointer to arrays of step values [2] */
  mysteps *scanSteps;	 	    
  /** pointer to array of scan mode [2] */
  int *scanMode;		    

  /** POINTER TO ARRAY OF SCAN PRECISION [2] */
  int *scanPrecision;

  /** pointer to array of scan scripts [2] */
  mystring *scanScript; 

  /** pointer to array of scan parameters [2] */
  mystring *scanParameter; 






  /**
     current scan variable of the detector
  */
  float currentScanVariable[MAX_SCAN_LEVELS];
  
  /**
     current scan variable index of the detector
  */
  int currentScanIndex[MAX_SCAN_LEVELS];
  



 private:
  int startIndex;
  int lastIndex;
  int nowIndex;
  string fName;

  




};
#endif
