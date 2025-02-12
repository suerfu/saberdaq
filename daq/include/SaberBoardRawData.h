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

    int samp_per_chan();
        // number of samples per channel in an event

    int size();
    
    int size() const;

    int bytes();

    void AllocateBuffer( unsigned int n);

    uint32_t* GetBufferAddr();


    // access details of the event raw data

    bool Valid();

    int operator[]( int n);

    int operator[]( int n) const;

    int GetEventSize();

    unsigned int GetChannelMask();

    int GetNChannelEnabled();

    int GetBoardID();

    int GetEventID();

    uint32_t GetTimeTag();

    int GetNSignal( int dev);

private:

    std::vector<uint32_t> buffer;

};

#endif
