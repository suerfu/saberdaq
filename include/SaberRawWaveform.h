// Object containing information about raw waveform - raw in that original digitizer count is stored.

#ifndef SABERRAWWAVEFORM_H
#define SABERRAWWAVEFORM_H 1

#include "Waveform.h"

#include <vector>
#include <algorithm>
#include <string>
#include <stdint.h>

using std::vector;
using std::string;


/// This class contains information that is available in the raw data file. It does not contain processed information such as pulse height or integral.
class SaberRawWaveform : public Waveform<uint16_t>{

public:

    SaberRawWaveform();
        //!< Default constructor

    SaberRawWaveform( const SaberRawWaveform& rhs ); 
        //!< Copy constructor

    ~SaberRawWaveform();
        //!< Destructor

    SaberRawWaveform& operator=( const SaberRawWaveform& rhs );
        //!< Assignment operator


    uint64_t GetEventID();
        //!< Get event ID. This is the event ID obtained from board.
    void SetEventID( uint64_t s );
        //!< Get event ID. This is the event ID obtained from board.


    int GetBoardID();
        //!< Get board ID
    void SetBoardID( int s );
        //!< Set board ID


    int GetChannelID();
        //!< Get channel ID
    void SetChannelID( int s );
        //!< Set channel ID


    int GetDescriptor();
        //!< Get descriptor
    void SetDescriptor( int s );
        //!< Set descriptor


    string GetLabel();
        //!< Get label. Label is a string that can be used to identify waveform.
    void SetLabel( string s);
        //!< Set label.


    uint32_t GetTrigTimeTag();
        //!< Get trigger time tag.
    void SetTrigTimeTag( uint32_t s );
        //!< Set trigger time tag.

    void SetOptionField( uint32_t a);
    uint32_t GetOptionField(){ return option; };

    int GetThreshold();
    int GetThreshold() const;
    void SetThreshold( int s );

    int GetTXThreshold();
        //!< Get time cross threshold.
    void SetTXThreshold( int s );
        //!< Set time cross threshold.

    vector<int> GetTXThreshold( int level, bool pos_polarity = false);
        //!< Returns a vector of time stamps where waveform crosses a set level

    int GetDAC();
        //!< Get DAC offset
    void SetDAC( int s );
        //!< Set DAC offset


    int GetPreTrigSample();
    void SetPreTrigSample( int s );

    int GetPostTrigSample();
    void SetPostTrigSample( int s );

    int GetSampleInterval();
    void SetSampleInterval( int s );

    int GetMax(){ return *std::max_element( cbegin(), cend());}

    int GetMin(){ return *std::min_element( cbegin(), cend());}

    int GetMax( int i, int j){ return *std::max_element( cbegin()+i, cbegin()+j);}

    int GetMin( int i, int j){ return *std::min_element( cbegin()+i, cbegin()+j);}

    int GetMaxIndex(){ return std::max_element( cbegin(), cend()) - cbegin(); }

    int GetMinIndex(){ return std::max_element( cbegin(), cend()) - cbegin(); }

private:

    uint64_t event_id;

    int board_id;

    int channel_id;

    int sample_interval;
        // time between two samples in ns.

    uint32_t trig_time_tag;

    int threshold;

    int txthreshold;

    int dac;

    int option;

    int descriptor;

    string label;

    int pre_trig_sample;

    int post_trig_sample;
};


#endif
