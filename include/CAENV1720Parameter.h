#ifndef CAENV1720PARAMETER_H
    #define CAENV1720PARAMETER_H

#include "VMEBoardParameter.h"
#include "ConfigParser.h"
#include "CAENV1720Registers.h"

#include <cstdint>
#include <string>

#include "H5FileManager.h"

class CAENV1720ChannelParameter{

public:

    //CAENV1720ChannelParameter();

    //~CAENV1720ChannelParameter();

    //CAENV1720ChannelParameter& operator= (const CAENV1720ChannelParameter& rhs); 

    unsigned int board_id;

    unsigned int channel_id;

    /* local channel setting */
    uint32_t threshold;
        //!< Local channel threshold.

    uint32_t tcrossthresh;
        //!< # of sample that signal must stay above/below threshold to issue a local trigger.
    
    uint32_t dac;
        //!< DAC register controls DC offset applied to the signal.

    uint32_t descriptor;
        //!< Custom pattern to describe characteristics of channels.

    string label;
        //!< A string for each channel that labels what the channel is. Typically illustrates the type of input.

    string name;
        //!< Used to uniquely identify a channel.

    int pre_trig_sample;

    int post_trig_sample;
        //!< number of samples in the channel's data.
};



/// Parameters of CAEN V1720 digitizer.
//
class CAENV1720Parameter : public VMEBoardParameter{

public:

    CAENV1720Parameter();
        // handle by default 0.

    ~CAENV1720Parameter();


    void SetParamFromConfig( ConfigParser*, string s="/module/daq/board0/");
        //!< It returns a V1720 parameter object from configuration parser.
        //!< This method will be used to initialize the parameter object for the board from user-specified config file.


    uint32_t GetVersion(){
        return 2;
    }


    uint32_t GetModel(){
        return 1720;
    }
        //!< The model of the ADC board used.


    string GetPrintString();
        //!< Return a string summarizing all parameters.


    static const int NCHANNEL = 8;
        //!< Total number of physical channels on the board


    CAENV1720ChannelParameter channel_param[ NCHANNEL ];
        //!< Fixed-length array that holds the actual data


    unsigned int GetNChannel(){ return NCHANNEL;}
        //!< CAENV1720 has fixed 8 channels.


    vector< CAENV1720ChannelParameter > GetEnabledChannels(){ 
        vector< CAENV1720ChannelParameter > result;
        for( unsigned int i=0; i<NCHANNEL; i++ ){
            if( ChannelNEnabled(i) ){
                result.push_back( channel_param[i] );
            }
        }
        return result;
    }
        //!< Returns a vector containing list of enabled channels.


    unsigned int GetNChannelEnabled();
        //!< Returns number of enabled channels.


    bool ChannelNEnabled( int i ){
        return ( (ch_enable_mask & (0x1<<i)) != 0 );
    }
        //!< Checks if channel i is enabled.
        

    uint32_t GetTotalSizeInWord();
        //!< Size of entire event, including headers and all channels.


    uint32_t GetTotalSizeInByte();
        //!< Size of entire event, including headers and all channels in number of bytes.


    uint32_t GetEvtSizeInSamp();
        //!< Get number of samples in one event. This does not include event header, but includes all channels.


    uint32_t GetEvtSizeInWord();
        //!< Returns number of 32-bit words in one event. This includes event header.


    uint32_t GetEvtSizeInByte();
        //!< Returns event size in bytes. This includes event header.


    uint32_t GetChanSizeInSamp();
        //!< Number of ADC samples per channel

    uint32_t GetChanSizeInWord();
        //!< Size of event per channel in number of 32-bit words

    uint32_t GetChanSizeInByte();
        //!< Szie of event per channel in number of bytes.


    /* channel global setting */
    uint32_t ch_enable_mask;
        //!< This mask controls which channels are enabled for data recording.
    
    bool trig_over_threshold;
        //!< If true, trigger issued when signal goes above threshold. Otherwise it is below threshold.
    
    bool trig_overlap;
        //!< If true, trigger overlap is enabled.



    /* event organization */
    bool enable_custom_size;
        //!< Custom size of number of samples of an event. (not power of 2).
    
    uint32_t pre_trig_sample;
        //!< Number of pre-trigger samples.
    
    uint32_t post_trig_sample;
        //!< Number of post-trigger samples.
    
    uint32_t buff_code;
        //!< Buffer code that divides the memory.



    /* acquisition control */
    V1720_RUNMODE runmode;


    /* board firmware */
    uint32_t board_fw_roc;

    uint32_t board_fw_amc;


    /* front panel IO control */
    bool lvds_io_output;
        //!< If true, LVDS front panel will be set as output.
    
    bool logic_level_ttl;
        //!< If true, front panel uses TTL signal level.



    /* trigger source  and front panel output*/
    bool sw_trig_enable;
        //!< If enabled, write access to software trigger register will trigger data acquisition.
    
    bool sw_fp_trigout;
        //!< If true, software trigger is also propagated to the LEMO output on the front panel.

    bool ext_trig_enable;
        //!< If true, external trigger input will trigger DAQ.
    
    bool ext_fp_trigout;
        //!< If true, external trigger received will be propagated to LEMO output on front panel.

    uint32_t local_trig_enable;
        //!< Local trigger by threshold can generate local trigger for acquisition

    uint32_t coin_level;
        //!< Coincidence level used in local auto-trigger. Board triggers when > coin_level. Default 0
        //!< maximum value is 7.

    uint32_t coin_window;
        //!< length of coincidence window in number of clock cycles.
        //!< maximum value is 15.

    uint32_t local_fp_trigout;
        //!< Local trigger by threshold will output TTL/NIM signal on the LEMO output on the front panel.
        //!< This output is not the LVDS signal.

    unsigned int GetHeaderSize();

    void Serialize( char* p );

    void Deserialize( ifstream& p );

    void Deserialize( char* p, bool flip = false );

    void ExportHDF5( H5FileManager* h5man);
        //!< Export the parameter settings into HDF5 file.
        //!< The H5 Manager must be managing an open file.
};


#endif
