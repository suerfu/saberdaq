#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>

#include "ConfigParser.h"

#include "SaberDecoder.h"
#include "PulseFinderRMS.h"
#include "FlatBaselineFinder.h"

#include "TApplication.h"
#include "TCanvas.h"
#include "TSystem.h"
#include "TMultiGraph.h"
#include "TGraph.h"
#include "TAxis.h"
#include "TLine.h"
#include "TBox.h"

using namespace std;


int main( int argc, char* argv[] ){

    if( argc==1 ){
        cerr << "usage: "<< argv[0] << " [--cfg config-file] --input raw-file\n";
        return -1;
    }

    ConfigParser config( argc, argv);
    #ifdef DBG
    config.Print();
    #endif

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

    SaberDecoder decoder;
    decoder.Decode( &file );
    if( decoder.GetEventNumber()==0 ){
        cout << "no event to display\n";
        return 0;
    }

    ConfigParser rawconfig = decoder.GetConfigParser();
        // configuration file used in acquiring the data.
    
    if ( config.Find("/cmdl/comment") ){
//        rawconfig.Print();
        cout << "\n" << rawconfig.GetString("/cmdl/comment") << endl << endl;
        return 0;
    }

    vector<CAENV1720Parameter> adcparam = decoder.GetADCParameter();

    vector< string > list_label;
    for( unsigned int i=0; i<adcparam.size(); i++){
        for( unsigned int j=0; j<8; j++){
            if( ( (0x1<<j) & adcparam[i].ch_enable_mask) !=0 )
                list_label.push_back( adcparam[i].channel_param[j].label );
        }
    }

    // ======================================================================================

    TApplication* app = new TApplication( "app", 0 ,0); app->Argc();

    string input = "";
        // user choice
        //
    uint32_t eID = 0;
        // event ID to display.
    unsigned int channel = 0;
        // ADC channel to display.

    TCanvas* canvas = new TCanvas();
        // main canvas for plotting

    vector< TMultiGraph* > vec_mgrph;
        // ROOT multigraph object for plotting graphs, lines and boxes.
        
    vector< TBox*> vec_box;
        // TBox for marking spe and main pulse
    vector< TLine*> vec_line;
        // TGraph will be managed by TMultiGraph, but auxiliary boxes and lines are not. So manually draw and delete.
    
    bool draw = false;
        // flag for deciding whether to update graph.        
    bool draw_all = false;
        // if true, all waveforms will be plotted parallel in the same canvas.

    while( 1 ){

        input = getstr();
        draw = false;

        if( input=="q" || input=="Q" || input=="quit" )
            break;

        else if( input=="n" || input=="next" ){
            eID++;
            draw = true;
        }

        else if( input=="N" ){
            if(eID>0){
                eID--;
                draw = true;
            }
        }

        else if( input=="c" ){
            channel++;
            draw = true;
            draw_all = false;
        }

        else if( input=="C" ){
            channel--;
            draw = true;
            draw_all = false;
        }

        else if( input=="a" ){
            draw_all = true;
            draw = true;
        }

        else if( input=="r" ){
            eID = rand()%decoder.GetEventNumber();
            channel = 0;
            draw = true;
        }

        else if( input!=""){
            if( input.find_first_not_of("0123456789") == string::npos ){
                eID = stoi( input );
                draw = true;
            }
        }

        if( draw ){

            // obtain event from raw file
            vector<SaberRawWaveform> event = decoder.GetEvent( eID );
            if( event.size()==0 ){
                cerr << eID << "exceeds range\n";
                continue;
            }

            channel %= event.size();

            // clear previous plots, threshold lines and spe boxes.
            canvas->Clear();
            for( unsigned int i=0; i<vec_mgrph.size(); i++)
                delete vec_mgrph[i];
            vec_mgrph.clear();

            for( unsigned int i=0; i<vec_box.size(); i++)
                delete vec_box[i];
            vec_box.clear();

            for( unsigned int i=0; i<vec_line.size(); i++)
                delete vec_line[i];
            vec_line.clear();

            // if plot all waveforms together, divide canvas into rows in advance.
            if( draw_all )
                canvas->Divide( 1, event.size() );
            
            PulseFinderRMS roi_finder;
            vector<PulseInfo> roi;

            PulseFinderRMS spe_finder;
            vector<PulseInfo> spe;

            // iterate over all channels. If no draw all, then skip incorrect channels.
            for ( unsigned int ch = 0; ch<event.size(); ch++ ){

		cout << "Processing channel " << ch << endl;

                if( draw_all)
                    canvas->cd(1+ch);
                else if( ch!=channel )
                    continue;

                TMultiGraph* multigraph = new TMultiGraph();
                vec_mgrph.push_back( multigraph );

                // ====================================================================
                //                           raw waveform
                // ====================================================================

                TGraph* graph_raw = new TGraph( event[ch].size() );
                double* x = graph_raw->GetX();
                double* y = graph_raw->GetY();
                for( unsigned int i=0; i<event[ch].size(); i++ ){
                    x[i] = i*0.004;
                    y[i] = event[ch][i];
                }
                multigraph->Add( graph_raw);

                stringstream ss;
                ss << "Event " << eID << " Channel "<< ch;
                if( list_label[ch]!="")
                    ss << ", Label " << list_label[ch];
                multigraph->SetTitle( ss.str().c_str() );

                graph_raw->GetXaxis()->SetTitle("Time (us)");
                graph_raw->GetYaxis()->SetTitle("ADC Count (a.u.)");

                // ====================================================================
                //                           baseline
                // ====================================================================

		cout << "Searching for baseline..." << endl;
                FlatBaselineFinder baseline_finder;
                SaberRawWaveform baseline = baseline_finder.GetBaseline( event[ch] );
		cout << "Baseline found!" << endl;

                TGraph* graph_bsln = new TGraph( baseline.size() );
                graph_bsln->SetLineColor( kRed );

                double* x_bsln = graph_bsln->GetX();
                double* y_bsln = graph_bsln->GetY();
                for( unsigned int i=0; i<baseline.size(); i++ ){
                    x_bsln[i] = i*0.004;
                    y_bsln[i] = baseline[i];
                }
                multigraph->Add( graph_bsln);
                // ====================================================================
                //                           main pulse
                // ====================================================================
                

                // ====== main pulse ======
                
                vector< TBox* > boxes_to_draw;
                    // TBoxes to draw at the end of this canvas subdirectory.
                    // Separated from memory management.

                string dir = "/roi/";
                if( config.GetBool( "/roi-" + list_label[ch] + "/enable", false) )
                    dir = "/roi-" + list_label[ch] + "/";

                if( config.GetBool( dir + "enable", false ) ){
                    roi_finder.SetParamFromConfig( config, dir);

                    roi = roi_finder.FindPulse( event[ch], baseline );
                    cout << "Channel " << ch << " " << roi.size() << " pulses in main" << endl;
            
                    for( unsigned int i=0; i<1/*roi.size()*/; ++i){

                        TBox* box_roi = new TBox();
                            // TBox indicating region of interest

                        int index_x1 = roi_finder.search_begin;//roi[i].begin;
                        int index_x2 = roi_finder.search_begin+roi_finder.window;//roi[i].end;
                        //cout << "( " <<index_x1 << ", " << index_x2 << " )" << endl;

                        box_roi->SetX1( index_x1*0.004 );
                        box_roi->SetX2( index_x2*0.004 );

                        box_roi->SetY1( event[ch].GetMin( index_x1, index_x2) );
                        box_roi->SetY2( event[ch].GetMax( index_x1, index_x2) );

                        box_roi->SetFillStyle( 0 );
                        box_roi->SetLineColor( kBlue );
                            // color blue

                        vec_box.push_back( box_roi );
                        boxes_to_draw.push_back( box_roi);

                        //cout << "--- pulse " << i << " height: " << event[ch].Height(roi[i].begin, roi[i].end) << ", integral: " << event[ch].Integral(roi[i].begin, roi[i].end) << endl;

                    }
                }

                // ====== single photoelectron pulses ======
                
                dir = "/spe/";
                if( config.GetBool( "/spe-" + list_label[ch] + "/enable", false) )
                    dir = "/spe-" + list_label[ch] + "/";

                if( config.GetBool( dir+"enable", false ) ){
                    spe_finder.SetParamFromConfig( config, dir );
                    spe = spe_finder.FindPulse( event[ch], baseline );
                
                    cout << spe.size() << " pulses in main" << endl;

                    for( unsigned int i=0; i<spe.size(); ++i){

                        TBox* box_spe = new TBox();

                        int index_x1 = spe[i].begin;
                        int index_x2 = spe[i].end;

                        box_spe->SetX1( index_x1*0.004 );
                        box_spe->SetX2( index_x2*0.004 );

                        int local_max = event[ch].GetMax( index_x1, index_x2);
                        int local_min = event[ch].GetMin( index_x1, index_x2);

                        box_spe->SetY1( local_max );
                        box_spe->SetY2( local_min );
                        box_spe->SetFillStyle( 0 );
                        box_spe->SetLineColor( kGreen );

                        boxes_to_draw.push_back( box_spe);
/*
                        TGraph* graph_baseline = new TGraph( spe[i].size() );
                        graph_baseline->SetLineColor( kRed );
                        for( unsigned int j=0; j<spe[i].size(); j++){
                            int absindex = j+spe[i].GetTimeTag();
                            graph_baseline->GetX()[j] = absindex*0.004;
                            graph_baseline->GetY()[j] = event[ch][absindex] - spe[i][j];
                        }
                        multigraph->Add( graph_baseline );
*/
                    }
                }


                // ====== threshold ======

                vector< TLine* > lines_to_draw;
                    // TLines to draw in the end of this iteration / canvas directory.
                    // It is separated from memory management to take into the possibility of canvas subdirectory.

                TLine* line_thrsh = new TLine();
                    // TLine indicating threshold of the channel

                line_thrsh->SetX1(0);
                line_thrsh->SetX2(0.004*event[ch].size());

                line_thrsh->SetY1( event[ch].GetThreshold());
                line_thrsh->SetY2( event[ch].GetThreshold());

                line_thrsh->SetLineColor( kRed );
                line_thrsh->SetLineStyle( 10 );

                // ====== baseline ======
/*
                if( roi.size()>0 ){
                    TLine* line_baseline = new TLine();
                        // TLine indicating threshold of the channel

                    line_baseline->SetX1(0);
                    line_baseline->SetX2(0.004*event[ch].size());

                    line_baseline->SetY1( roi[0].GetBaseline());
                    line_baseline->SetY2( roi[0].GetBaseline());

                    line_baseline->SetLineColor( kGreen );
                    line_baseline->SetLineStyle( 10 );

                    vec_line.push_back( line_baseline );
                    lines_to_draw.push_back( line_baseline);
                }
*/

                multigraph->Draw("AL");

                for( vector<TLine*>::iterator itr = lines_to_draw.begin(); itr!=lines_to_draw.end(); ++itr)
                    (*itr)->Draw();

                for( vector<TBox*>::iterator itr = boxes_to_draw.begin(); itr!=boxes_to_draw.end(); ++itr)
                    (*itr)->Draw();

                canvas->Update();
            }
        }
        gSystem->ProcessEvents();
    }


    if( canvas!=0 )
        delete canvas;

    return 0;

}

void PrintHelp(){
    cout << "ENTER: " << endl;
    cout << " - H or help to display help message\n";
    cout << " - config to view config file used in the run\n";
    cout << " - h to view run information\n";
    cout << " - d to view adc parameter\n";
    cout << " - n to view next\n";
    cout << " - b to view previous\n";
    cout << " - c to increment channel\n";
    cout << " - c to decrement channel\n";
    cout << " - p to view main pulse\n";
    cout << " - s to view single photoelectron\n";
    cout << " - N to view particular pulse\n";
}


