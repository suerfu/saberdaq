// This file displays event-related information on the commandline terminal.
// It reads event header, trigger-settings, ADC-settings and event statistics.

#include "ConfigParser.h"
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>

#include "SaberDecoder.h"
#include "PulseFinderRMS.h"
#include "FlatBaselineFinder.h"

#include "TApplication.h"
#include "TTree.h"
#include "TFile.h"
#include "TH1F.h"

using namespace std;


int main( int argc, char* argv[] ){

    if( argc==1 ){
        cerr << "usage: "<< argv[0] << " [--cfg config-file] --input raw-file --ascii text-file-output --root root-file-output\n";
        return -1;
    }

    ConfigParser config( argc, argv);
    config.Print();

    string rawfile = config.GetString( "/cmdl/input" );
    if( rawfile=="" ){
        cout << "input raw file not specified.\n";
        return -1;
    }

    ifstream file;
    file.open( rawfile.c_str(), ios_base::in );
    if( !file ){
        cerr << "error opening " << rawfile.c_str() << endl;
        return -1;
    }

    string outfile = config.GetString( "/cmdl/ascii" );
    string rootoutfile = config.GetString( "/cmdl/root" );
    if( outfile=="" && rootoutfile=="" ){
        cout << "output ascii file not specified.\n";
        return -1;
    }

    TFile* rootfile = 0;
    if( rootoutfile!="" )
    rootfile = new TFile( rootoutfile.c_str(), "recreate");

    ofstream ofile;
    if( outfile!="" ){
        if( outfile.size()>5)
            ofile.open( outfile.c_str(), ios_base::out);
        if( !ofile ){
            cerr << "error opening " << outfile.c_str() << endl;
            return -1;
        }
    }

    SaberDecoder decoder;
    decoder.Decode( &file );
    
    ConfigParser rawconfig = decoder.GetConfigParser();
    vector<CAENV1720Parameter> adcparam = decoder.GetADCParameter();


    unsigned int nchan = 0;
    for( unsigned int i=0; i<adcparam.size(); i++){
        nchan += adcparam[i].GetNChannelEnabled();
    }

    uint64_t ID = 0;
        // order of the event, start from 0 and increments by 1
    uint64_t eventID = 0;

    uint64_t count = 0;

    uint32_t prevID(0), evtID(0), overflow(0), maxID( 0xffffff);
        // take care of eventID overflow
    
    vector< Waveform<double> > avg_waveform;
    for( unsigned int i=0; i< nchan; i++){
        avg_waveform.push_back( Waveform<double>() );
    }


    FlatBaselineFinder bsln_finder;
    vector< PulseFinderRMS > roi_finder;

    vector< string > list_label;

    for( unsigned int i=0; i<adcparam.size(); ++i){
        for( unsigned int j=0; j<8; j++ ){
            if( (adcparam[i].ch_enable_mask & (0x1<<j)) != 0 ){
                string name = adcparam[i].channel_param[j].label;
                list_label.push_back( name );
            }
        }
    }

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

    unsigned int n_total_event = decoder.GetEventNumber();

    for( ID=0; ID<n_total_event; ID++){

        vector<SaberRawWaveform> event = decoder.GetEvent( ID );
        if( event.size()==0 ){
            break;
        }
        if( ID%1000==0 ){
            cout.precision(3);
            cout << (100.0*ID)/n_total_event << "\% processed\n";
        }

        evtID = event[0].GetEventID();
        if( evtID<prevID )
            overflow++;
        prevID = evtID;
        eventID = overflow*maxID+evtID;

        for( unsigned int chan=0; chan<nchan; chan++){

            if( chan<roi_finder.size() ){

                SaberRawWaveform baseline = bsln_finder.GetBaseline( event[chan] );
                vector<PulseInfo> pulses = roi_finder[chan].FindPulse( event[chan], baseline );

                if( pulses.size()>0 ){

                //===================== filters here ===================================
                int temp_a = roi_finder[chan].search_begin; // begin of search window
                int temp_b = temp_a + roi_finder[chan].window;    // end of search window
                int temp_c = event[chan].size()-temp_a;    // end of waveform
                if( temp_b > temp_c)
                    temp_b = temp_c;

                double roi_height = event[chan].Height( temp_a, temp_b) - baseline[0];
                double roi_integral = event[chan].Integral( temp_a, temp_b);
                double roi_cwmt = event[chan].CWMT( temp_a, temp_b);
                
//                if( -roi_height<2800 || -roi_height>3900 )
//                    continue;

                if( -roi_height<4000 )  // alpha selector
                    continue;

                int peak_pos_glb = min_element( event[chan].begin(), event[chan].end())-event[chan].begin();
                int trig_time = (decoder.GetADCParameter())[chan].pre_trig_sample;

//                cout << peak_pos_glb << "\t" << trig_time << endl;

                if( peak_pos_glb < trig_time || peak_pos_glb > trig_time+10 )
                    continue;

                //===================== pulse alignment ================================
//                int peak_time = max_element( event[chan].begin()+roi_finder[chan].search_begin, event[chan].begin()+roi_finder[chan].search_begin+roi_finder[chan].window) - event[chan].begin();
//                int align_to = 250;
                int diff = 0;   //peak_time-align_to;
//                cout << "valid pulse " << diff << endl;

                    if( avg_waveform[chan].size()==0 ){
//                        cout << "initializing\n";
                        for( unsigned int i=0; i<event[chan].size(); i++){
                            avg_waveform[chan].push_back(0);
                        }
                    }
                    for( unsigned int i=0; i<event[chan].size(); i++){
                        if( i-diff>0 && i-diff<event[chan].size() )
                            avg_waveform[chan][i-diff] += event[chan][i]-baseline[i];
                    }
                    count++;
                }
            }
        }
    }

    cout << "\n" << count << " pulses were used for obtaining average waveform." << endl;

    unsigned int max_size = 0;

    if( ofile.is_open() ){
        ofile << "#\n\n# Average waveform from " << count << " pulses\n#\n#";
        for( unsigned int i=0; i<avg_waveform.size(); i++ ){
            ofile << " channel" << i;

            if( max_size<avg_waveform[i].size() )
                max_size = avg_waveform[i].size();
        }
        ofile << endl;

        for( unsigned int i=0; i<max_size; i++){
            for( unsigned int w=0; w<avg_waveform.size(); w++){
                if( i<avg_waveform[w].size() )
                    ofile << avg_waveform[w][i]/count << '\t';
                else
                    ofile << -1 << "\t";
            }
            ofile << endl;
        }

        ofile.close();
    }

    if( rootfile!=0 ){
        for( unsigned int w=0; w<avg_waveform.size(); w++){
            stringstream ss;
            ss << "avg_pulse_" << w;
            TH1D* avg_pulse =  new TH1D( ss.str().c_str(), "average pulse shape", avg_waveform[w].size(), 0, 0.004*avg_waveform[w].size());

            for( unsigned int i=0; i<avg_waveform[w].size(); i++){
                if( avg_waveform[w][i]<0 )
                    avg_pulse->Fill( 0.004*i, -avg_waveform[w][i]);
            }
            avg_pulse->Sumw2();
            avg_pulse->Scale(1./count);
        }
        rootfile->Write();
        rootfile->Close();
    }
    

    return 0;
}
