#ifndef SABERDAQDATA_H
    #define SABERDAQDATA_H 1

#include <ostream>
#include <vector>

#include "SaberBoardRawData.h"
#include "CAENV1495Parameter.h"
#include "CAENV1720Parameter.h"


class SaberDAQData {

public:

    SaberDAQData();
        //!< Default constructor

    SaberDAQData( std::vector<CAENV1720Parameter> adc, CAENV1495Parameter trigger);
        //!< Constructor from array of board parameters

    SaberDAQData( const SaberDAQData& rhs);
        //!< Copy constructor

    ~SaberDAQData(){ ; };
        //!< Destructor

    SaberDAQData& operator=(const SaberDAQData& rhs);
        //!< Assignment operator

    void AddBoardData( const SaberBoardRawData& b);
        //!< Function to add raw data

    CAENV1495Parameter GetTriggerParameter();
    
    void AddTriggerParameter( const CAENV1495Parameter& a);
        //!< Function to add/change trigger parameter
    
    vector<CAENV1720Parameter> GetADCParameters();
    
    void AddADCParameter( const CAENV1720Parameter& a);

    virtual void Write( ostream& os);
        //!< Function to write output directly to file
        //!< Declared to be virtual so that header object can write differently

    int size(){ return board_data.size();}
        //!< Returns number of boards recorded in this object.

    SaberBoardRawData& operator[]( unsigned int n);
        //!< Element access operator. If argument exceeds vector length, will result in unpredictable behavior.

    SaberBoardRawData GetBoardData( int n);
        //!< Returns board data object. If argument exceeds vector length, will return default board data object.
        
    SaberBoardRawData* GetBoardDataPtr( int n);
        //!< Returns a pointer to a board data object. If argument exceeds vector length, it will return 0 to indicate an error.

    int GetNSignal( int min_dev);

    string GetVersion(){ return "1.0.0";}

    // Function to set and retrieve the entire configuration file
    //
    void SetConfig( string a){ config = a; }

    string GetConfig(){ return config; }

    // Function to set and retrieve comment used in the run
    //
    void SetComment( string a ){ comment = a; }

    string GetComment(){ return comment; }

    // Function to set and retrieve begin and end timestamp
    //
    void SetTimeStamp( uint32_t a ){ timestamp = a; }
    
    uint32_t GetTimeStamp(){ return timestamp; }

    void SetRandomTriggerPeriod( uint32_t a){ random_trigger_period = a; }

    uint32_t GetRandomTriggerPeriod(){ return random_trigger_period; }

    void SetRandomTriggerSource( string a){ random_trigger_src = a; }
    
    string GetRandomTriggerSource(){ return random_trigger_src; }


    // Functions related to header information
    //

    bool IsHeader(){ return (board_data.size()!=0); }

    /*
    uint32_t GetHeader(){
        uint32_t val = 0;
        if( header.size()>0 )
            memcpy( &val, &header[0], sizeof( val ) );
        return val;
    }

    void CopyHeader( void* v, int bytes){
        char* p = reinterpret_cast<char*>(v);
        header.clear();
        for( int i=0; i<bytes; i++){
            header.push_back(p[i]);
        }
    }

    void Write( ostream& os){
        if( header.size()>0 )
            os.write( &header[0], header.size() );
    }
    */

    void SetEventIndex( uint32_t a){ event_index = a; }

    uint32_t GetEventIndex(){ return event_index; }

private:

    // board data stores actual waveform
    std::vector<SaberBoardRawData> board_data;

    // header information to store ADC and trigger configuration
    vector<CAENV1720Parameter> adc_parameters;

    CAENV1495Parameter trigger_parameter;

    /// Information for global headers, run time, etc.
    
    string comment;

    string config;

    uint32_t timestamp;
    
    uint32_t random_trigger_period;

    uint32_t event_index;

    string random_trigger_src;

};

#endif
