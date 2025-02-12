
#include "PulseFinderRMS.h"


#include <algorithm>
#include <iostream>

using std::cin;
using std::cout;
using std::cerr;
using std::endl;


PulseFinderRMS::PulseFinderRMS(){

    // set default values.

    threshold = 5;

    window = 5;

    search_begin = 0;
    search_end = -1;

    only_peak_threshold = -1;

    pre_var_threshold = 3;
    pre_var_win = 20;
    pre_ncross_0 = 1;

    post_var_threshold = 3;
    post_var_win = 20;

    pre_max_dev = 4;
    post_max_dev = 4;
    pre_ncross_0 = 1;
}



PulseFinderRMS::~PulseFinderRMS(){;}




void PulseFinderRMS::SetParamFromConfig( ConfigParser config, string dir){

    threshold = config.GetFloat( dir+"threshold", threshold);

    window = config.GetFloat( dir+"window", window);

    search_begin = config.GetInt( dir+"search_begin", search_begin);
    search_end = config.GetInt( dir+"search_end", search_end);
    
    only_peak_threshold = config.GetFloat(  dir+"monopeak_threshold", only_peak_threshold );

    pre_var_win = config.GetFloat( dir+"pre_window", pre_var_win);
    post_var_win = config.GetFloat( dir+"post_window", post_var_win);

    pre_var_threshold = config.GetFloat(  dir+"pre_win_var_threshold", pre_var_threshold );
    post_var_threshold = config.GetFloat(  dir+"post_win_var_threshold", post_var_threshold );

    pre_ncross_0 = config.GetInt( dir+"pre_ncross_0", pre_ncross_0);
    post_ncross_0 = config.GetInt( dir+"post_ncross_0", post_ncross_0);
}



