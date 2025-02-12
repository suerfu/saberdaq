
#include "SaberDecoder.h"

#include "H5FileManager.h"

#include <vector>
#include <sstream>
#include <chrono>

uint32_t SaberDecoder::Read( ifstream& input){
    uint32_t rd;
    input.read( reinterpret_cast<char*>( &rd), sizeof(rd));
    return rd;    
}



uint32_t SaberDecoder::Probe( ifstream& input){
    uint32_t rd;
    input.read( reinterpret_cast<char*>( &rd), sizeof(rd));
    input.seekg( -1*sizeof(uint32_t), ios_base::cur );
    return rd;    
}



vector<uint32_t> SaberDecoder::Probe( ifstream& input, int n){
    uint32_t rd;
    vector<uint32_t> ret;

    for( int i=0; i<n; i++){
        input.read( reinterpret_cast<char*>( &rd), sizeof(rd));
        ret.push_back( rd );
    }

    input.seekg( -n*sizeof(uint32_t), ios_base::cur );

    return ret;    
}



int SaberDecoder::SetInputStream( ifstream* in ){

    input = in;

    if( !(*in) ){
        cerr << "Input stream corrupted.\n";
        input = 0;
        return -1;
    }

    // check the beginning of the file to see if it is in correct format.
    if( Probe( *input ) != 0xcc1234cc ){
        cerr << "Config parser not found. Wrong format." << endl;
        input = 0;
        return -2;
    }

    return 0;
}



int SaberDecoder::Decode( ifstream* in ){

    if ( SetInputStream( in )<0 )
        return -1;

    cout << "Decoding raw binary file..." << endl;

    // find the file size.
    input->seekg( 0, ios_base::end);
    bytes_total = input->tellg();
    input->seekg( 0, ios_base::beg);

    cout << "\tTotal " << bytes_total << " bytes" << endl;

    if( DecodeHeader( )<-1 ){
        cerr << "Failed to decode the global header.\n";
        return -1;
    }
    cout << "File header decoded." << endl;

    param_trig = GetTrigParameter();
    //cout << "Retrieved trigger parameter" << endl;
    
    param_adc = GetADCParameter();
    //cout << "Retrieved ADC parameter" << endl;

    // ******** Print file statistics

    bytes_per_event = 0;

    //cout << "\nacquisition info:\n";
    for( unsigned int i = 0; i<param_adc.size(); ++i){
        //cout << "\tboard"<<i<<endl;
        bytes_per_event += param_adc[i].GetTotalSizeInByte();
	    //cout << "\t\ttotal sample size (all channel): " << param_adc[i].GetEvtSizeInSamp() << endl;
	    //cout << "\t\tpre-trig sample size (per channel): " << param_adc[i].pre_trig_sample << endl;
	    //cout << "\t\tpost-trig sample size (per channel): " << param_adc[i].post_trig_sample << endl;
        //cout << "\t\tacquisition window: " << param_adc[i].GetEvtSizeInSamp()/param_adc[i].GetNChannelEnabled()*0.004 << " us\n";
    }

    #ifdef DBG
    cout << "\nfile info:\n";
    cout << dec << "    file size (bytes): " << bytes_total << endl;
    cout << "    begin/end header sizes (bytes): " << bytes_header_beg << " + " << bytes_header_end << endl;
    cout << "    bytes per event read from header: " << bytes_per_event << endl;
    #endif
    
    int nevt = (bytes_total - bytes_header_beg - bytes_header_end) / bytes_per_event;
    cout << "number of event : " << nevt << endl;

    if((bytes_total - bytes_header_beg - bytes_header_end) % bytes_per_event !=0 ){
        cerr << "warning: total size and size per event does not match\n";
    }
    //cout << "trigger rate    : " << 1.0*nevt/(end_time-begin_time) << " Hz\n";

    return 0;
}



