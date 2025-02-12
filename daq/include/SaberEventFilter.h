#ifndef SaberEventFilter_H
    #define SaberEventFilter_H 1

#include <string>

#include "plrsController.h"
#include "plrsModuleRecorder.h"

#include "SaberDAQData.h"

class SaberEventFilter : public plrsStateMachine{

public:

    SaberEventFilter( plrsController* c);

    ~SaberEventFilter();

    bool Pass( SaberDAQData* d);

    string GetModuleName(){ return "filter";}

protected:

    void Configure();

	void Deconfigure();

    void PreRun();

    virtual void Run();

    void PostRun();

//    void CleanUp(){;}

    int addr_prev;

    int min_dev;

    int nsig;

private:

    unsigned int evt_counter;

    unsigned int max_event;
};


extern "C" SaberEventFilter* create_SaberEventFilter( plrsController* c);

extern "C" void destroy_SaberEventFilter( SaberEventFilter* p );


#endif
