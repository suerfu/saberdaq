#include "SaberRawRecorder.h"
#include "SaberDAQData.h"
#include "SaberDAQHeader.h"


extern "C" SaberRawRecorder* create_SaberRawRecorder( plrsController* c ){ return new SaberRawRecorder(c);}


extern "C" void destroy_SaberRawRecorder( SaberRawRecorder* p ){ delete p;}


SaberRawRecorder::SaberRawRecorder( plrsController* c) : plrsModuleRecorder(c){}


SaberRawRecorder::~SaberRawRecorder(){}



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
        }
    }
}



void SaberRawRecorder::PreRun(){
    cparser->Serialize( output_file );
}



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

void SaberRawRecorder::PostRun(){

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
