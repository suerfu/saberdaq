#include "CAENV1720.h"


CAENV1720::CAENV1720( int32_t h, CAENV1720Parameter p): VMEBoard<CAENV1720Parameter>( h, p){

    Reset();

    //cout << "ROC firmware: " << GetROCFirmware() << endl;

    /* configure local channel settings, threshold, dac, etc. */
    ConfigLocalChannel();
    ConfigChannel();
    
    /* get channel firmware version */
    //for( uint32_t i=0; i<8; i++){
    //    cout << "AMC firmware : " << i << " = " << GetChannelAMCFirmware(i) << endl;
    //}

    /* pre and post trigger settings, buffer organization */
    ConfigBuffer();

    /* enable channels and trigger source mask */
    EnableChannels();
    ConfigTrigSource();

    /* front panel LEMO trigger out */
    ConfigFPTrigOut();
    ConfigFPIO();

    WriteRegister( ACQ_CON, 0x8);   // count all triggers
    WriteRegister( 0x81a0, 0x1111);   // Configures LVDS to reflect channel trigger status.

    SetEvtNumberBLT(1);
    SWClear();
}
// During the initialization of VMEBoard, Reset is called. Next param object is set to the argument and initialize is called accordingly.



CAENV1720::~CAENV1720(){}



void CAENV1720::Initialize(){

    if( Running() )
        StopBoard();

    /* local channel settings */
    ConfigLocalChannel();

    /* global channel configuration */
    ConfigChannel();

    //for( uint32_t i=0; i<8; i++){
    //    cout << "AMC firmware : " << i << " = " << GetChannelAMCFirmware(i) << endl;
    //}

    /* enable channels */
    EnableChannels();

    /* event buffer organization */
    ConfigBuffer();

    /* trigger source and output */
    ConfigTrigSource();
    ConfigFPTrigOut();
    ConfigFPIO();

    /*  configure acquisition */
    SetRunMode(param.runmode);

    WriteRegister( 0x81a0, 0x1111);
        // Configures LVDS to reflect channel trigger status.
}


/*  Retrieve board's ROC firmware   */
uint32_t CAENV1720::GetROCFirmware(){
    return ReadRegister( ROC_FW );
}


/* Get the AMC firmware version for each channel */
uint32_t CAENV1720::GetChannelAMCFirmware( uint32_t i ){
    return ReadRegister( AMC_FW + i * 0x100 );
}


/* local channel setting */

void CAENV1720::SetThreshold( int i, uint32_t _th){
    uint32_t th = _th & 0x0fff;
    WriteRegister( CHN_TRIG_THRESH + i*0x100, th);
    param.channel_param[i].threshold = th;
}


uint32_t CAENV1720::GetThreshold(int i){
//    return param.channel_param[i].threshold;
    return ReadRegister(CHN_TRIG_THRESH + (i%Nchan)*0x100);
}


void CAENV1720::SetTimeCrossThreshold(int i, uint32_t n){
    uint32_t nsamp = n & 0x0fff;
    WriteRegister( CHN_NSAMP_OVER_THRESH + i*0x100, nsamp);
    param.channel_param[i].tcrossthresh = nsamp;
}


uint32_t CAENV1720::GetTimeCrossThreshold(int i){
//    return param.channel_param[i].tcrossthresh;
    return ReadRegister(CHN_NSAMP_OVER_THRESH + (i%Nchan)*0x100);
}


void CAENV1720::SetDAC(int i, uint32_t _dc){
    uint32_t dc = _dc & 0xffff;         // DAC is 16 bit.
    WriteRegister(CHN_DAC+i*0x100,dc);
    param.channel_param[i].dac = dc;
    while(!DACUpdated(i)){;}
}


uint32_t CAENV1720::GetDAC(int i){
    return ReadRegister(CHN_DAC);
}


bool CAENV1720::DACUpdated(int j){
    if(j>=0 && j<8){
        uint32_t data = ReadRegister(CHN_STATUS + j*0x100);
        return (data&0x04)==0 ? true : false;
    }
    else
        for(int i=0;i<8;i++){
            uint32_t data = ReadRegister(CHN_STATUS + i*0x100);
            if ( (data&0x04)!=0 ) return false;
        }
    return true;
}


