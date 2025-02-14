#ifndef SABERGRAPHICS_H
    #define SABERGRAPHICS_H 1

#include "plrsStateMachine.h"
#include "SaberDAQData.h"

#include "TApplication.h"
#include "TSystem.h"
#include "TCanvas.h"

#include "TH1F.h"
#include "TMultiGraph.h"
#include "TGraph.h"
#include "TLine.h"
#include "TBox.h"

#include "PulseFinderRMS.h"
#include "FlatBaselineFinder.h"

using std::vector;

#include <iostream>



class SaberGraphics : plrsStateMachine {

public:

    SaberGraphics( plrsController* c);

    ~SaberGraphics();

protected:

    std::string GetModuleName(){ return "graphics";}

    void Configure();

    void UnConfigure();

    void CleanUp();

    void Run();

    virtual void Clear();

    void CommandHandler();

    int GetNextModuleID();

private:

    int next_addr;

    int board;

    int channel;

    uint32_t draw_flag;
        // LSB enable/disables drawing
        // bit 1 enables will draw waveform, disable to move to bit 2
        // bit 2 enables drawing channel main histogram, disable to move to bit 3
        // bit 3 enables drawing spe histogram

    uint32_t refresh_rate;

    uint32_t last_update, now;

    TApplication* root_app;

    TCanvas* canvas;

    void AnalyzeWaveform( SaberDAQData* data);
        // simple online analysis and monitoring

    bool analysis_enable;

    bool hist_initialized;

    vector< vector<TH1F*> > hist_height;

    vector< vector<TH1F*> > hist_roi;

    vector< vector<TH1F*> > hist_spe;

    PulseFinderRMS roi_finder;
        // PulseFinder for main pulse

    PulseFinderRMS spe_finder;
        // PulseFinder for single photoelectron

    SaberRawWaveform wfm;
        // hold raw waveform information from each channel

    FlatBaselineFinder baseline_finder;

    SaberRawWaveform bsln;
        // temporarily hold baseline info for subtraction.

};



extern "C" SaberGraphics* create_SaberGraphics( plrsController* c);



extern "C" void destroy_SaberGraphics( SaberGraphics* p);




#endif
