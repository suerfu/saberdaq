// This file displays event-related information on the commandline terminal.
// It reads event header, trigger-settings, ADC-settings and event statistics.

#include "ConfigParser.h"
#include <fstream>
#include <string>
#include <iostream>

#include "SaberDecoder.h"
#include "PulseFinderRMS.h"
#include "FlatBaselineFinder.h"

#include "BiPoFinder.h"

#include "TApplication.h"
#include "TTree.h"
#include "TFile.h"

using namespace std;


int main( int argc, char* argv[] ){

    if( argc==1 ){
        cerr << "usage: "<< argv[0] << " [--cfg config-file --print --runinfo --close-header] --input raw-file --output root-file\n";
        return -1;
    }

    ConfigParser config( argc, argv);
    if( config.Find( "/cmdl/print") )
        config.Print();

    int Nproc = config.GetInt( "/cmdl/n", 2000000000);

    // input file

    string rawfile = config.GetString( "/cmdl/input" );
    if( rawfile=="" ){
        cout << "error: input raw file not specified.\n";
        return -1;
    }

    ifstream file;
    file.open( rawfile.c_str(), ios_base::in );
    if( !file ){
        cerr << "error opening " << rawfile.c_str() << endl;
        return -1;
    }

    // output file

    TFile* tf = 0;
    string outfile = config.GetString( "/cmdl/output" );
    if( outfile != "" ){
        tf = new TFile( outfile.c_str(), "RECREATE");
        if( !tf->IsOpen() ){
            cerr << "Error opening root file\n";
            delete tf;
            return -1;
        }
    }

    // event decoder

    SaberDecoder decoder;
    decoder.Decode( &file );

    // flag variables

    bool print_runinfo = config.Find( "/cmdl/runinfo" );
    bool no_close_header = decoder.GetEndTime()<=decoder.GetBeginTime();
    bool process_event = ( tf!=0);
        // if root file is successfully created, then all data has to be processed.
    bool go_through_data = ( process_event || no_close_header );
        // if total runtime has to be calculated,
        // the event file has to be gone through without detailed processing.
    bool verbose = config.Find("/cmdl/verbose") || config.Find("/cmdl/v");

 
    ConfigParser rawconfig = decoder.GetConfigParser();
    vector<CAENV1720Parameter> adcparam = decoder.GetADCParameter();


    unsigned int nchan = 0;
    for( unsigned int i=0; i<adcparam.size(); i++){
        nchan += adcparam[i].GetNChannelEnabled();
    }

    ULong64_t ID = 0;
        // order of the event, start from 0 and increments by 1
    ULong64_t eventID;
        // event ID, obtained from raw file

    uint32_t prevID(0), evtID(0), overflow(0), maxID( 0xffffff);
        // take care of eventID overflow
    
    // ==============================================
    //         variable length array
    // ==============================================
    const int NROI = 128;
    const int NSPE = 256;

    unsigned int* nroi = new unsigned int[nchan];
    unsigned int* nspe = new unsigned int[nchan];
        // total number of channels in the data file.

    int txthreshold[NROI];

    uint32_t time_sec = 0;   // trigger time tag in sec since beginning
    uint32_t time_ns = 0;    // trigger time tag in ns

    int** roi_start_time = new int*[nchan];
    int** roi_duration = new int*[nchan];
    int** roi_peak_time = new int*[nchan];
    float** roi_height = new float*[nchan];
    float** roi_integral = new float*[nchan];
    float** roi_cwmt = new float*[nchan];

    float** roi_psd_20 = new float*[nchan];
    float** roi_psd_40 = new float*[nchan];
    float** roi_psd_60 = new float*[nchan];
    float** roi_psd_80 = new float*[nchan];
    float** roi_psd_100 = new float*[nchan];
    float** roi_psd_120 = new float*[nchan];

    float* tail_integral = new float[nchan];
    float* tail_height = new float[nchan];
    float* tail_cwmt = new float[nchan];

    int** spe_peak_time = new int*[nchan];
    float** spe_height = new float*[nchan];
    float** spe_integral = new float*[nchan];
    int** spe_duration = new int*[nchan];

    float* time_ptt = new float[nchan];

    for( unsigned int i=0; i<nchan; i++){
        roi_start_time[i] = new int[NROI];
        roi_duration[i] = new int[NROI];
        roi_peak_time[i] = new int[NROI];
        roi_height[i] = new float[NROI];
        roi_integral[i] = new float[NROI];
        roi_cwmt[i] = new float[NROI];

        roi_psd_20[i] = new float[NROI];
        roi_psd_40[i] = new float[NROI];
        roi_psd_60[i] = new float[NROI];
        roi_psd_80[i] = new float[NROI];
        roi_psd_100[i] = new float[NROI];
        roi_psd_120[i] = new float[NROI];

        spe_peak_time[i] = new int[NSPE];
        spe_height[i] = new float[NSPE];
        spe_integral[i] = new float[NSPE];
        spe_duration[i] = new int[NROI];

    }

    TTree* tree = 0;
    vector< string > list_label;

    if(tf != 0 ){
        tree = new TTree( "event", "PMT signal" );
        tree->Branch( "ID", &ID, "ID/l" );
        tree->Branch( "eventID", &eventID, "eventID/l" );
        tree->Branch( "time_sec", &time_sec, "time_sec/l" );
        tree->Branch( "time_ns", &time_ns, "time_ns/l" );

        for( unsigned int i=0,  i_ch_enabled=0; i<adcparam.size(); ++i){
            for( unsigned int j=0; j<8; j++ ){
                if( (adcparam[i].ch_enable_mask & (0x1<<j)) != 0 ){
                    string name = adcparam[i].channel_param[j].label;
                    list_label.push_back( name );

                    name = "";
                    if( name=="" ){
                        stringstream ss; ss << "ch" << j;
                        name = ss.str();
                    }

                    tree->Branch( (name+"_txthresh").c_str(), &txthreshold[i_ch_enabled], (name+"_txthreshold/I").c_str() );
                    tree->Branch( (name+"_nroi").c_str(), nroi, (name+"_nroi/I").c_str() );
                    tree->Branch( (name+"_roi_start_time").c_str(), roi_start_time[i_ch_enabled], (name+"_roi_start_time["+name+"_nroi]/I").c_str() );
                    tree->Branch( (name+"_roi_duration").c_str(), roi_duration[i_ch_enabled], (name+"_roi_duration["+name+"_nroi]/I").c_str() );
                    tree->Branch( (name+"_roi_peak_time").c_str(), roi_peak_time[i_ch_enabled], (name+"_roi_peak_time["+name+"_nroi]/I").c_str() );
                    tree->Branch( (name+"_roi_height").c_str(), roi_height[i_ch_enabled], (name+"_roi_height["+name+"_nroi]/F").c_str() );
                    tree->Branch( (name+"_roi_integral").c_str(), roi_integral[i_ch_enabled], (name+"_roi_integral["+name+"_nroi]/F").c_str() );
                    tree->Branch( (name+"_roi_cwmt").c_str(), roi_cwmt[i_ch_enabled], (name+"_roi_cwmt["+name+"_nroi]/F").c_str() );

                    tree->Branch( (name+"_roi_psd_20").c_str(), roi_psd_20[i_ch_enabled], (name+"_roi_psd_20["+name+"_nroi]/F").c_str() );
                    tree->Branch( (name+"_roi_psd_40").c_str(), roi_psd_40[i_ch_enabled], (name+"_roi_psd_40["+name+"_nroi]/F").c_str() );
                    tree->Branch( (name+"_roi_psd_60").c_str(), roi_psd_60[i_ch_enabled], (name+"_roi_psd_60["+name+"_nroi]/F").c_str() );
                    tree->Branch( (name+"_roi_psd_80").c_str(), roi_psd_80[i_ch_enabled], (name+"_roi_psd_80["+name+"_nroi]/F").c_str() );
                    tree->Branch( (name+"_roi_psd_100").c_str(), roi_psd_100[i_ch_enabled], (name+"_roi_psd_100["+name+"_nroi]/F").c_str() );
                    tree->Branch( (name+"_roi_psd_120").c_str(), roi_psd_120[i_ch_enabled], (name+"_roi_psd_120["+name+"_nroi]/F").c_str() );

                    tree->Branch( (name+"_tail_height").c_str(), &tail_height[i_ch_enabled], (name+"_tail_height/F").c_str());
                    tree->Branch( (name+"_tail_integral").c_str(), &tail_integral[i_ch_enabled], (name+"_tail_integral/F").c_str());
                    tree->Branch( (name+"_tail_cwmt").c_str(), &tail_cwmt[i_ch_enabled], (name+"_tail_cwmt/F").c_str());
        
                    tree->Branch( (name+"_nspe").c_str(), nspe, (name+"_nspe/I").c_str() );
                    tree->Branch( (name+"_spe_peak_time").c_str(), spe_peak_time[i_ch_enabled], (name+"_spe_peak_time["+name+"_nspe]/I").c_str() );
                    tree->Branch( (name+"_spe_height").c_str(), spe_height[i_ch_enabled], (name+"_spe_height["+name+"_nspe]/F").c_str() );
                    tree->Branch( (name+"_spe_integral").c_str(), spe_integral[i_ch_enabled], (name+"_spe_integral["+name+"_nspe]/F").c_str() );
                    tree->Branch( (name+"_spe_duration").c_str(), spe_duration[i_ch_enabled], (name+"_spe_duration["+name+"_nspe]/I").c_str() );
                    
                    tree->Branch( (name+"_time_ptt").c_str(), &time_ptt[i_ch_enabled], (name+"_time_ptt/F").c_str() );

                    i_ch_enabled++;
                }
            }
        }
    }

    FlatBaselineFinder bsln_finder;

    vector< PulseFinderRMS > roi_finder;
    vector< PulseFinderRMS > spe_finder;

    if( tree!= 0 ){
        for( unsigned int i=0; i<list_label.size(); ++i){
            if( config.GetBool( "/roi-"+list_label[i]+"/enable", false ) ){
                roi_finder.push_back( PulseFinderRMS() );
                roi_finder.back().SetParamFromConfig( config, "/roi-"+list_label[i]+"/");
            }
            else if( config.GetBool( "/roi/enable", false ) ){
                roi_finder.push_back( PulseFinderRMS() );
                roi_finder.back().SetParamFromConfig( config, "/roi/");
            }
        }

        for( unsigned int i=0; i<list_label.size(); ++i){
            if( config.GetBool( "/spe-"+list_label[i]+"/enable", false ) ){
                spe_finder.push_back( PulseFinderRMS() );
                spe_finder.back().SetParamFromConfig( config, "/spe-"+list_label[i]+"/");
            }
            else if( config.GetBool( "/spe/enable", false ) ){
                spe_finder.push_back( PulseFinderRMS() );
                spe_finder.back().SetParamFromConfig( config, "/spe/");
            }
        }
    }


    uint32_t prev_ttt = 0;
    uint32_t ttt_accumulator = 0;

    float ttt_period = 17.179869176;  // ttt is 125 MHz, or 8 ns spacing. It has 31 digits, so 2^31*8/10^9

    unsigned int n_total_event = decoder.GetEventNumber();

    if( Nproc < n_total_event )
        n_total_event = Nproc;

    if( go_through_data )
    for( ID=0; ID<n_total_event; ID++){

        vector<SaberRawWaveform> event = decoder.GetEvent( ID );
        if( event.size()==0 ){
            break;
        }

        if( verbose && ID%5000==0 ){
            cout.precision(2);
            cout << 100.0*ID/n_total_event << "\% processed\n";
        }

        if( prev_ttt>event[0].GetTrigTimeTag() )
            ttt_accumulator += 1;
        prev_ttt = event[0].GetTrigTimeTag();

        if( tree!= 0 ){

            if( evtID<prevID )
                overflow++;
            prevID = evtID;
            eventID = overflow*maxID+evtID;

            time_ns = event[0].GetTrigTimeTag()*8;
            time_sec = ttt_accumulator*ttt_period;

            for( unsigned int chan=0; chan<nchan; chan++){

                int threshold = event[chan].GetThreshold();
                vector<int> txt = event[chan].GetTXThreshold( threshold );

                if( txt.size()>=1)
                    txthreshold[chan] = txt[0];
                else
                    txthreshold[chan] = -1;

                SaberRawWaveform baseline = bsln_finder.GetBaseline( event[chan]);
                for( unsigned int i=0; i<event[chan].size(); i++){
                    event[chan][i] -= baseline[i];
                    baseline[i] = 0;
                }

                if( chan<roi_finder.size() ){
                    vector<PulseInfo> pulses = roi_finder[chan].FindPulse( event[chan], baseline );

                    nroi[chan] = pulses.size();
                    if( nroi[chan]==0 ){
                        roi_start_time[chan][0] = -1;
                        roi_duration[chan][0] = -1;
                        roi_height[chan][0] = 0;
                        roi_integral[chan][0] = 0;
                        roi_cwmt[chan][0] = -1;

                        roi_psd_20[chan][0] = -1;
                        roi_psd_40[chan][0] = -1;
                        roi_psd_60[chan][0] = -1;
                        roi_psd_80[chan][0] = -1;
                        roi_psd_100[chan][0] = -1;
                        roi_psd_120[chan][0] = -1;
                    }

                    int temp_a = roi_finder[chan].search_begin; // begin of search window
                    int temp_b = temp_a + roi_finder[chan].window;    // end of search window
                    int temp_c = event[chan].size()-temp_a;    // end of waveform
                    if( temp_b > temp_c)
                        temp_b = temp_c;

                    roi_height[chan][0] = event[chan].Height( temp_a, temp_b);
                    roi_integral[chan][0] = event[chan].Integral( temp_a, temp_b);
                    roi_cwmt[chan][0] = event[chan].CWMT( temp_a, temp_b);

                    roi_psd_20[chan][0] = event[chan].Integral( temp_a, temp_a+20/4)/roi_integral[chan][0];
                    roi_psd_40[chan][0] = event[chan].Integral( temp_a, temp_a+40/4)/roi_integral[chan][0];
                    roi_psd_60[chan][0] = event[chan].Integral( temp_a, temp_a+60/4)/roi_integral[chan][0];
                    roi_psd_80[chan][0] = event[chan].Integral( temp_a, temp_a+80/4)/roi_integral[chan][0];
                    roi_psd_100[chan][0] = event[chan].Integral( temp_a, temp_a+100/4)/roi_integral[chan][0];
                    roi_psd_120[chan][0] = event[chan].Integral( temp_a, temp_a+120/4)/roi_integral[chan][0];

                    tail_height[chan] = event[chan].Height( temp_b-500/4, temp_b );
                    tail_integral[chan] = event[chan].Integral( temp_b-500/4, temp_b );
                    tail_cwmt[chan] = event[chan].CWMT( temp_b-500/4, temp_b );

                    for( unsigned int i=0; i<nroi[chan]; ++i ){
                        roi_start_time[chan][i] = pulses[i].start_time;
                        roi_duration[chan][i] = pulses[i].end - pulses[i].begin;
                        //roi_height[chan][i] = event[chan].Height( pulses[i].begin, pulses[i].end);
                        //roi_integral[chan][i] = event[chan].Integral( pulses[i].begin, pulses[i].end);
                        //roi_cwmt[chan][i] = event[chan].CWMT( 0, event[chan].size() );
                    }
                }

                if( chan<spe_finder.size() ){
        
                    vector<PulseInfo> spe_pulse = spe_finder[chan].FindPulse( event[chan], baseline );
                    nspe[chan] = spe_pulse.size();
                    for( unsigned int i=0; i<nspe[chan]; ++i ){
                        spe_height[chan][i] = event[chan].Height( spe_pulse[i].begin, spe_pulse[i].end );
                        spe_integral[chan][i] = event[chan].Integral( spe_pulse[i].begin, spe_pulse[i].end );
                        spe_duration[chan][i] = spe_pulse[i].end - spe_pulse[i].begin;
                    }
                }

                time_ptt[chan] = GetTimePTT( event[chan] ); // was time_ptt
            }
            tree->Fill();
        }
    }
    file.close();

    if( print_runinfo){

        cout << "run info:\n";
        time_t tmbeg = decoder.GetBeginTime();

        time_t tmend = decoder.GetEndTime();
        if( no_close_header )
            tmend = decoder.GetBeginTime() + ttt_accumulator*ttt_period;

        cout << "    run began at " << tmbeg << " = " << ctime( &tmbeg);
        cout << "    run ended at " << tmend << " = " << ctime( &tmend);
        cout << "    duration (s): " << tmend - tmbeg << endl;

        cout << "    total event: " << decoder.GetEventNumber()<<endl;
        cout << "    trigger rate: " << 1.0*decoder.GetEventNumber() / (tmend-tmbeg) << endl;
        cout << "\n";
        cout << "    " << rawconfig.GetString("/cmdl/comment");
        cout << "\n" << endl;
    }

    if( no_close_header ){

        cout << "\nend header in hex format (use xxd to convert o binary): ";
        uint32_t glb_header2[5];
        glb_header2[0] = 0xff1234ff;
        glb_header2[1] = 5*sizeof( glb_header2[0] );
        glb_header2[2] = 0;
        glb_header2[3] = decoder.GetBeginTime() + ttt_accumulator*ttt_period;
        glb_header2[4] = 0xff1234ff;

        char* output = reinterpret_cast<char*>(glb_header2);
        for( int i=0; i<20; i++){
            uint32_t a = output[i];
            cout << setfill('0') << setw(2) << hex << (0xff&a) << ' ';
        }
        cout << endl;

        if( config.Find("/cmdl/close-header") ){
            ofstream ofile;
            ofile.open( rawfile.c_str(), ios_base::app);
            ofile.seekp( ios_base::end );
            ofile.write( reinterpret_cast<char*>(glb_header2), 5*sizeof( glb_header2[0]));
            ofile.close();
        }
            
    }

    if( tree != 0 )
        tree->Write();

    if( tf !=0 ){
        tf->Write();
        tf->Close();
        delete tf;
    }

    return 0;
}
