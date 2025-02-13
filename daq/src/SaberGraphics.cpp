
#include "SaberGraphics.h"

#include "SaberRawWaveform.h"


extern "C" SaberGraphics* create_SaberGraphics( plrsController* c ){ return new SaberGraphics(c);}
    //!< create method to return the DAQ object.


extern "C" void destroy_SaberGraphics( SaberGraphics* p ){ delete p;}
    //!< destroy method.


SaberGraphics::SaberGraphics( plrsController* c ) : plrsStateMachine( c ){
    channel = 0;
    board = 0;
    draw_flag = 0xf;
    refresh_rate = 1;   // refresh interval in second

    root_app = 0;

    analysis_enable = false;
    hist_initialized = false;
}



SaberGraphics::~SaberGraphics(){
    Print( "SaberGraphics deleted\n", DETAIL);
    if( canvas!=0 ){
        delete canvas;
        canvas = 0;
    }
}



void SaberGraphics::Configure(){
    if( cparser->GetBool("/module/graphics/enable", false) ){
        root_app = new TApplication( "app", 0, 0);
        refresh_rate = cparser->GetInt( "/module/graphics/refresh_rate", refresh_rate);
    }
    Print( "SaberGraphics configured\n", DETAIL);
    canvas = new TCanvas();

    analysis_enable =  cparser->GetBool("/module/graphics/analysis/enable", false);
    if( analysis_enable ){
        roi_finder.SetParamFromConfig( *cparser, "/module/graphics/analysis/roi/");
        spe_finder.SetParamFromConfig( *cparser, "/module/graphics/analysis/spe/");
    }
}



void SaberGraphics::UnConfigure(){
    Print( "SaberGraphics unconfigured\n", INFO);
    if( canvas!=0 ){
        delete canvas;
        canvas = 0;
    }
    for( unsigned int i=0; i<hist_roi.size(); i++){
        for( unsigned int j=0; j<hist_roi[i].size(); j++){
            delete hist_roi[i][j];
            hist_roi[i][j] = 0;
        }
    }

    for( unsigned int i=0; i<hist_spe.size(); i++){
        for( unsigned int j=0; j<hist_spe[i].size(); j++){
            delete hist_spe[i][j];
            hist_spe[i][j] = 0;
        }
    }
}



void SaberGraphics::CleanUp(){
    Print( "SaberGraphics cleaning up\n", INFO);
}



void SaberGraphics::Clear(){
}



void SaberGraphics::Run(){

    TMultiGraph* mgph = 0;
    void* rdo = 0;
    now = last_update = 0;

    while( GetState()==RUN && GetStatus()!=ERROR ){

        gSystem->ProcessEvents();
            // process to allow user interaction even if graphics is stopped.

        rdo = PullFromBuffer();
        if( rdo==0 )
            continue;

        SaberDAQData*  data = reinterpret_cast<SaberDAQData*>( rdo );

        if( !data->IsHeader() ){

            now = ctrl->GetTimeStamp();

            if( analysis_enable )
                AnalyzeWaveform( data );

            if( draw_flag && (now-last_update >= refresh_rate) ){

                last_update = now;

                if( mgph!=0 ){
                    delete mgph;
                    mgph = 0;
                }
                if( canvas )
                    canvas->Clear();

                if( data->size()==0 )
                    return;
                board %= data->size();

                if( (*data)[board].GetNChannelEnabled()==0 )
                    return;

                while( ( (0x1<<channel%8)&(*data)[board].GetChannelMask()) == 0 )
                    channel = (channel+1)%8;

                // ================================================================
                //                    Draw Waveform
                // ================================================================

                if( analysis_enable ){
                    canvas->Divide(2,2);
                    canvas->cd(1);
                }
                    
                // Fix multigraph title.
                mgph = new TMultiGraph();
                stringstream ss;
                ss << "Waveform : Board " << board << " Channel " << channel << endl;
                mgph->SetTitle( ss.str().c_str() );

                int spc = (*data)[board].samp_per_chan();
                    //size()/(*data)[board].GetNChannelEnabled();
                    // number of samples per channel

                TGraph* gph = new TGraph( spc );
                mgph->Add( gph );

                // count number of channels enabled before
                int nc = 0;
                for( int i=0; i<channel; i++)
                    if( ( (0x1<<i)&(*data)[board].GetChannelMask() ) !=0 )
                        nc++;

                for( int i=0; i<spc; i++){
                    gph->GetX()[i] = i*0.004;
                    gph->GetY()[i] = (*data)[board][spc*nc+i];
                }

                mgph->Draw( "AL" );

                // ================================================================
                //                    Draw Main Histogram
                // ================================================================

                if( analysis_enable ){
                    canvas->cd(2);
                    hist_height[board][channel]->Draw();
                    canvas->cd(3);
                    hist_roi[board][channel]->Draw();
                    canvas->cd(4);
                    hist_spe[board][channel]->Draw();
                }

                canvas->Modified();
                canvas->Update();
                gSystem->ProcessEvents();
            }
        }

        PushToBuffer( addr_nxt, rdo);
        rdo = 0;
        CommandHandler();
        sched_yield();
    }

    if( mgph!=0 )
        delete mgph;
}