// Update registers relevant to local channel settings according to value stored in member parameter variable.
void CAENV1720::ConfigLocalChannel(){
    for( int i=0 ; i<Nchan ; ++i){
        WriteRegister( 0x1024+i*0x100, 0x80002200);
        WriteRegister( 0x1028+i*0x100, 0x5);
        SetThreshold( i, param.channel_param[i].threshold );
        SetTimeCrossThreshold( i, param.channel_param[i].tcrossthresh );
        SetDAC( i, param.channel_param[i].dac );
    }
}


/*  global channel configuration */

void CAENV1720::ConfigChannel(){
    /*
    uint32_t pr = ReadRegister(CH_CONFIG);
    //SetBit( &pr, param.trig_overlap?1:0, 1, 1);
    SetBit( &pr, 0, 1, 1);
        // disable trigger overlap
    SetBit( &pr, 0, 3, 3);
        // no test pattern generation
    SetBit( &pr, 1, 4, 4);
        // no ram, sequential access
    SetBit( &pr, param.trig_over_threshold ? 0 : 1, 6, 6);
    SetBit( &pr, 0, 11, 11);
        // no pack2.5 ( 2 words contain 5 events in readout)
    SetBit( &pr, 0, 16, 19);
        // no zero suppression
    WriteRegister( CH_CONFIG, pr );
    */
    uint32_t pr = 0x0;
    SetBit( &pr, 1, 4, 4);   // no ram
    SetBit( &pr, param.trig_overlap?1:0, 1, 1);
    SetBit( &pr, param.trig_over_threshold?0:1, 6, 6);
    WriteRegister( CH_CONFIG, pr );
}


void CAENV1720::EnableTrigOverThresh(bool t){
    param.trig_over_threshold = t;
    ConfigChannel();
}


bool CAENV1720::TrigOverThreshEnabled(){
    uint32_t data = ReadRegister(CH_CONFIG);
    return (GetBit(data,6,6)==0)?true:false;
    //return param.trig_over_threshold;
}


void CAENV1720::EnableTrigOverlap(bool t){
    param.trig_overlap = t;
    ConfigChannel();
}


bool CAENV1720::TrigOverlapEnabled(){
    uint32_t data = ReadRegister(CH_CONFIG);
    return (GetBit(data,1,1)!=0)?true:false;
    //return param.trig_overlap = t;
}


/*  event organization */

void CAENV1720::ConfigBuffer(){
    if(Running()){
        std::cerr<<"WARNING: V1720::UpdateBuffer Cannot change buffer during run.\n";
        StopBoard();
    }
    WriteRegister( BUFFER_ORG, param.buff_code);
//    WriteRegister( POST_TRIG_SETTING, param.post_trig_sample/4 - 10);
    WriteRegister( POST_TRIG_SETTING, param.post_trig_sample/4 - 10);
        // number of post trig sample will be 4* the value of the register
    if( param.enable_custom_size )
        WriteRegister( CUSTOM_SIZE, param.pre_trig_sample/4 + param.post_trig_sample/4 );
            // event will consist of 4*Nloc samples.
            // in Pack2.5 mode, 2 locations 5 sample, so for non-Pack2.5 it should be 2 location 4 sample.
            // the register sets mem location per event, but I guess acquisition clock is 2 times faster than writing clock
    else
        WriteRegister(CUSTOM_SIZE,0x0);
            // disable custom size by writing 0.
}


