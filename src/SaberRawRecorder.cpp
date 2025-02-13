#include "SaberRawRecorder.h"
#include "SaberDAQData.h"
#include "SaberDAQHeader.h"


extern "C" SaberRawRecorder* create_SaberRawRecorder( plrsController* c ){ return new SaberRawRecorder(c);}


extern "C" void destroy_SaberRawRecorder( SaberRawRecorder* p ){ delete p;}


SaberRawRecorder::SaberRawRecorder( plrsController* c) : plrsModuleRecorder(c){}


SaberRawRecorder::~SaberRawRecorder(){}


void SaberRawRecorder::Configure(){
    

    // there should be an empty template data object pushed from DAQ object.
    
    void* rdo = 0;
    
    while( rdo==0 && GetState()==CONFIG ){
        
        rdo = PullFromBuffer();

        // If state is no longer CONFIG due to error, return the memory and exit
        if( GetState()!=CONFIG ){
            if( rdo!=0 )
    	        PushToBuffer( next_addr, rdo);
            return;
        }
    }

    
    // first write the configuration information
    //
    cparser->Serialize( output_file );
    
    // Use the empty data template to configure the output meta data.
    //
    SaberDAQData* data = reinterpret_cast<SaberDAQdata*>(rdo);

    // compute total header size
    //
    CAENV1495Parameter trigger_param = data->GetTriggerParameter();
    
    vector<CAENV1720Parameter> adc_params = data->GetADCParameters();

    unsigned int header_size = 0;
    
    header_size += 4*trigger_param.GetHeaderSize();
    
    for( unsigned int i=0; i<adc_params.size(); i++){
        header_size += 4*adc_params[i].GetHeaderSize();
    }

    // Write Global header

    uint32_t glb_header[4];
    glb_header[0] = 0xaa1234aa;
    glb_header[1] = 4*sizeof( glb_header[0] ) + header_size;
    glb_header[2] = 0;
    glb_header[3] = data->GetTimeStamp();
    output_file.write( glb_header, 4*sizeof(glb_header[0]) );


    // Write trigger parameter

    unsigned int len = 4*trigger_param.GetHeaderSize(); 
    char* p_trigger = new char[ 4*trigger_param.GetHeaderSize() ];
    trigger_param.Serialize( p_trigger );
    output_file.write( p_trigger, len );
    delete p_trigger


    // Write ADC board parameters

    for( unsigned int i=0; i<adc_params.size(); i++){
        char* p_adc = new char[ 4*adc_params[i].GetHeaderSize() ];
        adc_params[i].Serialize( p_adc );
        output_file.write( p_adc);
        delete p_adc;
    }

    // Write Global closing header 
    uint32_t glb_header_cls[2];
    glb_header_cls[0] = 0xaa1234aa;
    glb_header_cls[1] = 2*sizeof( glb_header_cls[0]);
    output_file.write( glb_header_cls, 2*sizeof( glb_header_cls[0]));
 

    PushToBuffer( next_addr, rdo);
    
    Print( this->GetModuleName()+" configured.\n", DEBUG);

}


void SaberRawRecorder::Deconfigure(){

    if(!output_file || !output_file.is_open() )
        return;

    void* rdo = PullFromBuffer();

    while( rdo!=0 && GetState()!=ERROR ){

        rdo = PullFromBuffer();

        if( rdo!=0 ){
            SaberDAQData* d = reinterpret_cast<SaberDAQData*>(rdo);
            d->Write( output_file);
            PushToBuffer( addr_nxt, d);
            rdo = 0;
            sleep(1);
        }
    }
    
    // all events in FIFO buffer has been depleted
    // write event closure header and global closure header

}


// PreRun()
// mainly write event header to mark the begin of waveforms
//
void SaberRawRecorder::PreRun(){
    
    // *** initial header
    
    uint32_t evt_header[4];
    evt_header[0] = 0xee1234ee;
    evt_header[1] = 4*sizeof(evt_header[0]);
    evt_header[2] = 0;
    evt_header[3] = 0;

    // calculate bytes per event:
    for( unsigned int i=0; i<v1720.size(); ++i){
        evt_header[3] += v1720[i].GetTotalSizeInByte();
    }

    output_file.write( evt_header, 4*sizeof( evt_header[0] ) );
    
}



// Run()
// Continuously pulls from buffer and write output to file.
// Output is written by calling SaberDAQData's member method Write
//
void SaberRawRecorder::Run(){

    Print( "running...\n", DEBUG);

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
            if( output_file ){
                SaberDAQData* d = reinterpret_cast<SaberDAQData*>( rdo );
                d->Write( output_file );
            }
            PushToBuffer( addr_nxt, rdo );
            rdo = 0;
            
            count++;
        }
        else{
            break;
        }

        // calculate the average trigger rate in the past 30 seconds.
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

    Print( "run ended.\n", DEBUG);
}


// PostRun()
// In the PostRun phase, pull from the buffer pool to make sure everything has been written to disk.
// The end is marked by receiving a header type data.
//
void SaberRawRecorder::PostRun(){

    bool exit_ok = false;
    
    void* rdo = 0;

    while( rdo==0 && GetState()!=ERROR ){

        rdo = PullFromBuffer( );

		if( rdo!=0 ){

			SaberDAQData* d = reinterpret_cast<SaberDAQData*>(rdo);
			
			if( d->IsHeader() ){
                exit_ok = true;
                break;
			}
            else{
				d->Write( output_file);
            }

			PushToBuffer( addr_nxt, rdo);
			rdo = 0;
		}

        sched_yield();
	}

    // ****** close event header
    
    uint32_t evt_header[2];
    evt_header[0] = 0xee1234ee;
    evt_header[1] = sizeof(evt_header[0])*2;
    output_file.write( evt_header, 2*sizeof(evt_header[0]) );


    // ****** close global header

    uint32_t glb_header[5];
    glb_header[0] = 0xff1234ff;
    glb_header[1] = 5*sizeof( glb_header[0] );
    glb_header[2] = 0;
    glb_header[3] = rdo->GetTimeStamp();
    glb_header[4] = 0xff1234ff;
    output_file.write( glb_header, 5*sizeof( glb_header[0] ) );
    
    PushToBuffer( addr_nxt, rdo);
 
    Print( "Run ended.\n", DEBUG);

}
