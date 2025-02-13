#include "TestDAQ.h"
#include "SaberDAQData.h"
#include "SaberDAQHeader.h"

#include <algorithm>
#include <sstream>
#include <unistd.h>

using namespace std;


extern "C" TestDAQ* create_TestDAQ( plrsController* c ){ return new TestDAQ(c);}
    //!< create method to return the DAQ object.

extern "C" void destroy_TestDAQ( TestDAQ* p ){ delete p;}
    //!< destroy method.


TestDAQ::TestDAQ( plrsController* ctrl) : plrsModuleDAQ( ctrl){
    ext_trig_to_start = false;
    rand_trig = false;
    rand_trig_via_fpga = false;

    event_counter = 0;
    total_event_number = 0;

    ftest[0] = ftest[1] = Scint;
    ftest[2] = ftest[3] = Scint; //Gauss;
    ftest[4] = ftest[5] = TriPulse;
    ftest[6] = ftest[7] = RecPulse;

}


TestDAQ::~TestDAQ(){

    void* rdo = PullFromBuffer();
    while( rdo!=0 ){
        delete reinterpret_cast<SaberDAQData*>(rdo);
        rdo = PullFromBuffer();
    }

    Print( "TestDAQ deleted\n", DETAIL);

}


// Establish connection with the VME crate.
void TestDAQ::Initialize(){

    Print( "TestDAQ initializing...\n", DETAIL);

    bool found = false;;

    // get all connection types (USB, optical link, daisy chain).
    Print( "Checking VME connections.\n", DETAIL);

    map< string, vector<string> > connections = cparser->GetListOfParameters( "/module/daq/connection" );
    map< string, vector<string> >::iterator con_itr;
    for( con_itr = connections.begin(); con_itr!=connections.end(); ++con_itr){

        string key = con_itr->first;
        string vme_type = cparser->GetString( key+"type" );

        if( vme_type=="vme_optical_link" || vme_type=="vme_usb" ){

            // check if this is the first instance
            if( vme_connection.find(key)==vme_connection.end() ){

                VMEConnection value;
                value.type = vme_type;

                value.link_number = cparser->GetInt( key+"link_number", &found );
                if( !found ){
                    Print( "Cannot find link_number for " +key +vme_type +"\n", ERR);
                    SetStatus( ERROR );
                    return;
                }

                value.board_number = cparser->GetInt( key+"board_number", &found );
                if( !found ){
                    Print( "Cannot find board_number for " +key +vme_type +"\n", ERR);
                    SetStatus( ERROR );
                    return;
                }

                value.handle = -1;
                    // Connection is not established yet. Set it to -1.
                vme_connection[key] = value;
            }
        }
    }

    std::map<string, VMEConnection>::iterator itr;
    std::map<string, VMEConnection>::const_iterator citr;


    // **************************************************************
    //                          trigger
    // **************************************************************

    Print( "Checking connect options for trigger.\n", DETAIL);

    bool trig_en = cparser->GetBool("/module/daq/logic_trigger/enable", &found );
    if( trig_en && found ){

        string type = cparser->GetString("/module/daq/logic_trigger/connect_via");
        citr = vme_connection.find( "/module/daq/"+type+"/" );

        if( type=="" || citr==vme_connection.end() ){
            Print( "Trigger connection method unspecified.\n", ERR);
            SetStatus( ERROR );
            return ;
        }
    }

    total_event_number = cparser->GetInt( "/cmdl/event", &found );
    if( !found ){
        total_event_number = cparser->GetInt( "/cmdl/e", &found );
        if( !found ){
            total_event_number = 0xffffffff;
        }
    }

    // ****************************************************************
    //                          ADC
    // ****************************************************************
    
    Print( "Checking connect options for ADC.\n", DETAIL);

    map< string, vector<string> > adcs = cparser->GetListOfParameters( "/module/daq/board" );
    map< string, vector<string> >::iterator adc_itr;

    for( adc_itr = adcs.begin(); adc_itr!=adcs.end(); ++adc_itr){

        string dirname = adc_itr->first;

        // if board is enabled in the config file, create a parameter object and see the details such as enabled channels.
        bool adc_enabled = cparser->GetBool( dirname+"enable", false );

        if( adc_enabled ){

            string type = cparser->GetString( dirname+"connect_via" );

            if( type=="" ){
                Print( "No connect option specified for " + dirname + "\n", ERR);
                SetStatus( ERROR );
                return;
            }

            else if( type=="direct"){
                VMEConnection value;
                value.type = "/direct"+dirname;

                value.link_number = cparser->GetInt( dirname+"link_number", &found );
                if( !found ){
                    Print( "Cannot find link_number for " + dirname + "\n", ERR);
                    SetStatus( ERROR );
                    return;
                }

                value.board_number = cparser->GetInt( dirname+"board_number", &found );
                if( !found ){
                    Print( "Cannot find board_number for " + dirname + "\n", ERR);
                    SetStatus( ERROR );
                    return;
                }

                value.handle = -1;
                vme_connection[type] = value;
            }

            else{
                citr = vme_connection.find( "/module/daq/"+type+"/" );

                if( citr==vme_connection.end() ){
                    Print( type + " not found in connections.\n", ERR);
                    SetStatus( ERROR );
                    return;
                }
            }
        }
    }

    Print( "Initializing all connections.\n", DETAIL);

    for( map< string, VMEConnection >::iterator i=vme_connection.begin(); i!=vme_connection.end(); ++i){
        Print( "Initializing " + (i->first) + " with " + (i->second).type + "\n", INFO );
        Print( "Connection established.\n", DETAIL);
    }
    
}