void CAENV1720::SetSample(uint32_t pre, uint32_t post){
/*
    uint32_t s = 4*(pre/4);
    uint32_t t = 4*(post/4);
    if( s+t<s || s+t<t ){
        // error due to integer addition overflow.
        std::cerr<<"ERROR: Inappropriate value for pre/post trigger sample. Using 1024 for both\n";
        pre = post = 1024;
    }
    else if( s>1024*1024){
        std::cerr<<"ERROR: pre + post trigger samples too large to fit in memory. Using 1024 for both\n";
        pre = post = 1024;
    }

    param.pre_trig_sample = s;
    param.post_trig_sample = t;
        // number of samples, not values of the register
        // make sure it is dividable by 4.
    uint32_t sum = param.pre_trig_sample + param.post_trig_sample;

    // make sure the buffer is larger than pre+post samples.
    param.buff_code =  0x00;
    for( unsigned int i = 0; i<10; i++ )
        if( sum <= (1024<<i) ) param.buff_code = (0x0A - i*0x01);
    ConfigBuffer();
        // after setting pre and post, call this method to update.
*/
    param.pre_trig_sample = 4*(pre/4);
    param.post_trig_sample = 4*(post/4);
        // number of samples, not values of the register
        // make sure it is dividable by 4.
    uint32_t s = param.pre_trig_sample + param.post_trig_sample;
    param.buff_code =  0x00;
    if( s<param.pre_trig_sample || s<param.post_trig_sample )
        std::cerr<<"V1720::SetSample Pre/post trigger value too large.\n";
    else
        for( unsigned int i = 0; i<10; i++ ){
            if( s <= (unsigned int)(1024<<i) ) param.buff_code = (0x0A - i*0x01);
            break;
        }
    ConfigBuffer();
        // after setting pre and post, call this method to update.

}


uint32_t CAENV1720::GetPreSample(){
    return param.pre_trig_sample;
}


uint32_t CAENV1720::GetPostSample(){
    return param.post_trig_sample;
}


uint32_t CAENV1720::GetBufferCode(){
    return param.buff_code;
}


void CAENV1720::EnableCustomSize(bool e){
    param.enable_custom_size = e;
    ConfigBuffer();
}


uint32_t CAENV1720::GetCustomSize(){
    return ReadRegister(CUSTOM_SIZE);
}


uint32_t CAENV1720::GetNEvtStored(){
    return ReadRegister(EVT_STORED);
}


uint32_t CAENV1720::GetTotalSizeInByte(){
    return param.GetTotalSizeInByte();
}


uint32_t CAENV1720::GetTotalSizeInWord(){
    return param.GetTotalSizeInWord();
}


uint32_t CAENV1720::GetEvtSizeInSamp(){
    return param.GetEvtSizeInSamp();
}


uint32_t CAENV1720::GetEvtSizeInWord(){
    return param.GetEvtSizeInWord();
}


uint32_t CAENV1720::GetEvtSizeInByte(){
    return param.GetEvtSizeInByte();
}


void CAENV1720::EnableChannel( int i, bool e){
    if( Running() ){
        std::cerr<<"WARNING: V1720::EnableChannel Attempting to enable a channel while running."<<std::endl;
        StopBoard();
    }
    SetBit( &param.ch_enable_mask, e ? 1:0, i, i);
    EnableChannels();
}


bool CAENV1720::ChannelEnabled( int ii){
    int i = ii%Nchan;
    uint32_t data = ReadRegister(CH_ENABLE_MASK);
    return ( GetBit(data,i,i)!=0 ) ? true:false;
}


void CAENV1720::SetChannelEnableMask( uint32_t mask){
    param.ch_enable_mask = mask&0xff;
    EnableChannels();
}


void CAENV1720::EnableChannels(){
    param.ch_enable_mask &= 0xff;
    WriteRegister( CH_ENABLE_MASK, param.ch_enable_mask);
}


int CAENV1720::GetNChannelEnabled(){
    return param.GetNChannelEnabled();
}


uint32_t CAENV1720::ReadFIFO( uint32_t* bytes, uint32_t bytes_to_read){

    if(!EventReady())
        return 0;
    uint32_t trd = GetTotalSizeInByte();

    if( bytes_to_read!=trd )
        std::cerr << "Warning: V1720 ReadFIFO user attempting to read " << bytes_to_read << " while each event has " << trd << " bytes.\n";

    /************************ first read header *****************************************************/
    //
    int rd=0;                   // number of bytes read.
    CVErrorCodes err = CAENVME_FIFOBLTReadCycle( handle, param.GetBaseAddr(), (unsigned char*)bytes, bytes_to_read, cvA32_U_MBLT, cvD64, &rd);

    if( err!=cvSuccess ){
        std::cerr << "Error V1720:" << CAENVME_DecodeError( err ) << endl;
        return 0;
    }
    else if( (bytes[0] & 0xf0000000) !=  0xa0000000 ){
        std::cerr<<"ERROR: V1720 ReadFIFO event framing error. Header is 0x"<<hex<<bytes[0]<<" 0x"<<hex<<bytes[1]<<" 0x"<<hex<<bytes[2]<<" 0x"<<hex<<bytes[3]<<endl;
        return 0;
    }

    return rd;
}


