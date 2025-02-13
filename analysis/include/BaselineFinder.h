#ifndef BASELINEFINDER_H
    #define BASELINEFINDER_H 1

#include "SaberRawWaveform.h"

class BaselineFinder{

public:

    BaselineFinder(){}

    ~BaselineFinder(){}

    virtual SaberRawWaveform GetBaseline( SaberRawWaveform wfm ) = 0;
};


#endif
