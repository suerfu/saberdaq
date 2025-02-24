#ifndef SABERBOARDRAWDATA_H
    #define SABERBOARDRAWDATA_H 1

#include <vector>
#include <cstdint>

class SaberBoardRawData {

public:

    // constructors and destructor

    SaberBoardRawData();

    ~SaberBoardRawData();

    SaberBoardRawData( const SaberBoardRawData& rhs);

    SaberBoardRawData& operator = ( const SaberBoardRawData& rhs);


    // related to raw memory

    unsigned int samp_per_chan();
        // number of samples per channel in an event

    unsigned int size();
        // number of data samples total allocated for storing waveform
        // headers excluded
    
    unsigned int size() const;

    unsigned int bytes();
        // number of data samples in bytes

    void AllocateBuffer( unsigned int n);

    uint32_t* GetBufferAddr();


    // access details of the event raw data

    bool Valid();

    int operator[]( int n);

    int operator[]( int n) const;

    unsigned int GetEventSize();

    unsigned int GetChannelMask();

    unsigned int GetNChannelEnabled();

    int GetBoardID();

    unsigned int GetEventID();

    uint32_t GetTimeTag();

    int GetNSignal( int dev);

private:

    std::vector<uint32_t> buffer;

};

#endif
