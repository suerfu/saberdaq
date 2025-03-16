#include "SaberHDF5Recorder.h"
#include "SaberDAQData.h"
#include "SaberDAQHeader.h"

#include <unistd.h>

extern "C" SaberHDF5Recorder* create_SaberHDF5Recorder( plrsController* c ){ return new SaberHDF5Recorder(c);}


extern "C" void destroy_SaberHDF5Recorder( SaberHDF5Recorder* p ){ delete p;}


SaberHDF5Recorder::SaberHDF5Recorder( plrsController* c) : plrsModuleRecorder(c){ next_addr = -1; }


SaberHDF5Recorder::~SaberHDF5Recorder(){}


int SaberHDF5Recorder::GetNextModuleID(){

    if( next_addr<0 ){

	    string next_module = GetConfigParser()->GetString("/module/"+GetModuleName()+"/next_module", "");
	        // if not found, returns default value of ""
	    if( next_module!="" ){
	        next_addr = ctrl->GetIDByName( next_module );   // nonnegative if valid
	    }
	    if( next_module=="" || next_addr<0 ){
	        Print("Next module not specified. Setting up loop-back.\n", INFO);
	        next_addr = ctrl->GetIDByName( this->GetModuleName() );
        }
    }

    return next_addr;
}

void SaberHDF5Recorder::Configure(){

    Print("Configuring...\n", DEBUG);

    // =========================================================
    // Initialize HDF5 manager. Data will be written in ConfigureOutput..
    // =========================================================
    
    h5man = new H5FileManager();
    Print("Initialized HDF5 file manager.\n", DEBUG);

    string filename = GetFileName();

    Print("Opening file "+filename+" for output...\n", INFO);
    if( h5man->OpenFile( filename, "w+")==false ){
        Print("Failed to create HDF5 file for output.\n", ERR);
        SetStatus(ERROR);
        return;
    }
    Print("File successfully opened for output.\n", DEBUG);


    // =========================================================
    // Get a template data object from DAQ module and use it to configure metadata.
    // =========================================================

    Print("Configuring HDF5 file metadata using empty data template...\n", DEBUG);
    void* rdo = 0;
    while( rdo==0 && GetState()==CONFIG ){
        rdo = PullFromBuffer();
    }

    // If state is no longer CONFIG due to error, return the memory and exit
    if( GetState()!=CONFIG ){
        if( rdo!=0 )
    	    PushToBuffer( GetNextModuleID(), rdo);
        return;
    }

    // Use the empty data template to configure the output meta data.
    ConfigureOutput( reinterpret_cast<SaberDAQData*>(rdo) );
    Print("HDF5 file metadata configured.\n", DEBUG);

    // After configuring the output, give the data to the next module.
    PushToBuffer( GetNextModuleID(), rdo);

    Print( this->GetModuleName()+" configured.\n", DEBUG);

}

void SaberHDF5Recorder::Deconfigure(){

    Print( this->GetModuleName() + " deconfiguring...\n", DEBUG);
    
    void* rdo = 0;
    
    while( GetState()!=ERROR ){
        
        rdo = PullFromBuffer();
        
        if( rdo!=0 ){
            
            SaberDAQData* d = reinterpret_cast<SaberDAQData*>(rdo);
            
            if ( d->IsHeader() ){
                Print("Closing header received.\n", DEBUG);
                CloseOutput( d );
                PushToBuffer( GetNextModuleID(), d);
                return;
            }
            else{
                Print("Processing events...\n", DEBUG);
                WriteToOutput( d );
                PushToBuffer( GetNextModuleID(), d);
                rdo = 0;
            }
        }
    }
    
    Print( this->GetModuleName() + " deconfigured.\n", DEBUG);
}



void SaberHDF5Recorder::PreRun(){
    evt_counter = 0;
}



void SaberHDF5Recorder::Run(){

    Print( "Running...\n", DEBUG);

    void* rdo = 0;

    int count, last_count;
    uint32_t now, past;
    count = last_count = 0;
    now = past = ctrl->GetTimeStamp();

    while( GetState()==RUN && GetStatus()==RUN ){

        rdo = PullFromBuffer( RUN );
            // if rdo is 0, RUN has finished
            // otherwise there is new event to be written to disk

        if( rdo!=0 ){

            SaberDAQData* d = reinterpret_cast<SaberDAQData*>(rdo);

            if( !d->IsHeader() ){
                WriteToOutput( d );
                PushToBuffer( GetNextModuleID(), rdo);
                count++;
            }
            // if it is a header, then run is finished
            // put it in recorder FIFO buffer for reprocessing in deconfig phase
            else{
                Print( "Closing header received during Run.\n", INFO);
                PushToBuffer( ctrl->GetIDByName( this->GetModuleName()), rdo);
                break;
            }
            rdo = 0;
        }
        else{
            break;
        }

        now = ctrl->GetTimeStamp();
        if( now-past>30 ){
            stringstream ss;
            int delta = count - last_count;
            ss << "In the past 30 sec : " << delta << " events\n";
            ss << "Trigger rate : " << delta/30. << " Hz\n";
            Print( ss.str(), INFO);
            last_count = count;
            past = now;
        }
    }

    Print( "Run ended.\n", DEBUG);
}


