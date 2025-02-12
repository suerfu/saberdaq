#include "SaberEventFilter.h"
#include "SaberDAQData.h"
#include "SaberDAQHeader.h"


extern "C" SaberEventFilter* create_SaberEventFilter( plrsController* c ){ return new SaberEventFilter(c);}


extern "C" void destroy_SaberEventFilter( SaberEventFilter* p ){ delete p;}


SaberEventFilter::SaberEventFilter( plrsController* c) : plrsStateMachine(c){
    addr_prev = -1;
    nsig = 0;
    min_dev = 7;
}


SaberEventFilter::~SaberEventFilter(){}



void SaberEventFilter::Configure(){

    bool fd = false;
    max_event = cparser->GetInt( "/cmdl/fevent", &fd);
    if( !fd )
        max_event = 0xffffffff;
    stringstream ss;    ss << "run will stop after filtered event reaches " << max_event << endl;
    Print( ss.str(), INFO);

    string self = GetModuleName();
    string nxt = "";
    nxt = cparser->GetString( "/module/"+self+"/next_module" );

    // next destination in data flow is explicitly specified.
    if( nxt!="" ){
        addr_nxt = ctrl->GetIDByName( nxt );
        if( addr_nxt>=0 ){
            stringstream ss;
            ss << this->GetModuleName() + " configured to send data to " << ctrl->GetNameByID(addr_nxt) << "\n";
            Print( ss.str(), DETAIL);
        }
        else{
            Print( "invalid next_module. "+nxt+" not registered.\n", ERR);
        }
    }
    if( nxt=="" ){
        Print( "next address not specified - setting up data loopback.\n", ERR);
        addr_nxt = ctrl->GetIDByName( this->GetModuleName());
    }

    nxt = cparser->GetString( "/module/"+self+"/prev_module" );
    // next destination in data flow is explicitly specified.
    if( nxt!="" ){
        addr_prev = ctrl->GetIDByName( nxt );
        if( addr_prev>=0 ){
            stringstream ss;
            ss << this->GetModuleName() + " configured to reject data to " << ctrl->GetNameByID(addr_nxt) << "\n";
            Print( ss.str(), DETAIL);
        }
        else{
            Print( "invalid next_module. "+nxt+" not registered.\n", ERR);
        }
    }
    if( nxt=="" ){
        Print( "next address not specified - setting up data loopback.\n", ERR);
        addr_prev = ctrl->GetIDByName( this->GetModuleName());
    }

    bool found = false;
    int a = cparser->GetInt( "/module/"+self+"/nsig", &found );
    if( found )
        nsig = a;
    
    found = false;
    a = cparser->GetInt( "/module/"+self+"/min_dev", &found );
    if( found )
        min_dev = a;
}


void SaberEventFilter::Deconfigure(){

    void* rdo = PullFromBuffer();
    while( rdo!=0 && GetState()!=ERROR ){
        rdo = PullFromBuffer();
        if( rdo!=0 ){
            SaberDAQData* d = reinterpret_cast<SaberDAQData*>(rdo);
            if( Pass(d) )
                PushToBuffer( addr_nxt, d);
            else
                PushToBuffer( addr_prev, d);
            rdo = 0;
        }
    }
}



void SaberEventFilter::PreRun(){
    evt_counter = 0;
}


bool SaberEventFilter::Pass( SaberDAQData* data){
    if( data->IsHeader())
        return true;
    else{
        if( data->GetNSignal( min_dev ) >= nsig ){
            evt_counter++;
            //stringstream ss;    ss<<"event counter - " << evt_counter << endl;
            //Print( ss.str(), INFO);
            return true;
        }
    }
    return false;
}


void SaberEventFilter::Run(){

    Print( "running...\n", DEBUG);

    void* rdo = 0;

    while( GetState()==RUN && GetStatus()==RUN ){

        rdo = PullFromBuffer( RUN );
            // if rdo is 0, RUN has finished

        if( rdo!=0 ){
            SaberDAQData* d = reinterpret_cast<SaberDAQData*>(rdo);
            if( Pass(d) )
                PushToBuffer( addr_nxt, d);
            else
                PushToBuffer( addr_prev, d);
            rdo = 0;
        }
        else{
            break;
        }

        if( evt_counter>=max_event){
            Print("maximum filtered event reached\n", INFO);
            PushCommand( 0, "max-evt" );
        }


        sched_yield();
    }

    Print( "run ended.\n", DEBUG);
}

void SaberEventFilter::PostRun(){

    bool exit_ok = false;
    void* rdo = 0;

    while( rdo==0 && GetState()!=ERROR ){

        rdo = PullFromBuffer( );

		if( rdo!=0 ){
			SaberDAQData* d = reinterpret_cast<SaberDAQData*>(rdo);
			if( d->IsHeader() ){
				SaberDAQHeader* h = reinterpret_cast<SaberDAQHeader*>(rdo);
				if( h->GetHeader()==0xff1234ff){
                    exit_ok = true;
				}
			}
            if( Pass(d) )
                PushToBuffer( addr_nxt, d);
            else
                PushToBuffer( addr_prev, d);

			rdo = 0;
		}

        if( exit_ok )
            break;

        sched_yield();
	}
    Print( "run ended.\n", DEBUG);
}
