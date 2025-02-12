#include "SaberDiskRecorder.h"
#include "SaberDAQData.h"
#include "SaberDAQHeader.h"


extern "C" SaberDiskRecorder* create_SaberDiskRecorder( plrsController* c ){ return new SaberDiskRecorder(c);}


extern "C" void destroy_SaberDiskRecorder( SaberDiskRecorder* p ){ delete p;}


SaberDiskRecorder::SaberDiskRecorder( plrsController* c) : plrsModuleRecorder(c){}


SaberDiskRecorder::~SaberDiskRecorder(){}



void SaberDiskRecorder::Deconfigure(){

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
        }
    }
}



void SaberDiskRecorder::PreRun(){
    cparser->Serialize( output_file );
}



void SaberDiskRecorder::Run(){

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
                SaberDAQData* d = reinterpret_cast<SaberDAQData*>(rdo);
                d->Write( output_file);
            }
            PushToBuffer( addr_nxt, rdo);
            rdo = 0;
            
            count++;
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

    Print( "run ended.\n", DEBUG);
}

void SaberDiskRecorder::PostRun(){

    bool exit_ok = false;
    void* rdo = 0;

    while( rdo==0 && GetState()!=ERROR ){

        rdo = PullFromBuffer( );

		if( rdo!=0 ){
			SaberDAQData* d = reinterpret_cast<SaberDAQData*>(rdo);
			if( output_file )
				d->Write( output_file);

			if( d->IsHeader() ){
				SaberDAQHeader* h = reinterpret_cast<SaberDAQHeader*>(rdo);
				if( h->GetHeader()==0xff1234ff){
                    exit_ok = true;
				}
			}

			PushToBuffer( addr_nxt, rdo);
			rdo = 0;
		}

        if( exit_ok )
            break;

        sched_yield();
	}

    Print( "run ended.\n", DEBUG);

}