void SaberGraphics::CommandHandler(){

    plrsCommand cmd = PullCommand();

    string s = cmd.cmd;

    if( s=="pause" )
        draw_flag = 0;
    else if( s=="go")
        draw_flag = 1;
    else if( s=="b" )
        board++;
    else if( s=="B")
        board--;
    else if( s=="c" )
        channel++;
    else if( s=="C" )
        channel--;
    return;
}




void SaberGraphics::AnalyzeWaveform( SaberDAQData* data ){

    // if first call, allocate histogram memory for all boards and channels
    if( hist_initialized==false ){
        // iterate over all boards
        for( int i=0; i<data->size(); i++ ){

            vector< TH1F* > vec_height;
            vector< TH1F* > vec_roi;
            vector< TH1F* > vec_spe;

            for( int j=0; j<8; j++ ){

                stringstream name_height, title_height;
                name_height << "roi_height_bd"<<i<<"_ch"<<j;
                title_height << "Pulse Height - Board "<<i<<" Channel "<<j;

                vec_roi.push_back(  new TH1F( name_height.str().c_str(), title_height.str().c_str(), 4096, 0, 4096 ) );

                stringstream name_roi, title_roi;
                name_roi << "roi_bd"<<i<<"_ch"<<j;
                title_roi << "Spectrum - Board "<<i<<" Channel "<<j;

                vector< int > range = cparser->GetIntArray("/module/graphics/analysis/roi/hist_range");
                if( range.size()<3 )
                    vec_roi.push_back(  new TH1F( name_roi.str().c_str(), title_roi.str().c_str(), 10000, 0, -1000000 ) );
                else{
                    vec_roi.push_back(  new TH1F( name_roi.str().c_str(), title_roi.str().c_str(), range[0], range[1], range[2]) );
                }

                stringstream name_spe, title_spe;
                name_spe << "spe_bd"<<i<<"_ch"<<j;
                title_spe << "Single Photoelectron Spectrum - Board "<<i<<" Channel "<<j;
                range = cparser->GetIntArray("/module/graphics/analysis/spe/hist_range");
                if( range.size()<3 )
                    vec_spe.push_back(  new TH1F( name_spe.str().c_str(), title_spe.str().c_str(), 100, 0, -100) );
                else{
                    vec_spe.push_back(  new TH1F( name_spe.str().c_str(), title_spe.str().c_str(), range[0], range[1], range[2]) );
                }
            }

            hist_height.push_back( vec_height);
            hist_roi.push_back( vec_roi);
            hist_spe.push_back( vec_spe);

        }
        hist_initialized = true;
    }

    // iterate over board

    for( int bd=0; bd<data->size(); bd++){

        wfm.SetBoardID( bd );

        int sp_per_chan = (*data)[bd].samp_per_chan();
        wfm.reserve( sp_per_chan );

        for( int ch=0; ch<8; ch++){

            if( ( (0x1<<ch)&(*data)[bd].GetChannelMask() ) ==0 )
                continue;

            wfm.SetChannelID( ch );
            wfm.clear();
            for( int sp=0; sp<sp_per_chan; sp++){
                wfm.push_back( (*data)[bd][ ch*sp_per_chan+sp ] );
            }

            bsln = baseline_finder.GetBaseline( wfm );

            vector<PulseInfo> roi_pulse = roi_finder.FindPulse( wfm, bsln );
            for( unsigned int i=0; i<roi_pulse.size(); i++){
                hist_roi[bd][ch]->Fill( -wfm.Integral( roi_pulse[i].begin, roi_pulse[i].end) );
                hist_height[bd][ch]->Fill( -wfm.Height( roi_pulse[i].begin, roi_pulse[i].end) );
            }


            vector<PulseInfo> spe_pulse = spe_finder.FindPulse( wfm, bsln );
            for( unsigned int i=0; i<spe_pulse.size(); i++){
                hist_spe[bd][ch]->Fill( -wfm.Integral( spe_pulse[i].begin, spe_pulse[i].end) );
            }

        }
    }
                
}