void TestDAQ::Configure(){

    Print( "Configuring TestDAQ.\n", DETAIL);

    bool found = false;;
    std::map<string, VMEConnection>::const_iterator citr;


    vector<SaberDAQHeader*> header_to_send;


    // **************************************************************
    //                          trigger
    // **************************************************************

    bool trig_en = cparser->GetBool("/module/daq/logic_trigger/enable", &found );
    if( trig_en && found ){

        CAENV1495Parameter param;
        param.SetParamFromConfig( cparser );

        string type = cparser->GetString("/module/daq/logic_trigger/connect_via");
        citr = vme_connection.find( "/module/daq/"+type+"/" );

        param_trig.push_back(param);
    
        // ** send configuration information to disk recorder.
        char* p = new char[ 4*param.GetHeaderSize() ];
        param.Serialize( p );

        SaberDAQHeader* hdr = new SaberDAQHeader();
        hdr->CopyHeader( p, 4*param.GetHeaderSize() );
        header_to_send.push_back( hdr );
        delete p;

        Print( "Trigger configured.\n", DETAIL);
    }


    // ***************************************************************
    //                      Random trigger
    // ***************************************************************
    rand_trig = cparser->GetBool("/module/daq/periodic_trigger/enable", &found );

    if( rand_trig && found ){

        if( cparser->GetString("/module/daq/periodic_trigger/source" )=="fpga" ){
            rand_trig_via_fpga = true;
        }
        else
            rand_trig_via_fpga = false;

         rand_trig_period = cparser->GetInt("/module/daq/periodic_trigger/rate", &found );

        if( !found ){
            Print("Cannot find sampling rate. Using default value (1 Hz).\n", ERR);
            rand_trig_period = 1000;
        }

        //control->SetSamplingPeriod( rate );
        Print( "Periodic sampling enabled and configured.\n", DETAIL);
    }
        

    // ****************************************************************
    //                      ADC
    // ****************************************************************

    map< string, vector<string> > adcs = cparser->GetListOfParameters( "/module/daq/board" );
    map< string, vector<string> >::iterator adc_itr;

    for( adc_itr = adcs.begin(); adc_itr!=adcs.end(); ++adc_itr){

        string dirname = adc_itr->first;
        if(dirname=="/module/daq/board*/")
            continue;


        CAENV1720Parameter param;
        param.SetParamFromConfig( cparser, dirname);

        bool adc_enabled = cparser->GetBool( dirname+"enable", &found );

        if( adc_enabled && found ){

            if( param.ch_enable_mask>0x0 ){

                string type = cparser->GetString( dirname+"connect_via" );
                citr = vme_connection.find( "/module/daq/"+type+"/" );

                param_adc.push_back( param );
            }

            if( param.runmode==FIRST_TRIG_CON )
                ext_trig_to_start = true;

            char* p = new char[ 4*param.GetHeaderSize() ];
            param.Serialize( p );

            // add the board only if it is enabled
            SaberDAQHeader* hdr = new SaberDAQHeader();
            hdr->CopyHeader( p, 4*param.GetHeaderSize() );
            header_to_send.push_back( hdr );

            delete p;
        }
    }


    if( param_adc.size()==0 && param_trig.size()==0 )
        Print( "No digitizer or trigger is enabled in the configuration file.\n", INFO);
    else
        Print( "ADC configured.\n", DETAIL);

    for( int i=0; i<NBUFF; i++){
        int id = ctrl->GetIDByName( this->GetModuleName() );
        PushToBuffer( id, new SaberDAQData( param_adci, param_trig[0]) );
    }

    Print( "Data buffer configured.\n", DETAIL);


    // **************************************************************
    //               Config global header and send all
    // **************************************************************


    // global header begin

    uint32_t glb_header[4];
    glb_header[0] = 0xaa1234aa;

    glb_header[1] = 4*sizeof( glb_header[0] );
    for( unsigned int i=0; i<header_to_send.size(); ++i)
        glb_header[1] += header_to_send[i]->size();

    glb_header[2] = 0;
    glb_header[3] = ctrl->GetTimeStamp();

    SaberDAQHeader* glb = new SaberDAQHeader();
    glb->CopyHeader( glb_header, 4*sizeof( glb_header[0] ) );
    PushToBuffer( addr_nxt, glb);


    // send ADC and Trigger header

    for( unsigned int i=0; i<header_to_send.size(); ++i)
        PushToBuffer( addr_nxt, header_to_send[i]);


    uint32_t glb_header_cls[2];
    glb_header_cls[0] = 0xaa1234aa;
    glb_header_cls[1] = 2*sizeof( glb_header_cls[0]);

    SaberDAQHeader* glb_cls = new SaberDAQHeader();
    glb_cls->CopyHeader( glb_header_cls, 2*sizeof( glb_header_cls[0] ) );
    PushToBuffer( addr_nxt, glb_cls );

}