int SaberDecoder::DecodeHeader( ){

    if( !(*input) || input==0 ){
        cerr << "input file is not in good status.\n";
        return -1;
    }

    // clear parameter files from previous function call
    config.Clear();
    param_trig.clear();
    param_adc.clear();

    uint32_t hdr = Probe( *input );
        // header of the binary file segment
    uint32_t size, version;
        // size of header and version of software used in writing

    if( hdr == 0xcc1234cc ){
        config.Deserialize( *input );
    }
    else{
        cerr << "Configuration header missing.\n";
        return -1;
    }
    //config.Print();

    hdr = Probe( *input );
    if( hdr!= 0xaa1234aa ){
        cerr << "ADC/Trigger header missing.\n";
        return -1;
    }

    hdr = Read( *input );
    size = Read( *input );

    if( size>2*sizeof(uint32_t) ){
        version = Read( *input );
        begin_time = Read( *input );

        //cout << "\tVersion " << version << endl;
        //cout << "\tDAQ began at " << begin_time << endl;
    }
    else{
        cerr << "global header missing version number and begin time\n";
        return -1;
    }

    if( version != 0 ){
        cerr << "header version is not supported by current software.\n";
        return -2;
    }

    // loop over the raw file and keep reading until global header is closed.
    // next time 0xaa1234aa is encountered is the closing header of event.
    hdr = Probe( *input );
    while( hdr!= 0xaa1234aa ){
    
        // trigger header
        if( hdr==0xae1234ae ){
            //cout << "Located trigger header." << endl;
            CAENV1495Parameter param;
            param.Deserialize( *input );
            param_trig.push_back( param );
        }

        // ADC header
        else if( hdr==0xad1234ad ){
            //cout << "Located ADC header." << endl;
            CAENV1720Parameter param;
            param.Deserialize( *input );
            //cout << param.GetPrintString();
            if( param.GetNChannelEnabled()>0 ){
                param_adc.push_back( param );
            }
        }
        else{
            cerr << "Unknown header 0x" << hex << hdr << endl;
            return -1;
        }

        hdr = Probe( *input );

    }

    if( hdr == 0xaa1234aa ){
    
        cout << "Global header closed." << endl;

        hdr = Read( *input );
        unsigned int size = Read( *input );

        cout << "Size of this header is " << size << endl;

        // read the rest of the global header
        // size in byte, 4 bytes is one word
        for( unsigned int i=0; i<size/sizeof(uint32_t)-2; i++ ){
            
            hdr = Read( *input );
            
            if( hdr== 0xee1234ee ){
                break;
            }
        }
    }


    cout << "Looking for event header..." << endl;

    if( hdr!= 0xee1234ee ){
        hdr = Probe( *input );
    }
    if( hdr!=0xee1234ee ){
        cerr << "Event header (0xee1234ee) is missing after global header (0xaa1234aa)" << endl;
        return -1;
    }

    hdr = Read( *input );
    size = Read( *input );

    cout << "\tSize of event header " << size << endl;

    // read off the remaining bytes in the event header
    for( unsigned int i=0; i<size/sizeof(uint32_t)-2; i++){
        Read( *input );
    }

    // locate beginning and ending of event data.
    // currently pointer is at beginning of event data.
    // next move to end of file and move backward.
    bytes_header_beg = input->tellg();

    input->seekg( -1*sizeof(hdr), ios_base::end );

    // if we successfully locate end of header indicator, then iterate and move back to locate the next header indicator.
    if( Probe( *input )==0xff1234ff ){

        input->seekg( -1*sizeof(hdr), ios_base::cur );

        while( Probe( *input )!=0xff1234ff ){
            input->seekg( -1*sizeof(hdr), ios_base::cur ); 
        }

        hdr = Read( *input );
        uint32_t size = Read( *input );
        bytes_header_end = 2*sizeof(uint32_t) + size;
            // global header plus 2 for event header
        uint32_t version = Read( *input );
        if( version==0 ){
            end_time = Read( *input );
        }
        return 0;
    }

    else{
        cerr << "Warning: data probably not properly closed.\n";
        end_time = 0;
        bytes_header_end = 0;
        return 1;
    }
}



std::vector<CAENV1495Parameter> SaberDecoder::GetTrigParameter(){
    return param_trig;
}




std::vector<CAENV1720Parameter> SaberDecoder::GetADCParameter(){
    return param_adc;
}




