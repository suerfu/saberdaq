#ifndef TESTDAQ_H
    #define TESTDAQ_H 1

#include <cstdint>  // needed for int32_t and uint32_t
#include <vector>
#include <map>
#include <chrono>

#include <cmath>

#include "ConfigParser.h"
#include "plrsModuleDAQ.h"
#include "plrsController.h"

#include "SaberDAQ.h"

#include "CAENV1495Parameter.h"
#include "CAENV1720Parameter.h"


typedef int (*TestFunction)( float x, float range);

int TriPulse( float x, float range);

int RecPulse( float x, float range);

int Gauss( float x, float range);

int Scint( float x, float range );



class TestDAQ : public plrsModuleDAQ {

public:

    TestDAQ( plrsController* );
        //!< Constructor. All necessary parameters can be retrieved through polaris controller object.

    ~TestDAQ();
        //!< Destructor

protected:

    void Initialize();

    void Configure();

    void UnConfigure();

    void CleanUp();

    void StartDAQ();

    void StopDAQ();

    void PreRun();

    void PreEvent();

    void Event();

    void PostEvent();

    void PostRun();

    vector<CAENV1495Parameter> GetTriggerParameter(){ return param_trig;}

    vector<CAENV1720Parameter> GetADCParameter(){ return param_adc;}

private:

    std::map< string, VMEConnection > vme_connection;

    vector<CAENV1495Parameter> param_trig;
    
    vector<CAENV1720Parameter> param_adc;

    static const int NBUFF = 64;

    bool rand_trig;
    
    float rand_trig_period;
    
    bool rand_trig_via_fpga;

    bool ext_trig_to_start;
        
    bool UpdateTimeSinceLastTrigger( );

    unsigned int event_counter;

    unsigned int total_event_number;

    std::chrono::time_point<std::chrono::steady_clock> trig_time_prev;
    
    std::chrono::time_point<std::chrono::steady_clock> trig_time_cur;

    void ReadFIFO( CAENV1720Parameter, uint32_t* bytes, uint32_t bytes_to_read );

    TestFunction ftest[8];
};


extern "C" TestDAQ* create_TestDAQ( plrsController* c);

extern "C" void destroy_TestDAQ( TestDAQ* p );

#endif
