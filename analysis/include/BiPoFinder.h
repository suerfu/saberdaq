#include "SaberRawWaveform.h"

int time_sep = 25;

float GetTimePTT( SaberRawWaveform wfm ){
    // Get trigger time:
    vector<float>::iterator itr_trig = min_element( wfm.begin()+wfm.GetPreTrigSample()-25, wfm.begin()+wfm.GetPreTrigSample()+250);

    vector<float>::iterator itr_po = min_element( wfm.begin(), wfm.end());

    return 4*(itr_po - itr_trig);
}