vector<SaberRawWaveform> SaberDecoder::GetEvent( uint64_t ID ){

    vector<SaberRawWaveform> event;

    // check if good for read.
    if( input==0 || !(*input).good() ){
        cerr << "input file is not in good status.\n";
        return event;
    }

    // check if given index is a valid one or not.
    if( ID>= GetEventNumber() ){
        cerr << "maximum event exceeded " << endl;
        return event;
    }
    
    // place cursor at the right position
    input->seekg( bytes_header_beg+ID*bytes_per_event, ios_base::beg);


    // Get the channel parameters ready
    for( unsigned int i=0; i<param_adc.size(); i++){
        for( int j=0; j<8; j++){
            if( ( param_adc[i].ch_enable_mask & (1<<j) ) != 0 ){
                SaberRawWaveform wfm;

                wfm.SetBoardID( i );
                wfm.SetChannelID( j );
                wfm.SetThreshold( param_adc[i].channel_param[j].threshold );
                wfm.SetTXThreshold( param_adc[i].channel_param[j].tcrossthresh );
                wfm.SetDAC( param_adc[i].channel_param[j].dac );
                wfm.SetDescriptor( param_adc[i].channel_param[j].descriptor );
                wfm.SetLabel( param_adc[i].channel_param[j].label);
                wfm.SetPreTrigSample( param_adc[i].pre_trig_sample );
                wfm.SetPostTrigSample( param_adc[i].post_trig_sample );

                wfm.reserve( param_adc[i].pre_trig_sample + param_adc[i].post_trig_sample );

                event.push_back( wfm );
            }
        }
    }


    // current waveform to fill, incremented after each waveform is filled
    unsigned int bd = 0;
    unsigned int ch = 0;

    uint64_t evtID(0);

    while( bd<param_adc.size() ){

        // total size of the event in one board
        uint32_t event_size = Read( *input );

        // if there is a misalign, search for next valid beginning
        while( (event_size & 0xf0000000) != 0xa0000000 ){
            cerr << "reading event "<< ID 
                 << ", not valid begin of event : 0x" << hex
                 << event_size << ", skipping " << endl;
            event_size = Read( *input );
//            return vector<SaberRawWaveform>();
        }

        // first word, event size
        event_size &= 0xfffffff;

        // check if channel mask is consistent
        uint32_t channel_mask = Read( *input );
        channel_mask &= 0x0ff;
        if( param_adc[bd].ch_enable_mask != channel_mask ){
            cerr << "channel enable mask inconsistent:"
                 << " from config file board" << dec << bd
                 << " has mask 0x" << hex << param_adc[bd].ch_enable_mask
                 << ", data has mask 0x" << channel_mask <<endl;
            return vector<SaberRawWaveform>();
        }

        // find out number of enabled channels
        int nchan_enabled = 0;
        for( int i=0; i<8; i++){
            if( ( (channel_mask)&(0x1<<i) ) != 0  )
                nchan_enabled++;
        }

        // numbver of 32-bit integer per channel
        int word_per_chan = (event_size - 4)/nchan_enabled;

        // event counter
        evtID = Read( *input );
        evtID &= 0xffffff;
        
        // trigger time tag
        uint32_t ttt = Read( *input );

        // iterate over enabled channels for this board
        for( int i=0; i<nchan_enabled; i++){
            
            // check if event size matches
            if( event[ch].GetPreTrigSample() + event[ch].GetPostTrigSample() != 2*word_per_chan ){
                cerr << "event ID " << dec << ID
                     << " size mismatch" << endl;
                cerr << "   |-pre and post trig samples are: "
                     << event[ch].GetPreTrigSample() << '\t'
                     << event[ch].GetPostTrigSample() << endl;
                cerr << "   |-from event header expects " 
                     << 2*word_per_chan << " samples per event\n\n";
                return vector<SaberRawWaveform>();
            }

            event[ch].SetTrigTimeTag( ttt );
            event[ch].SetEventID( evtID );

            // read out data
            uint32_t data = 0;
            for( int j=0; j<word_per_chan; j++){
                data = Read( *input );
                event[ch].push_back( data & 0xfff );
                event[ch].push_back( (data>>16) & 0xfff );
            }
            ch++;
        }
        bd++;
    }
    return event;
}