/*  Acquisition control */

void CAENV1720::SetRunMode(V1720_RUNMODE rm){
    uint32_t data = 0x08;   // by default count all triggers
    SetBit( &data, rm, 0, 1);
    WriteRegister( ACQ_CON, data);
    param.runmode = rm;
}


V1720_RUNMODE CAENV1720::GetRunMode(){
    return param.runmode;
}


void CAENV1720::StartBoard(){

    while( !BoardReady() ){;}

    //std::cout<<" Starting board, available events: "<<GetNEvtStored()<<" VME status: "<<std::hex<<ReadRegister(0xEF04)<<std::endl;

    uint32_t data = ReadRegister( ACQ_CON );
    SetBit( &data, (unsigned int)(param.runmode), 0, 1 );
    WriteRegister( ACQ_CON, data );

    SetBit( &data, 1, 2, 2 );
    WriteRegister( ACQ_CON, data );

}


void CAENV1720::StartBoard( V1720_RUNMODE rm ){
    SetRunMode( rm );
    StartBoard();
}


void CAENV1720::StopBoard(){

//    uint32_t data = ReadRegister(ACQ_CON);
//    data = ReadRegister(ACQ_CON);
//    SetBit(&data,0,2,2);
    WriteRegister( ACQ_CON, 0);

}


bool CAENV1720::Running(){
    return GetBit( ReadRegister(ACQ_STATUS), 2, 2 )==1 ? true:false;
}


bool CAENV1720::BoardReady(){
    return GetBit( ReadRegister(ACQ_STATUS), 8, 8 )==1 ? true:false;
}


bool CAENV1720::EventReady(){
    return GetBit(ReadRegister(ACQ_STATUS), 3, 3 )==1 ? true:false;
}


bool CAENV1720::EventFull(){
    return GetBit(ReadRegister(ACQ_STATUS), 4, 4 )==1 ? true:false;
}


void CAENV1720::SetEvtNumberBLT( uint32_t s ){
    WriteRegister( BLT_EVT_NUM, s );
}


/* reset/reinitialization */

void CAENV1720::Reset(){
    uint32_t d = 0x01;  WriteRegister( SOFT_RESET, d);
    while(!BoardReady()){;}
}


void CAENV1720::SWClear(){
    uint32_t d = 0x01;  WriteRegister( SOFT_CLEAR, d);
    while(!BoardReady()){;}
}


/* acquisition trigger source */

void CAENV1720::SWTrigger(){
    WriteRegister( SOFT_TRIGGER, 0x1);
}


void CAENV1720::ConfigTrigSource(){
    uint32_t data = (param.local_trig_enable) & 0xff;

    SetBit(&data, param.sw_trig_enable?1:0, 31, 31);
    SetBit(&data, param.ext_trig_enable?1:0, 30, 30);

    SetBit(&data, param.coin_level & 0x7, 24, 26);
    SetBit(&data, param.coin_window & 0xf, 20, 23);

    WriteRegister(TRIG_SRC_MASK, data);
}


void CAENV1720::EnableSoftTrig(bool e){
    param.sw_trig_enable = e;
    ConfigTrigSource();
}


void CAENV1720::EnableExtTrig(bool e){
    param.ext_trig_enable = e;
    ConfigTrigSource();
}


void CAENV1720::EnableLocalTrig(int ii,bool e){
    int i = ii%Nchan;
    uint32_t r = e?1:0;
    SetBit( &param.local_trig_enable, r, i, i);
    ConfigTrigSource();
}


