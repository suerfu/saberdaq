#ifndef LINUX
    #define LINUX
#endif

#ifndef DIGITIZERV1720_H
#define DIGITIZERV1720_H 1

#include <cstdint>

#include "CAENVMElib.h"
#include "CAENVMEtypes.h"

#include "VMEBoard.h"

#include "CAENV1720Parameter.h"

/// Digitizer class providing necessary definition of registers, and access and control of registers.

class CAENV1720 : public VMEBoard<CAENV1720Parameter>{

public:

    /*  constructor and destructor */
    CAENV1720( int32_t h, CAENV1720Parameter p=CAENV1720Parameter() );
        //!< Constructor. Will initialize parameters from the V1720 parameter object.
    ~CAENV1720();
        //!< Destructor. Should free any memory allocated.

    /*  local channel settings to channel n */
    void SetThreshold(int i, uint32_t th);
        //!< Set threshold.
        //
        //!< First argument is channel number and second is threshold. Threshold is a 12-bit number.
    uint32_t GetThreshold(int i);
        //!< Returns threshold.

    void SetTimeCrossThreshold(int i, uint32_t nsamp);
        //!< Time above/below threshold.
        //
        //!< First argument is channel number and second is number of samples above the threshold.
    uint32_t GetTimeCrossThreshold(int i);
        //!< Return current setting for time above/below threshold for local trigger.

    /*  DC offset of channel n  */
    void SetDAC(int i, uint32_t a);
        //!< DC offset. First arg is channel number and second is DC offset in 16-bit number.
    uint32_t GetDAC(int i);
        //!< Returns 16-bit DC offset.
    bool DACUpdated(int i=-1);
        //!< If argument is not in [0,7], it will check all channels and return false if at least one channel is not ready.

    /*  channel global configurations   */
    void EnableTrigOverThresh(bool t);
        //!< If true will enable trigger over threshold. Otherwise it will be trigger under threshold.
    bool TrigOverThreshEnabled();
        //!< Check the status of trigger over/under threshold.

    void EnableTrigOverlap(bool t);
        //!< If true, enables trigger overlap.
    bool TrigOverlapEnabled();
        //!< Check if trigger overlap is enabled.

    void EnableChannel(int i, bool e);
        //!< If true, enables channel in the first argument for data acquisition.
    bool ChannelEnabled(int i);
        //!< Check if channel in the argument is enabled.
    void SetChannelEnableMask(uint32_t);
        //!< Enable/disable channels by writing the enable mask directly. EnableChannel implicitly calls this method.
    int GetNChannelEnabled();
        //!< Get number of channels enabled.

    /*  event organization */
    void SetSample(uint32_t pre, uint32_t post);
        //!< Sets number of pre-trigger and post-trigger samples.
    uint32_t GetPreSample();
        //!< Returns pre-trigger sample.
    uint32_t GetPostSample();
        //!< Returns post-trigger sample.

    uint32_t GetBufferCode();
        //!< Based on the pre- and post-trigger samples, returns appropriate buffer code to write onto the register for setting number of event.
        //
        //!< This function utilizes values of relevant member variables.
    void EnableCustomSize(bool e);
        //!< If enabled, custom-sized event can be acquired (not necessarily 2^n samples).
    uint32_t GetCustomSize();
        //!< Returns the value in the custom size register.

    uint32_t GetNEvtStored();
        //!< Access VME and returns number of events currently stored.


    uint32_t GetTotalSizeInByte();
        //!< Get event size in byte. Useful for reading out into a char* array.
        //!< Includes header and all channels

    uint32_t GetTotalSizeInWord();
        //!< Returns event size in number of 32-bit words.
        //!< Includes header and all channels

    uint32_t GetEvtSizeInSamp();
        //!< Get number of digitizer samples in each event.
        //!< Sample size considers all enabled channels.

    uint32_t GetEvtSizeInByte();
        //!< Get event size in byte. Useful for reading out into a char* array.

    uint32_t GetEvtSizeInWord();
        //!< Returns event size in number of 32-bit words.


    void SetEvtNumberBLT(uint32_t s);
        //!< Max number of events to read out in BLT transfer.

    uint32_t ReadFIFO(uint32_t* , uint32_t );
        //!< Accesses FIFO event buffer to read out. First argument is address to store output. Second argument is bytes to read. Returns bytes read.

    /*  acquisition control */

    void SetRunMode(V1720_RUNMODE rm);
    V1720_RUNMODE GetRunMode();

    void StartBoard();
    void StartBoard(V1720_RUNMODE rm);
        //!< Start board using specified run mode.
    void StopBoard();
        //!< Stop acquisition.

    bool Running();
        //!< Returns true if board is running.
    bool BoardReady();
    bool EventReady();
        //!< Returns true if there is at least one event ready for readout.
    bool EventFull();
        //!< Returns true if event buffer is full.

    /*  Re-initialize   */
    void Reset();
        //!< Resets all registers to factuary default value.
    void SWClear();


    /*  Trigger source for data acquisition*/
    void SWTrigger();
        //!< Triggers the board via software. Does nothing if enable software trigger is false.
    void EnableSoftTrig(bool e);
    void EnableExtTrig(bool e);
    void EnableLocalTrig(int i,bool e);
    void SetTrigSrcMask(uint32_t);

    bool SoftTrigEnabled();
    bool ExtTrigEnabled();
    bool LocalTrigEnabled(int i);

    /*  Front panel trigger output*/
    void EnableFPSWTrigOut(bool e);
    bool FPSWTrigOutEnabled();

    void EnableFPExtTrigOut(bool e);
    bool FPExtTrigOutEnabled();

    void EnableFPLocalTrigOut(int i, bool e);
    bool FPLocalTrigOutEnabled(int i);

    void SetFPTrigOutMask(uint32_t);

    /*  Front-panel output*/
    void SetLogicTTL(bool);
    bool GetLogicTTL();

    void SetLVDSDirection(bool output);
    bool GetLVDSDirection();


private:

    static const int Nchan = 8; //!< number of maximum channels.

    void Initialize();
        //!< This method calls all Config*** methods to update register with value specified by param member variable.

    void ConfigLocalChannel();
        //!< Sets local channel related registers at once using public set-methods.
        //
        //!< This function is private so that nobody calls it after initialization. Subsequent changes should be done via set functions. Related registers are local channel threshold, sample over trigger, DC offset. 

    void ConfigChannel();
        //!< Sets global channel-related registers at once using public set-methods.
        //
        //!< This function is private so that nobody calls it after initialization. Subsequent changes should be done via set functions. Related registers are trigger over/under threshold, enable/disable overlap, 

    void EnableChannels();
        //!< Writes channel enable mask register with value from param object.

    void ConfigBuffer();
        //!< Sets buffer and event organization related registers.
        //
        //!< Relevant registers include buffer organization, post trigger setting and custom size.
    
    void ConfigTrigSource();
        //!< Sets trigger source related registers at once by writing register directly with values from param object.
        //
        //!< This function is private so that nobody calls it after initialization. Subsequent changes should be done via set functions.
    
    void ConfigFPTrigOut();
        //!< Sets front panel output related registers at once.
        //
        //!< This function is private so that nobody calls it after initialization. Subsequent changes should be done via set functions.
    
    void ConfigFPIO();
        //!< Sets front panel I/O related registers at once.
        //
        //!< This function is private so that nobody calls it after initialization. Subsequent changes should be done via set functions.
};

#endif