void SaberDecoder::WriteHDF5( H5FileManager* h5man){

    bool stat = h5man->IsFileOpen();
    if ( !stat ){
        cerr << "H5FileManager is not managing an open file.\n";
    }

    // Write the HDF5 global header
    //

    ConfigParser rawconfig = GetConfigParser();
    //rawconfig.Print();
    
    // Global header should include
    // Begin and end time stamp
    // Configuration
    // Comments
    // No of ADC boards
    // Trigger configuration
    // etc.
    //
    h5man->AddAttribute( "/", "version", string("1.0.0") );
    h5man->AddAttribute( "/", "comment", rawconfig.GetString("/cmdl/comment") );
    
    std::ostringstream ostr;
    rawconfig.Print( ostr );
    h5man->AddAttribute( "/", "config", ostr.str() );
    
    h5man->AddAttribute( "/", "timestamp", GetBeginTime() );
    h5man->AddAttribute( "/", "timestamp_end", GetEndTime() );
    
    vector<CAENV1720Parameter> adcparam = GetADCParameter();
    
    h5man->AddAttribute( "/", "nb_adc_board", adcparam.size() );

    cout << adcparam.size() << " ADC boards enabled.\n";

    for( unsigned int i=0; i<adcparam.size(); i++){
        adcparam[i].SetBoardIndex( i );
        adcparam[i].ExportHDF5( h5man );
    }
    
    vector<CAENV1495Parameter> trigparam = GetTrigParameter();

    if( trigparam.size()>0 ){
        trigparam[0].ExportHDF5( h5man );
    }


    // Iterate over events of each board
    //

    for( unsigned int i=0; i<adcparam.size(); i++){
        
        uint64_t ID = 0;
            // order of the event, start from 0 and increments by 1
        uint64_t eventID;
            // event ID, obtained from raw file
        
        unsigned int nchannel = adcparam[i].GetNChannelEnabled();
            // Nb of channels enabled.
            // This is just an alias to make things more concise.
        unsigned int nsample = adcparam[i].GetEvtSizeInSamp()/nchannel;
            // Nb of samples in each event.

        uint16_t* data = new uint16_t[ nchannel * nsample ];
        
        cout << "\tBoard " << i << " has " << adcparam[i].GetNChannelEnabled() << " channels enabled.\n";
        cout << "\tEach channel has " << nsample << " samples\n";

        uint32_t ttt;
            // trigger time tag


        // Add total number of events as an attribute
        //
        uint64_t n_total_event = GetEventNumber();

        stringstream ss;
        ss << "/adc_" << i;
        h5man->AddAttribute( ss.str(), "nb_events", n_total_event );

        int dataset_index = 0;
            // Data will be grouped into datasets, which contains max of max_evt_per_set events.
            // This index keeps track of which dataset a given event belongs to.
        unsigned int max_evt_per_set = 1000000;

        unsigned int evt_counter = 0;
            // Used to keep track of number of events within each dataset.

        auto start = std::chrono::system_clock::now();
        
        for( ID=0; ID<n_total_event; ID++){

            vector<SaberRawWaveform> event = GetEvent( ID );
            //cout << "Retrieved event " << ID << endl;
            //

            if( ID==0 ){
                if( event.size()!=nchannel ){
                    cerr << "Warning: event " << ID << " nb of raw waveform is different from nb of enabled channels!\n";
                    break;
                }
                else if( event.size()>0 ){
                    if( event[0].size()!=nsample )
                        cerr << "Warning: nb of events in raw waveform " << event[0].size() << " is different from that expected from C1720Parameter " << nsample << endl;
                }
            }

            ttt = event[0].GetTrigTimeTag();
            eventID = event[0].GetEventID();

            unsigned int dim[2] = { nchannel, nsample};
                // rank, or dimention of the output data
                // used in writing HDF5 data

            for( unsigned int m=0; m<nchannel; m++ ){
                for( unsigned int n=0; n<nsample; n++){
                    data[m*nsample+n] = event[m][n];
                }
            }


            stringstream dsetname;
                // Name of the dataset
                // Increments from 0
            stringstream evtname;
                // Name of individual event
            
            dsetname << "/adc_" << i << "/dataset_" << dataset_index;
            evtname << "/adc_" << i << "/dataset_" << dataset_index << "/event_" << evt_counter;

            // If event counter is zero, then this is the first event in the dataset.
            // Need to create the dataset group first.
            if( evt_counter==0 ){
                cout << "Opening group " << dsetname.str() << endl;
                h5man->OpenGroup( dsetname.str() );
            }

            h5man->WriteData( data, evtname.str(), H5::PredType::NATIVE_UINT16, 2, dim);
            evt_counter++;
                // Everytime an event is written, increment the counter to keep track of number of events.
                // This will be written into the HDF5 file later.

            h5man->AddAttribute( evtname.str(), "index", ID);
            h5man->AddAttribute( evtname.str(), "eventID", eventID);
            h5man->AddAttribute( evtname.str(), "trigger_time_tag", ttt);

            // If there are max_evt_per_set (1 million currently) events in the dataset, or for-loop reached its end
            // increment the dataset index and reset event counter to be 0.
            if( evt_counter % max_evt_per_set == 0 || ID ==n_total_event-1 ){
                h5man->AddAttribute( dsetname.str(), "nb_events", evt_counter);
                evt_counter = 0;
                dataset_index++;
            }
            
            if( ID%2000==0 && ID!=0 ){

                auto end = std::chrono::system_clock::now();
                std::chrono::duration<float> seconds = end-start;

                // cout.precision(4);
                // cout << "Processed event " << ID << ", " << 100.0*ID/n_total_event << "\% completed. Efficiency " << 1000./seconds.count() << " events/seconds" << endl;
                cout << "\33[2K\r";
                cout << ID << ",\t" << 100.0*ID/n_total_event << ",\t" << 2000./seconds.count() << std::flush;
                
                start = std::chrono::system_clock::now();
            }
        }
        cout << endl;
    
        delete [] data;
    }

}
