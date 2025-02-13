#ifndef SABERH5RECORDER_H
    #define SABERH5RECORDER_H 1

#include <string>

#include "plrsController.h"
#include "plrsModuleRecorder.h"


class SaberHDF5Recorder : public plrsModuleRecorder{

public:
    SaberHDF5Recorder( plrsController* c);

    ~SaberHDF5Recorder();

protected:

    void Configure(){ plrsModuleRecorder::Configure(); }

	void Deconfigure();

    void PreRun();

    virtual void Run();

    void PostRun();

};


extern "C" SaberHDF5Recorder* create_SaberHDF5Recorder( plrsController* c);

extern "C" void destroy_SaberHDF5Recorder( SaberHDF5Recorder* p );


#endif
