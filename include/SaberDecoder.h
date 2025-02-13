#ifndef SABERDECODER_H
    #define SABERDECODER_H 1


#include "CAENV1495Parameter.h"
#include "CAENV1720Parameter.h"

#include "ConfigParser.h"

#include "SaberRawWaveform.h"

#include "H5FileManager.h"

#include <fstream>
#include <vector>

using namespace std;


/// Decoder class used to read SABRE raw data file and retrieve waveforms.
class SaberDecoder {

public:

    SaberDecoder(){
        input = 0;
        begin_time = end_time = 0;
        bytes_total = 0;
        bytes_header_end = bytes_header_beg = 0;
        bytes_per_event = 0;
    }
        //!< Constructor.

    ~SaberDecoder(){;}
        //!< Destructor
    

    int Decode( ifstream* );
        //!< Function call to decode the event.

    void WriteHDF5( H5FileManager* man);
        //!< This function will write the contents of SABRE raw file into an HDF5 file managed by the manager.
        //!< The H5FileManager must have opened an HDF5 file.
        //!< Input argument is a pointer to HDF5 file manager

    uint32_t GetBeginTime(){ return begin_time;}
        //!< Return the starting time of the run

    uint32_t GetEndTime(){ return end_time;}
        //!< Return the ending time of the run

    ConfigParser GetConfigParser(){ return config;}
        //!< Return the config parser object used in the run

    uint64_t GetEventNumber(){
        if(bytes_per_event>0)
            return (bytes_total-bytes_header_beg-bytes_header_end)/bytes_per_event;
        else
            return 0;
    }
        //!< Get total number of events in the raw file
        //!< Event number is calculated from size of file minus header size, divided by size of each event.


    vector<CAENV1495Parameter> GetTrigParameter();
        //!< Get trigger settings used in the run.
        //!< For details, refer to V1495 parameter class.
    

    vector<CAENV1720Parameter> GetADCParameter();
        //!< Get ADC settings used in the run.
        //!< For details, refer to V1720 parameter class.


    vector<SaberRawWaveform> GetEvent( uint64_t index);
        //!< Read input from stream, and decode event according to the given ADC parameters in the header.
        //!< function returns SaberRawWaveform object that contains waveform, threshold, sampling frequency, channel descriptor, etc.
        //!< Each entry in the vector corresponds to waveforms in each channels.


private:

    uint32_t begin_time;
        //!< Begin time of the run.
    
    uint32_t end_time;
        //!< End time of the run.

    uint64_t bytes_total;
        //!< Total number of bytes for the entire file.
        //!< Since there could be overflow, uint64 is used instead of 32.

    uint32_t bytes_per_event;
        //!< Total number of bytes per event.

    uint32_t bytes_header_beg;
        //!< Number of bytes in the global header.
        //!< Used in skipping correct number of bytes in random access of event.

    uint32_t bytes_header_end;
        //!< Number of bytes in the global ending header.

    ifstream* input;
        //!< Input stream
        //!< Everytime this is set, parameters will refresh.

    ConfigParser config;
        //!< Configuration parser

    vector<CAENV1495Parameter> param_trig;
        //!< Trigger parameters

    vector<CAENV1720Parameter> param_adc;
        //!< ADC parametersd
    
    int SetInputStream( ifstream* );
        //!< Used to set and specify raw file
        //!< In case of corrupted input stream, negative values are returned.

    int DecodeHeader();
        //!< Calling this function will start decoding the header.
    

    uint32_t Probe( ifstream& );
        //!< Get the next word without extracting

    vector<uint32_t> Probe( ifstream&, int n);
        //!< Probe next n words
        //!< Returned as a vector

    uint32_t Read( ifstream& );
        //!< Extract next word for reading

};

#endif
