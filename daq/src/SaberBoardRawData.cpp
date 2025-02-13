#include "SaberBoardRawData.h"

#include <iostream>


SaberBoardRawData::SaberBoardRawData(){;}


SaberBoardRawData::~SaberBoardRawData(){;}


SaberBoardRawData::SaberBoardRawData( const SaberBoardRawData& rhs){
    buffer = rhs.buffer;
}


SaberBoardRawData& SaberBoardRawData::operator = ( const SaberBoardRawData& rhs){
    buffer = rhs.buffer;
    return *this;
}


int SaberBoardRawData::samp_per_chan(){
    return size()/GetNChannelEnabled();
}


int SaberBoardRawData::size(){
    return 2*(buffer.size()-4);
}


int SaberBoardRawData::size() const{
    return 2*(buffer.size()-4);
}


int SaberBoardRawData::bytes(){
    return sizeof(buffer[0])*buffer.size();
}


void SaberBoardRawData::AllocateBuffer( unsigned int n){
    for( unsigned int i=buffer.size(); i<n/4; ++i)
        buffer.push_back(0);
}


uint32_t* SaberBoardRawData::GetBufferAddr(){
    return &buffer[0];
}


int SaberBoardRawData::operator[]( int n){
    int i = n/2;
    if( 4+i >= static_cast<int>(buffer.size()) )
        return -1;
    int j = n%2;
    if( j==0 ){
        return ( buffer[4+i] & 0xfff);
    }
    else{
        return ( (buffer[4+i]>>16) & 0xfff);
    }
}


int SaberBoardRawData::operator[]( int n) const{
    int i = n/2;
    if( 4+i >= static_cast<int>(buffer.size()) )
        return -1;
    int j = n%2;
    if( j==0 ){
        return ( buffer[4+i] & 0xfff);
    }
    else{
        return ( (buffer[4+i]>>16) & 0xfff);
    }
}


bool SaberBoardRawData::Valid(){
    uint32_t a = (buffer[0]>>28)&0xf;
    unsigned int e = GetEventSize();

    if( a==0xa && e==buffer.size()/4)
        return true;
    else
        return false;
}


int SaberBoardRawData::GetEventSize(){
    return buffer[0]&0x0fff;   
}


unsigned int SaberBoardRawData::GetChannelMask(){
    return buffer[1]&0xff;
}


int SaberBoardRawData::GetNChannelEnabled(){
    uint32_t t = buffer[1]&0xff;
    int n = 0;
    for( int i=0; i<8; i++)
        if( ( t&(0x1<<i) ) != 0 )
            n++;
    return n;
}



int SaberBoardRawData::GetBoardID(){
    return (buffer[1]>>27)&0x1f;
}


int SaberBoardRawData::GetEventID(){
    return buffer[2]&0xffffff;
}


uint32_t SaberBoardRawData::GetTimeTag(){
    return buffer[3];
}



int SaberBoardRawData::GetNSignal( int dev){

    int NC = GetNChannelEnabled();
    int NS = size()/NC;
        // parameters of the channel
    int nsig = 0;
        // number of signal with variation above threshold
    int min, max;
        // store min and max value
    for( int i=0; i<NC; i++){
        min = max = (*this)[i*NS];
        for( int j=0; j<NS; j++){
            min = min<(*this)[i*NS+j] ? min : (*this)[i*NS+j];
            max = max>(*this)[i*NS+j] ? max : (*this)[i*NS+j];
            if( max - min >dev){
                nsig++;
                break;
            }
        }
    }
    return nsig;
}