vector<PulseInfo> PulseFinderRMS::FindPulse( SaberRawWaveform waveform, SaberRawWaveform baseline ){

    // subtract baseline.
    for( unsigned int i = 0; i<waveform.size(); i++){
        //cout << i << '\t' << waveform[i] << '\t' << baseline[i] << '\t';
        waveform[i] -= baseline[i];
        //cout << waveform[i] << endl;
    }

    std::vector<PulseInfo> pulses;
        // pulse object to return in the end.


    // use search begin and search end as boundary for locating the peak of pulse
    // once peak is located, move to left to identify beginning of pulse by testing RMS
    // once beginning of pulse is identified, move by integration window to end of pulse/window

    float max_elem = *std::max_element( waveform.cbegin(), waveform.cend());
        // find maximum element of entire waveform and use a number larger than max as a marker.
        // this marker is chosesn to be larger than global maximum.


    bool out_of_range = false;
        // if true, pulse is too close to the boundaries.

    bool overlap = false;
        // if true, pulses are too close to each other


    while(1) {
        std::vector<uint16_t>::iterator itr;
            // iterator for general use

        // boundary of waveform iterators.
        std::vector<uint16_t>::iterator wfmbegin =  waveform.begin();
            // begin of wavefor
        std::vector<uint16_t>::iterator wfmend = waveform.end();
            // end of waveform

        // if beginnining is out of range, immediately return.
        if( wfmbegin>=wfmend )
            return pulses;

        if( wfmbegin+search_begin>=wfmend){
            //cout << "search begin is beyond end of waveform."<<endl;
            break;
        }

        if( search_end <0 )
            // if not specified, default to entire waveform.
            search_end = wfmend - wfmbegin;
        
        // locate minimum element and start searching to left for beginning of pulse
        // minimum of the pulse must be withing the specified range.
        std::vector<uint16_t>::iterator minpos = std::min_element( wfmbegin+search_begin, wfmbegin+search_end > wfmend ? wfmend : wfmbegin+search_end);
        //cout << "\nminimum located at " << minpos - wfmbegin << ", " << *minpos << endl;
        if( minpos==wfmend ){
            //cout << "warning: minimum is at end of pulse." << endl;
            break;
        }

        // ======================================================================================
        // get baseline and test for termination criteria
        // ======================================================================================
        
        if( -(*minpos) < threshold ){
            //cout << "pulse height found is smaller than threshold, terminating.\n";
            break;
        }
            // termination criteria - the difference between max and min is below the threshold.


        // ======================================================================================
        // find begin of pulse by decrementing
        // ======================================================================================

        std::vector<uint16_t>::iterator plsbegin = minpos - 1; // pulse begin is pre_var_win before minimum position
        if( plsbegin < wfmbegin )
            plsbegin = wfmbegin;
        //cout << "search will start from " << plsbegin - wfmbegin << endl;

        //float local_baseline = 0;
            // used at the beginning of pulse to tell pulse height, and thereforth termination criteria.

        while( 1 ){
            // first test if beginning of pulse is beyond range
            if( plsbegin - pre_var_win < wfmbegin ){
                out_of_range = true;
                plsbegin = wfmbegin;
                //cout << "begin of pulse too close to the beginning of waveform.\n";
                break;
            }
            // test if there is overlap by checking the value at begin of pulse and compare to the marker
            else if( *plsbegin > max_elem ){
                overlap = true;
                break;
            }
            // if above two criteria are passed, check if it is a clean baseline.
            if( TestBaseline( waveform, plsbegin-pre_var_win, plsbegin, pre_var_threshold, pre_max_dev)){
                //local_baseline = Average( plsbegin-pre_var_win, plsbegin );
                //cout << "test baseline passed\n";

                int ncross = 0;  // number of times that waveform crosses 0.
                while( ncross<pre_ncross_0 && plsbegin-pre_var_win>wfmbegin ){
                    if( *plsbegin * (*(plsbegin-1)) < 0 )
                        ncross++;
                    plsbegin--;
                }
                break;
            }
            else{
                plsbegin--;
                //cout << "baseline test did not pass. Decrementing begin of pulse to " << plsbegin - wfmbegin << endl;
            }
        }


        // ======================================================================================
        // find end of pulse by incrementing
        // ======================================================================================
        
        std::vector<uint16_t>::iterator plsend = plsbegin + window;    // window is integration window
        if( plsend<=minpos )
            plsend=minpos+1;
        //cout << "end of pulse set at " << plsend - wfmbegin << endl;


        while( 1 ){
                // increment, then judge if termination criteria is met
                // endpoint is also inclusive - it can be dereferenced.
            if( plsend+post_var_win >= wfmend ){
                out_of_range = true;
                plsend = wfmend;
                //cout << "end of pulse too close to end of waveform.\n";
                break;
            }
            else if( *plsend > max_elem ){   // plsend + window is guaranteed to be valid element
                overlap = true;
                //cout << *plsend << ", " << max_elem << endl;
                //cout << "end of pulse overlapping.\n";
                break;
            }
            if( TestBaseline( waveform, plsend, plsend+post_var_win, post_var_threshold, post_max_dev)){ // TestBaseline end is exclusive.
                //cout << "end of pulse baseline passed.\n";
                int ncross = 0;
                while( ncross<post_ncross_0 && (plsend+post_var_win)<=wfmend ){
                    if( *plsend * (*(plsend+1)) < 0 )
                        ncross++;
                    plsend++;
                }
                break;
            }
            else{
                //cout << "incrementing end of pulse to " << plsend - wfmbegin << endl;
                plsend++;
            }
        }


        // ======================================================================================
        // handle out of range problem and region overlapping
        // ======================================================================================
        
        if( overlap || out_of_range ){
            for( itr = plsbegin; itr<plsend; ++itr){
                //cout << "setting " << itr-wfmbegin << " to marker" << endl;
                *itr = max_elem+1;
            }
            overlap = false;
            out_of_range = false;
            continue;
        }

        // ======================================================================================
        // number of samples below zero.
        // for spe, there could be noise, which is short.
        // ======================================================================================

        int below0_count = 1;

        itr = minpos;
        while( itr>=plsbegin && *itr<0 ){
            itr--;
            below0_count++;
        }

        itr = minpos;
        while( itr<plsend && *itr<0 ){
            itr++;
            below0_count++;
        }

        if( below0_count < 3 ){
            for( itr = plsbegin; itr<plsend; ++itr ){
                *itr = max_elem+1;
            }
            continue;
        }
        //cout << below0_count << " samples under 0 " << endl;

        // ======================================================================================
        // in addition, require that it is only peak in the range
        // ======================================================================================

        if( only_peak_threshold > 0 ){
    
            itr = minpos;
            std::vector<uint16_t>::iterator peak_left = minpos;
            std::vector<uint16_t>::iterator peak_right = minpos;

            //cout << "pulse begin and end: " << 4*(plsbegin-wfmbegin) << ", " << 4*(plsend-wfmbegin) << endl;
            while( peak_left-1>=plsbegin && *(peak_left)<(only_peak_threshold-0.1)*(*minpos) ){
                peak_left--;
            }

            while( peak_right+1<plsend && *(peak_right)<(only_peak_threshold-0.1)*(*minpos) ){
                peak_right++;
            }
//cout << "min at: " << 4*(minpos-wfmbegin) <<", left: " << 4*(peak_left-wfmbegin) << ", right: " << 4*(peak_right - wfmbegin) << endl;

            float lpeak = *std::min_element( plsbegin-pre_var_win, peak_left );
            float rpeak = *std::min_element( peak_right, plsend+post_var_win );
//cout <<"peak: " << *minpos << "; next two biggest peaks: " << lpeak << ", " << rpeak << endl;
            if( lpeak < (*minpos)*only_peak_threshold || rpeak < (*minpos)*only_peak_threshold ){
                for( itr = plsbegin; itr<plsend; ++itr ){
                    *itr = max_elem+1;
                }
                continue;
            }
        }
        // ======================================================================================
        // standard deviation test
        // ======================================================================================
        /*
        std::vector<uint16_t> temp_avg;
        for( itr = plsbegin-pre_var_win; itr<plsend+post_var_threshold; itr++ ){
            if( itr < peak_left || itr > peak_right )
                temp_avg.push_back( *itr );
        }
        
        float avg = Average(temp_avg.begin(), temp_avg.end());
        float var = Variance( temp_avg.begin(), temp_avg.end());
        //cout << "min, avg, var = " << *minpos <<", "<< avg << ", " << var << ", " << (avg-*minpos)/sqrt(var) << endl;
        //cout << avg-*minpos << endl << sqrt(var) << endl;
        
        if( (avg - *minpos) / sqrt(var) < 2 ){
            for( itr = plsbegin; itr<plsend; ++itr ){
                *itr = max_elem+1;
            }
            continue;
        }
        */

        // ======================================================================================
        // valid pulse
        // ======================================================================================
        
        // if program reaches this point, plsbegin and plsend are both valid, with plsend inclusive.
        PulseInfo tmp;
        tmp.begin = plsbegin - wfmbegin;
        tmp.end = plsend - wfmbegin;

        /*
        // baseline subtraction. --- baseline should have already been subtracted
        // interpolate the baseline.
        float lavg = Average( plsbegin, plsbegin+pre_var_win );
            // left average
            // Average function right endpoint is exclusive, so shift by one to include endpoint.
        float ravg = Average( plsend-post_var_win, plsend );
            // right average
        float rate = (ravg - lavg) / ( plsend-post_var_win/2 - plsbegin - pre_var_win/2 );
            // change rate per unit distance from begin of pulse
        */

        // find pulse start time.
        // pulse start time defined as the first sample outside 2-sigmas away from baseline.

        /*
        float lavg = Average( plsbegin-pre_var_win, plsbegin );
            // left average, needed to compute pulse start time

        float twosig_threshold = lavg-2*Variance( plsbegin - pre_var_win, plsbegin );
        for( itr=minpos; itr>=plsbegin-pre_var_win; --itr ){
            if( *itr>twosig_threshold ){
                tmp.start_time = itr - (plsbegin+pre_var_win/2);
                break;
            }
        }
        */

        // write waveform to return variable.
        bool ovlp = false;
        for( itr = plsbegin; itr<plsend; itr++ )
            if( *itr>max_elem ){
                ovlp = true;
                break;
            }
        for( itr = plsbegin; itr<plsend; ++itr ){
            *itr = max_elem+1;
        }

//        if( -*minpos<threshold ){
//            return pulses;
//        }

        if( ovlp==false)
            pulses.push_back( tmp );
        //cout << "pushing back pulse info"<<endl;
    }

    //cout << "returning"<<endl;
    return pulses;
}


bool PulseFinderRMS::TestBaseline( const SaberRawWaveform& wfm, std::vector<uint16_t>::const_iterator beg, std::vector<uint16_t>::const_iterator end, float varthresh, float maxdev ){

    float var = Variance( beg, end );
    if( var>varthresh ){
        return false;
    }

    std::vector<uint16_t>::const_iterator itr;
    int dev = *max_element( beg, end) - *min_element( beg, end);

    if( dev > maxdev ){
        return false;
    }

    return true;
}
