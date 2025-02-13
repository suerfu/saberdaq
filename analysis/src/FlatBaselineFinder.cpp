
#include "FlatBaselineFinder.h"
#include "PulseFinderRMS.h"

#include <map>
#include <vector>
#include <algorithm>

using std::map;
using std::vector;
using std::pair;

SaberRawWaveform FlatBaselineFinder::GetBaseline( SaberRawWaveform waveform){

    vector<uint16_t>::iterator itr;

    double baseline = 0;
    int count = 0;

    int interval = 20;
    float threshold_var = 2.5;
    int max_dev = 6;

    int max = *max_element( waveform.begin(), waveform.end() );
    int dist_from_max = 10;

    while( count == 0 ){

        for( itr=waveform.begin(); (itr+interval)<waveform.end();){
            if ( Variance( itr, itr+interval)<threshold_var && *max_element( itr, itr+interval)-*min_element(itr, itr+interval)<max_dev && max-*min_element(itr, itr+interval)<dist_from_max ){
                baseline += Average( itr, itr+interval);
                count++;
                itr += interval;
            }
            else
                itr++;
        }
        interval--;

        if( interval<5 )
            break;
    }

	if( count==0 ){
        for( itr=waveform.begin(); itr<waveform.end();itr++){
		    if(max-*itr<100){
		        baseline+=*itr;
		        count++;
			}
        }
    }

    baseline /= count;

    //cout << avg << " " << count << avg/count << endl;
    //avg /= count;
    for( itr=waveform.begin(); itr!=waveform.end(); ++itr ){
        *itr = baseline;
    }
    return waveform;
}

/*
    map< int, int> hist;
    vector<uint16_t>::iterator itr;

    for( itr=waveform.begin(); itr!=waveform.end(); ++itr ){
        if ( hist.find(*itr)!=hist.end() )
            hist[*itr]++;
        else
            hist[*itr] = 1;
    }

    for( itr=waveform.begin(); itr<waveform.begin()+200; ++itr ){
        baseline += *itr;
    }
    baseline /= 200;
    for( itr=waveform.begin(); itr!=waveform.end(); ++itr ){
        *itr = baseline;
    }
    return waveform;

    map< int, int>::iterator mapitr;
    float max1, max2, count1, count2;

    mapitr = std::max_element( hist.begin(), hist.end(), [](const pair<int, int>& p1, const pair<int, int>& p2) {return p1.second < p2.second;});
    max1 = mapitr->first;
    count1 = mapitr->second;

    mapitr->second = 0;
    mapitr = std::max_element( hist.begin(), hist.end(), [](const pair<int, int>& p1, const pair<int, int>& p2) {return p1.second < p2.second;});
    max2 = mapitr->first;
    count2 = mapitr->second;

    baseline = ( max1*count1 + max2*count2 ) / (count1+count2);

    for( itr=waveform.begin(); itr!=waveform.end(); ++itr ){
        *itr = baseline;
    }

    return waveform;
*/
