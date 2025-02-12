#ifndef FLATBASELINEFINDER_H
    #define FLATBASELINEFINDER_H 1

#include "BaselineFinder.h"

class FlatBaselineFinder : public BaselineFinder{

public:

    FlatBaselineFinder(){}

    ~FlatBaselineFinder(){}

    SaberRawWaveform GetBaseline( SaberRawWaveform  waveform);
};


#endif
