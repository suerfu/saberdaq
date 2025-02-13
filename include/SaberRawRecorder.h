#ifndef SABERRAWRECORDER_H
    #define SABERRAWRECORDER_H 1

#include <string>

#include "plrsController.h"
#include "plrsModuleRecorder.h"


class SaberRawRecorder : public plrsModuleRecorder{

public:
    SaberRawRecorder( plrsController* c);

    ~SaberRawRecorder();

protected:

    void Configure(){ plrsModuleRecorder::Configure(); }

	void Deconfigure();

    void PreRun();

    virtual void Run();

    void PostRun();

};


extern "C" SaberRawRecorder* create_SaberRawRecorder( plrsController* c);

extern "C" void destroy_SaberRawRecorder( SaberRawRecorder* p );


#endif
