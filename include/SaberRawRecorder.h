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

    void Configure();

	void Deconfigure();

    void PreRun();

    virtual void Run();

    void PostRun();

private:

    int adc_total_size_in_byte;
};


extern "C" SaberRawRecorder* create_SaberRawRecorder( plrsController* c);

extern "C" void destroy_SaberRawRecorder( SaberRawRecorder* p );


#endif
