#ifndef SABERDAQ_H
    #define SABERDAQ_H 1

#include <cstdint>  // needed for int32_t and uint32_t
#include <vector>
#include <map>
#include <chrono>

#include "ConfigParser.h"
#include "plrsModuleDAQ.h"
#include "plrsController.h"

#include "CAENV1495Parameter.h"
#include "CAENV1720Parameter.h"
#include "CAENV1720.h"
#include "CAENV1495.h"

#include "CAENVMElib.h"

typedef struct {
    string type;
    int32_t link_number;
    int32_t board_number;
    int32_t handle;
} VMEConnection ;


class SaberDAQ : public plrsModuleDAQ {

public:

    SaberDAQ( plrsController* );
        //!< Constructor. All necessary parameters can be retrieved through polaris controller object.


    ~SaberDAQ();
        //!< Destructor


protected:

    void Initialize();

    void Configure();

    void Deconfigure();

    void Deinitialize();

    void StartDAQ();

    void StopDAQ();

    void PreRun();

    void Run();

    void PreEvent();

    void Event();

    void PostEvent();

    void PostRun();

    vector<CAENV1495Parameter> GetTriggerParameter(){ return param_trig;}

    vector<CAENV1720Parameter> GetADCParameter(){ return param_adc;}

    int GetNextModuleID();

private:

    int next_addr;

    CVErrorCodes error_code;
        //!< Store the status of communication with VME.

    vector<int32_t> handles;
        //!< store all handles created. Used in freeing memory.

    std::map< string, VMEConnection > vme_connection;
        //!< Map for all VME connection types and parameters specified in the config file.
        //!< String key is the connection name (connection0, connection1, ...); VMECOnnection struct contains the connection type, link number and board number.

    
    // ********** Board Parameters **********

    vector<CAENV1495Parameter> param_trig;
        //!< Vector for storing fpga trigger parameters.
    
    vector<CAENV1720Parameter> param_adc;
        //!< Vector for storing ADC parameters.

    // ********** Actual VME Board Objects **********
    
    vector<CAENV1495> v1495;
        //!< FPGA trigger instances. Typically there will be only one.
    
    vector<CAENV1720> v1720;
        //!< Digitizer objects. Can be multiple.

    static const int NBUFF = 64;

    bool rand_trig;
        //!< Flag for enabling/disabling random sampling/trigger
    
    float rand_trig_period_ms;
        //!< Flag for enabling/disabling random sampling/trigger
    
    bool rand_trig_via_fpga;
        //!< If true, random trigger will be issued from V1495 board.

    bool ext_trig_to_start;
        //!< Start of board is via external trigger signal.
        
    bool UpdateTimeSinceLastTrigger( );
        //!< Used in auto trigger mode. Return true if time to re-trigger. It will use internal variable trigger rate to compare and update.

    unsigned int event_counter;
        //!< Count total number of events.

    unsigned int total_event_number;
        //!< Total number of events to accumulate.

    std::chrono::time_point<std::chrono::steady_clock> trig_time_prev;
    
    std::chrono::time_point<std::chrono::steady_clock> trig_time_cur;
};


extern "C" SaberDAQ* create_SaberDAQ( plrsController* c);

extern "C" void destroy_SaberDAQ( SaberDAQ* p );

#endif