void TestDAQ::UnConfigure(){

    DAQSTATE st = GetStatus();
    if( st!=CONFIG )
        StopDAQ();

    param_trig.clear();
    param_adc.clear();
}



void TestDAQ::CleanUp(){

    Print( "TestDAQ cleaning up.\n", DETAIL);

    DAQSTATE st = GetStatus();
    if( st!=INIT && st!=CONFIG )
        StopDAQ();

    std::map< string, VMEConnection>::iterator itr;
    for( itr=vme_connection.begin(); itr!=vme_connection.end(); ++itr){
        if( (itr->second).handle>=0 ){
            itr->second.handle = -1;
        }
    }

    vme_connection.clear();
}



void TestDAQ::StartDAQ(){
    Print( "TestDAQ starting...\n", INFO);
    trig_time_prev = std::chrono::steady_clock::now();
}



void TestDAQ::StopDAQ(){
    Print( "TestDAQ stopping...\n", INFO);
}



void TestDAQ::Event(){

    if( GetState()!=RUN || event_counter>=total_event_number )
        return;

    // At this point, there is at least 1 event in each board.

    void* vptr = 0;
    SaberDAQData* rdo = 0;

    while( vptr==0 ){
        vptr = PullFromBuffer();
        if( vptr!=0 ){   // could be header instead of data
            rdo  = reinterpret_cast<SaberDAQData*>( vptr );
            if( rdo->IsHeader() ){
            delete rdo;
            vptr = 0;
            rdo = 0;
            }
        }
        else
            sched_yield();
    }

    // valid rdo
    
    rdo  = reinterpret_cast<SaberDAQData*>( vptr );

    for( unsigned int bd=0; bd<param_adc.size(); ++bd){
        ReadFIFO( param_adc[bd], (*rdo)[bd].GetBufferAddr(), (*rdo)[bd].bytes() );
    }
    PushToBuffer( addr_nxt, rdo);

    rdo = 0;
    vptr = 0;

    ++event_counter;
}