void CAENV1720::SetTrigSrcMask(uint32_t mask){
    param.sw_trig_enable = (mask&0x8000)!=0?true:false;
    param.ext_trig_enable = (mask&0x4000)!=0?true:false;
    param.local_trig_enable = mask & 0xff;
    WriteRegister(TRIG_SRC_MASK,mask);
}


bool CAENV1720::SoftTrigEnabled(){
    uint32_t data = ReadRegister(TRIG_SRC_MASK);
    return GetBit(data,31,31);
}


bool CAENV1720::ExtTrigEnabled(){
    uint32_t data = ReadRegister(TRIG_SRC_MASK);
    return GetBit(data,30,30);
}


bool CAENV1720::LocalTrigEnabled(int ii){
    int i = ii%Nchan;
    uint32_t data = ReadRegister(TRIG_SRC_MASK);
    return GetBit(data,i,i);
}


/*  Front panel trigger output*/

void CAENV1720::ConfigFPTrigOut(){
    uint32_t data = param.local_fp_trigout & 0xff;
    SetBit(&data, param.sw_fp_trigout ? 1 : 0, 31, 31);
    SetBit(&data, param.ext_fp_trigout ? 1 : 0, 30, 30);
    WriteRegister(FRONT_PANEL_TRIGOUT, data);
}


void CAENV1720::EnableFPSWTrigOut(bool e){
    param.sw_fp_trigout = e;
    ConfigFPTrigOut();
}


void CAENV1720::EnableFPExtTrigOut(bool e){
    param.ext_fp_trigout = e;
    ConfigFPTrigOut();
}


void CAENV1720::EnableFPLocalTrigOut(int ii, bool e){
    int i = ii%Nchan;
    SetBit(&param.local_fp_trigout, e?1:0, i, i);
    ConfigFPTrigOut();
}


bool CAENV1720::FPSWTrigOutEnabled(){
    uint32_t data = ReadRegister(FRONT_PANEL_TRIGOUT);
    return (GetBit(data,31,31)!=0)?true:false;
}


bool CAENV1720::FPExtTrigOutEnabled(){
    uint32_t data = ReadRegister(FRONT_PANEL_TRIGOUT);
    return (GetBit(data,30,30)!=0)?true:false;
}


bool CAENV1720::FPLocalTrigOutEnabled(int ii){
    int i = ii%Nchan;
    uint32_t data = ReadRegister(FRONT_PANEL_TRIGOUT);
    return (GetBit(data,i,i)!=0)?true:false;
}


void CAENV1720::SetFPTrigOutMask(uint32_t mask){
    param.sw_fp_trigout = (mask&0x8000)!=0?true:false;
    param.ext_fp_trigout = (mask&0x4000)!=0?true:false;
    param.local_fp_trigout = mask & 0xff;
    WriteRegister(FRONT_PANEL_TRIGOUT,mask);
}


/*  Front panel IO control */

void CAENV1720::ConfigFPIO(){
    uint32_t data = 0x0;
    SetBit(&data, param.logic_level_ttl?1:0, 0, 0);
    SetBit(&data, param.lvds_io_output?0xf:0x0, 2, 5);
    SetBit(&data, 1, 8, 8);
        // enable new mode
    
    // enable extended trigger timetag by setting bit 22;21 to 10
    // set to 01 for outputting trigger source in the event field
    SetBit(&data, 2, 21, 22);

    //cout << "Writing " << std::hex << FRONT_PANEL_IOCON << " with " << data << endl;

    WriteRegister(FRONT_PANEL_IOCON, data);
}


void CAENV1720::SetLogicTTL( bool ttl){
    param.logic_level_ttl = ttl;
    ConfigFPIO();
}


bool CAENV1720::GetLogicTTL(){
    uint32_t data = ReadRegister(FRONT_PANEL_IOCON);
    return GetBit(data,0,0);
}


void CAENV1720::SetLVDSDirection(bool output){
    param.lvds_io_output = output;
    ConfigFPIO();
}


bool CAENV1720::GetLVDSDirection(){
    uint32_t data = ReadRegister(FRONT_PANEL_IOCON);
    return GetBit(data,2,5)==0xf;
}


