#ifndef PULSE_H
    #define PULSE_H 1

#include "SaberRawWaveform.h"
#include "Waveform.h"

#include <algorithm>
#include <iostream>

struct PulseInfo{
    int begin;
    int end;
    int start_time;
};

/// Pulse object. It contains processed information such as pulse height and integral.
class Pulse : public Waveform<float>{

public:

    Pulse(){
        start_time = -1;
        polarity = true;
    }

    ~Pulse(){;}



    virtual float GetPulseHeight(){
        return *std::min_element( begin(), end());
    }
        //!< Pulse height after baseline subtraction

    virtual float GetPulseIntegral(){
        float integral = 0;
        for( unsigned int i=0; i<size()-1; i++){
            integral += ( (*this)[i] + (*this)[i+1])/2;
        }
        return integral;
    }
        //!< Pulse integral from begin to end



    int GetPulseDuration(){
        return (*this).size();
    }
        //!< Return length of pulse in number of samples.


    int GetTimeTag(){
        return time_tag;
    }


    void SetTimeTag( int s){
        time_tag = s;
    }


    int GetPulseStartTime(){
        return start_time;
    }
        //!< Pulse starting time in number of samples from beginning of waveform.


    void SetPulseStartTime( int s){
        start_time = s;
    }
        //!< Sets the starting time. In number of samples from beginning of waveform.


    int GetPulseMinTime(){
        return std::min_element( begin(), end()) - begin() + start_time;
    }
        //!< Return position of minimum pulse height in number of samples from beginning of pulse.


    int GetPulseMaxTime(){
        return std::max_element( begin(), end()) - begin() + start_time;
    }
        //!< Return position of minimum pulse height in number of samples from beginning of pulse.


    float GetF90(){
        float integral0 = 0;
        float integral1 = 0;
        for( unsigned int i=0; i<size()-1; i++){
            integral0 += ( (*this)[i] + (*this)[i+1])/2;
            if( i<90/4 )
                integral1 += ( (*this)[i] + (*this)[i+1])/2;
        }
        return integral1/integral0;
    }


    void SetPolarity( bool p){
        polarity = p;
    }

    bool GetPolarity(){
        return polarity;
    }

    void SetBaseline( float bsln ){ baseline = bsln;}

    float GetBaseline(){ return baseline;}

private:
    
    int time_tag;
        //!< position of the first sample of pulse in the entire waveform.

    int start_time;
        //!< start time of the pulse, defined as the first sample outside the variance.

    bool polarity;
        //!< Polarity. True if positive polarity.

    float baseline;
        //!< offset of the sample.

};


#endif
