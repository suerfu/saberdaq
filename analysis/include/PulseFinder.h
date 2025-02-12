#ifndef PULSEFINDER_H
    #define PULSEFINDER_H 1

#include <map>
#include <vector>

#include "ConfigParser.h"

#include "Pulse.h"
#include "SaberRawWaveform.h"

using std::map;
using std::vector;

/// PulseFinder general base class.
/// Provides parameter registration and access methods, and pulse finding algorithms.
class PulseFinder {

public:


    PulseFinder();

    ~PulseFinder();
/*
    void RegisterParameter( string, int);
        //!< Registers int parameter.

    void RegisterParameter( string, float);
        //!< Registers float parameter.

    int GetInt( string s ){ return parameter_int[s];}

    float GetFloat( string s ){ return parameter_float[s];}
*/

    virtual void SetParamFromConfig( ConfigParser config, string dirname) = 0;
        //!< Initializes parameters using config file and directory name.

    virtual std::vector<PulseInfo> FindPulse( SaberRawWaveform input, SaberRawWaveform baseline ) = 0;
        //!< Virtual method for running pulse finding algorithm.

/*
protected:

    map<string, int> parameter_int;

    map<string, float> parameter_float;
*/
};

#endif
