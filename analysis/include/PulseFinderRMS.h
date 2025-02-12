#ifndef PULSEFINDERRMS_H
    #define PULSEFINDERRMS_H 1

#include "PulseFinder.h"

#include "Waveform.h"
#include "Pulse.h"


/// Pulse finding by identifying peaks.
/// The algorithm will iteratively locate peaks, then expand pulse region from the peak until clean baseline is found.
/// Once clean baseline before the pulse is located, a main window is applied and clean baseline after the main window is searched.
/// After locating clean baseline after the region of interest, interpolation is used to subtract baseline from main region based on the average value before and after the pulse and a Pulse object is returned.
/// Different criteria can be used to judge if baseline is clean, such as variance or maximum deviation.

class PulseFinderRMS : public PulseFinder {

public:

    PulseFinderRMS();

    ~PulseFinderRMS();

    void SetParamFromConfig( ConfigParser config, string dirname);
        //!< Initializes parameters using config file and directory name.

    std::vector<PulseInfo> FindPulse( SaberRawWaveform input, SaberRawWaveform baseline );
        //!< Default algorithm. Searches for pulses from minimum and them move to left and right for clean baseline as cutoff.

private:

    float GetFlatBaseline( const SaberRawWaveform& wave);

    bool TestBaseline( const SaberRawWaveform&, std::vector<uint16_t>::const_iterator beg, std::vector<uint16_t>::const_iterator end, float varthresh, float maxdev );
        //!< Judge whether the baseline is clean enough to mark end of pulse.


public:

    int window;
        //!< Integration window size for pulse in number of samples. Default 1.
        // for NaI, ~ 3 us; PC, short; spe even shorter.

    int search_begin;
        //!< Pulse search begin time.

    int search_end;
        //!< Pulse search end time.

    float threshold;
        //!< Threshold for pulse height to return

    float only_peak_threshold;
        //!< two pulses in the same window must differ by at least this fraction of larger pulse.

    int pre_var_win;
        //!< Pre-pulse window for computing variance
        //
    float pre_var_threshold;
        //!< Threshold on variance for pre-pulse
        //
    float pre_max_dev;
        //!< Maximum deviation in pre-pulse region

    int pre_ncross_0;
        //!< Number of times waveform crosses 0 before the pulse.


    int post_var_win;
        //!< Post-pulse window for computing variance
        //
    float post_var_threshold;
        //!< Threshold on variance for post-pulse
        //
    float post_max_dev;
        //!< Maximum deviation in pre-pulse region
    
    int post_ncross_0;
        //!< Number of times waveform crosses 0 after the pulse.

};



/// Calculate average for the range bounded by the iterators.
template < class T >
float Average( T beg, T end){
    float sum = 0;
    int count = 0;
    for( T itr= beg; itr != end; ++itr ){
        sum += *itr;
        ++count;
    }
    if( count!=0 )
        return sum/count;
    else
        return 0;
}



/// Calculate variance for the range specified by the iterators.
template < class T >
float Variance( T beg, T end){
    float sum = 0;
    int count = 0;
    float avg = Average( beg, end);
    for( T itr= beg; itr != end; ++itr ){
        sum += (*itr-avg) * (*itr-avg);
        ++count;
    }
    if( count!=0)
        return sum/count;
    else
        return 0;
}


#endif
