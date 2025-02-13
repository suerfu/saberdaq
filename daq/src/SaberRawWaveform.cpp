
#include "SaberRawWaveform.h"

#include <iostream>

SaberRawWaveform::SaberRawWaveform(){;}


SaberRawWaveform::SaberRawWaveform( const SaberRawWaveform& rhs) : Waveform( rhs ) { 
    event_id = rhs.event_id;

    board_id = rhs.board_id;
    channel_id = rhs.channel_id;

    sample_interval = rhs.sample_interval;

    trig_time_tag = rhs.trig_time_tag;

    threshold = rhs.threshold;
    txthreshold = rhs.txthreshold;
    dac = rhs.dac;
    descriptor = rhs.descriptor;

    label = rhs.label;

    pre_trig_sample = rhs.pre_trig_sample;
    post_trig_sample = rhs.post_trig_sample;
}


SaberRawWaveform::~SaberRawWaveform(){}


SaberRawWaveform& SaberRawWaveform::operator=( const SaberRawWaveform& rhs){
    event_id = rhs.event_id;

    board_id = rhs.board_id;
    channel_id = rhs.channel_id;

    sample_interval = rhs.sample_interval;

    trig_time_tag = rhs.trig_time_tag;

    threshold = rhs.threshold;
    txthreshold = rhs.txthreshold;
    dac = rhs.dac;
    descriptor = rhs.descriptor;

    label = rhs.label;

    pre_trig_sample = rhs.pre_trig_sample;
    post_trig_sample = rhs.post_trig_sample;
    return *this;
}


uint64_t SaberRawWaveform::GetEventID(){
    return event_id;
}


void SaberRawWaveform::SetEventID( uint64_t s) {
    event_id = s;
}


int SaberRawWaveform::GetBoardID(){
    return board_id;
}


void SaberRawWaveform::SetBoardID( int s) {
    board_id = s;
}


int SaberRawWaveform::GetChannelID(){
    return channel_id;
}


void SaberRawWaveform::SetChannelID( int s) {
    channel_id = s;
}


int SaberRawWaveform::GetSampleInterval(){ 
    return sample_interval;
}


void SaberRawWaveform::SetSampleInterval( int s){ 
    sample_interval = s;
}


uint32_t SaberRawWaveform::GetTrigTimeTag(){ 
    return trig_time_tag;
}


void SaberRawWaveform::SetTrigTimeTag( uint32_t s){ 
    trig_time_tag = s;
}


int SaberRawWaveform::GetThreshold(){ 
    return threshold;
}


int SaberRawWaveform::GetThreshold() const { 
    return threshold;
}


void SaberRawWaveform::SetThreshold( int s){ 
    threshold = s;
}


int SaberRawWaveform::GetTXThreshold(){ 
    return txthreshold;
}


void SaberRawWaveform::SetTXThreshold( int s){ 
    txthreshold = s;
}


vector<int> SaberRawWaveform::GetTXThreshold( int level, bool pos_polarity){
    vector<int> result;
    for( unsigned int i=0; i<size()-1; i++){
        if( pos_polarity){
            if( (*this)[i]<level && (*this)[i+1]>=level )
                result.push_back( i+1 );
        }
        else{
            if( (*this)[i]>level && (*this)[i+1]<=level )
                result.push_back( i+1 );
        }
    }
    return result;
}


int SaberRawWaveform::GetDAC(){ 
    return dac;
}


void SaberRawWaveform::SetDAC( int s){ 
    dac = s;
}


int SaberRawWaveform::GetDescriptor(){ 
    return descriptor;
}


void SaberRawWaveform::SetDescriptor( int s){ 
    descriptor = s;
}


string SaberRawWaveform::GetLabel(){ 
    return label;
}


void SaberRawWaveform::SetLabel( string s){ 
    label = s;
}


int SaberRawWaveform::GetPreTrigSample(){ 
    return pre_trig_sample;
}


void SaberRawWaveform::SetPreTrigSample( int s){ 
    pre_trig_sample = s;
}


int SaberRawWaveform::GetPostTrigSample(){ 
    return post_trig_sample;
}


void SaberRawWaveform::SetPostTrigSample( int s){ 
    post_trig_sample = s;
}