void TestDAQ::PreEvent(){
}


void TestDAQ::PostEvent(){
    if( event_counter == total_event_number){
        PushCommand(0, "max-evt");
        Print( "Maximum event number reached.\n", INFO);
        return;
    }
}


bool TestDAQ::UpdateTimeSinceLastTrigger(){

    if( !rand_trig )
        return false;

    trig_time_cur = std::chrono::steady_clock::now();

    float diff = std::chrono::duration_cast< std::chrono::microseconds > ( trig_time_cur - trig_time_prev).count();

    if( diff > rand_trig_period ){
        trig_time_prev = trig_time_cur;
        return true;
    }

    return false;
}



void TestDAQ::PreRun(){

    // *** initial header

    uint32_t evt_header[4];
    evt_header[0] = 0xee1234ee;
    evt_header[1] = 4*sizeof(evt_header[0]);
    evt_header[2] = 0;
    evt_header[3] = 0;

    // calculate bytes per event:
    for( unsigned int i=0; i<param_adc.size(); ++i){
        evt_header[3] += param_adc[i].GetTotalSizeInByte();
    }

    SaberDAQHeader* evt = new SaberDAQHeader();
    evt->CopyHeader( evt_header, 4*sizeof( evt_header[0] ) );
    PushToBuffer( addr_nxt, evt);

    StartDAQ();
}



void TestDAQ::PostRun(){

    StopDAQ();

    // ****** close event header

    uint32_t evt_header[2];
    evt_header[0] = 0xee1234ee;
    evt_header[1] = sizeof(evt_header[0])*2;

    SaberDAQHeader* evt = new SaberDAQHeader();
    evt->CopyHeader( evt_header, 2*sizeof( evt_header[0] ) );
    PushToBuffer( addr_nxt, evt);

    // ****** clode global header

    uint32_t glb_header[5];
    glb_header[0] = 0xff1234ff;
    glb_header[1] = 5*sizeof( glb_header[0] );
    glb_header[2] = 0;
    glb_header[3] = ctrl->GetTimeStamp();
    glb_header[4] = 0xff1234ff;

    SaberDAQHeader* glb = new SaberDAQHeader();
    glb->CopyHeader( glb_header, 5*sizeof( glb_header[0] ) );
    PushToBuffer( addr_nxt, glb);
}


void TestDAQ::ReadFIFO( CAENV1720Parameter param, uint32_t* bytes, uint32_t bytes_to_read){
    bytes[0] = 0xa0000000+bytes_to_read/4;
    bytes[1] = param.ch_enable_mask;

    int n = param.GetNChannelEnabled();
    int spc = param.GetEvtSizeInSamp() / n;
        // sample per channel
    float r = 0.004*spc;

    int index = 0;
    for( int ch=0; ch<8; ch++){
        if( ( (0x1<<ch)&param.ch_enable_mask ) !=0  ){
            for( int id=0; id<spc/2; id++){
                bytes[ 4+index*spc/2+id ] = ftest[ch]( 2*id*0.004, r) + (ftest[ch]( (2*id+1)*0.004, r) << 16);
            }
            index++;
        }
    }
}



int TriPulse( float x, float r){
    float del = r/40;
    if( x<=r/5 )
        return 3969 + rand()%2;
    else if( x>r/5 && x<=r/5+del )
        return 3969 - 400*(x - r/5)/del + rand()%2;
    else
        return 3969 - 400*exp( -(x-r/5-del)/(r/40)) + rand()%2;
}


int RecPulse( float x, float r){
    if( x>2*r/5 && x<3*r/5 )
        return 95+rand()%2;
    else
        return 0 + rand()%2;
}


int Gauss( float x, float r){
    int p = 3965 - rand()%2;
    if( rand()%500<10 )
        p -= 50;
    return p;
}



int Scint( float x, float r){
    int p = TriPulse( x, r);
    if( rand()%1000<10 )
    //if( x>5.01&&x<5.02 )
        p -= 50;
    return p;
}