void SaberHDF5Recorder::PostRun(){
}


string SaberHDF5Recorder::GetFileName(){
    
    if( GetConfigParser()->Find("/cmdl/output") ){
        return GetConfigParser()->GetString("/cmdl/output");
    }

    string file_prefix = GetConfigParser()->GetString( "/cmdl/prefix", "Default");

    stringstream ss;
    ss << file_prefix;

    time_t t = ctrl->GetTimeStamp();
    struct tm tm = *localtime( &t );

    ss << "_" << tm.tm_year+1900;
    ss << setfill('0') << setw(2) << tm.tm_mon+1 << setfill('0') << setw(2) << tm.tm_mday;
        // Date
    ss << "_" << setfill('0') << setw(2) << tm.tm_hour;
    ss << setfill('0') << setw(2) << tm.tm_min;
    ss << setfill('0') << setw(2) << tm.tm_sec;
        // Time
    ss << ".hdf5";
            // File extension.
    return ss.str();
}


void SaberHDF5Recorder::CloseOutput( SaberDAQData* data ){

    Print( "Closing output HDF5 file...\n", INFO );
    
    h5man->AddAttribute( "/", "timestamp_end", data->GetTimeStamp() );
   
    for( unsigned int i=0; i<nb_adc_board; i++){
        stringstream ss;
        ss << "/adc_" << i;
        h5man->AddAttribute( ss.str(), "nb_events", evt_counter );
    
        stringstream ss2;
        ss2 << evt_counter << " events recorded.\n";
        Print( ss2.str(), INFO );
    }

    h5man->CloseFile();
}


void SaberHDF5Recorder::ConfigureOutput( SaberDAQData* data ){

    bool stat = h5man->IsFileOpen();
    if ( !stat ){
        Print( "HDF5 file is not open. Nothing will be written", INFO);
        return;
    }
    
    ConfigParser* rawconfig = GetConfigParser();
    
    // ******************************
    //          Global header 
    // ******************************
    
    h5man->AddAttribute( "/", "version", string("2.0.0") );
    h5man->AddAttribute( "/", "comment", rawconfig->GetString("/cmdl/comment") );
    h5man->AddAttribute<string>( "/", "config", rawconfig->GetConfigFileTxt() );

    h5man->AddAttribute( "/", "timestamp", data->GetTimeStamp() );
    

    // ******************************
    //              ADC
    // ******************************

    vector<CAENV1720Parameter> adcparam = data->GetADCParameters();
    
    nb_adc_board = adcparam.size() ;

    h5man->AddAttribute( "/", "nb_adc_board", (uint32_t) adcparam.size() );

    for( unsigned int i=0; i<adcparam.size(); i++){
        adcparam[i].SetBoardIndex( i );
        adcparam[i].ExportHDF5( h5man );
    }
    

    // ******************************
    //          Trigger
    // ******************************

    CAENV1495Parameter trigparam = data->GetTriggerParameter();
    trigparam.ExportHDF5( h5man );

}


void SaberHDF5Recorder::WriteToOutput( SaberDAQData* data){

    for( unsigned int i=0; i < data->size(); i++){

        // first find out the dimension of the data matrix
        //
        unsigned int nchannel = (*data)[i].GetNChannelEnabled();
            // Nb of channels enabled.
            // This is just an alias to make things more concise.
        unsigned int nsample = (*data)[i].samp_per_chan();
            // Nb of samples in each event.

        unsigned int dim[2] = { nchannel, nsample};
            // rank, or dimention of the output data
            // used in writing HDF5 data

        // get the corresponding names
        //
        stringstream dsetname;
            // Name of the dataset
            // Increments from 0
        stringstream evtname;
            // Name of individual event
    
        dsetname << "/adc_" << i;
        evtname << "/adc_" << i << "/event_" << evt_counter;

        // If event counter is zero, then this is the first event in the dataset, create the dataset group first.
        //
        if( evt_counter==0 ){
            Print(  string("Opening group ") + dsetname.str() + string("\n"), INFO);
            h5man->OpenGroup( dsetname.str() );
        }

        // Write the actual waveform to data, noting that the first 4 32-bit words are headers
        //
        h5man->WriteData( (*data)[i].GetBufferAddr()+4, evtname.str(), H5::PredType::NATIVE_UINT16, 2, dim);
        
        evt_counter++;
            // Everytime an event is written, increment the counter to keep track of number of events.
            // This will be written into the HDF5 file later.

        // write other attributes
        //
        uint32_t ttt = (*data)[i].GetTimeTag();
        uint32_t timestamp = data->GetTimeStamp();
        uint64_t index = evt_counter;
        uint64_t eventID = (*data)[i].GetEventID();
        
        h5man->AddAttribute( evtname.str(), "index", index);
        h5man->AddAttribute( evtname.str(), "counter", eventID);
        h5man->AddAttribute( evtname.str(), "trigger_time_tag", ttt);
        h5man->AddAttribute( evtname.str(), "timestamp", timestamp);
    }

}
