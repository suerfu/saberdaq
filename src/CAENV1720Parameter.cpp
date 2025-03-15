#include <iostream>
#include <cstring>
#include <cmath>

#include "CAENV1720Parameter.h"

using namespace std;

/*
CAENV1720ChannelParameter::CAENV1720ChannelParameter(){
    board_id = 0;
    channel_id = 8;
    threshold = 0;
    tcrossthresh = 0;
    dac = 0;
    descriptor = 0;
    label = "";
    name = "";
    pre_trig_sample = 0;
    post_trig_sample = 0;
}


CAENV1720ChannelParameter::~CAENV1720ChannelParameter(){}

CAENV1720ChannelParameter& CAENV1720ChannelParameter::operator= (const CAENV1720ChannelParameter& rhs){
    board_id = rhs.board_id;
    channel_id = rhs.channel_id;
    threshold = rhs.threshold;
    tcrossthresh = rhs.tcrossthresh;
    dac = rhs.dac;
    descriptor = rhs.descriptor;
    label = rhs.label;
    name = rhs.name;
    pre_trig_sample = rhs.pre_trig_sample;
    post_trig_sample = rhs.post_trig_sample;

    return *this;
}
*/


CAENV1720Parameter::CAENV1720Parameter() : VMEBoardParameter(){

    // Sets default values.
    link_number = 0;
    board_number = -1;
    base_addr = 0x32110000;


    /* event organization */
    pre_trig_sample = 1024;
    post_trig_sample = 1024;
    enable_custom_size = false;

    buff_code =  0x00;
    uint32_t s = pre_trig_sample + post_trig_sample;
    for( unsigned int i = 0; i< 10; i++ ){
        if( s <= (uint32_t(1024)<<i) )
            buff_code = (0x0A - i*0x01);
        break;
    }

    /* local channel setting */
    for(int i=0;i<8;i++){
        channel_param[i].threshold = 2046;
        channel_param[i].tcrossthresh = 0x4;
        channel_param[i].dac = 0xfd66;
        channel_param[i].descriptor = 0;
        channel_param[i].label = "";
        channel_param[i].name = "";
    }

    trig_overlap = false;
    trig_over_threshold = true;

    ch_enable_mask = 0x0;       // by default no channel enabled

    local_trig_enable = 0x0;    // local trigger disable
    coin_level = 0;
    coin_window = 0;
    
    sw_trig_enable = false;
    ext_trig_enable = false;    // external trig and sw trig disabled

    /* acquisition control */
    runmode = REG_CON;

    /* front panel and trigger outpout */
    local_fp_trigout = 0x0;
    sw_fp_trigout = false;
    ext_fp_trigout = false;

    logic_level_ttl = true; // TTL and LVDS output by default.
    lvds_io_output = true;
}



CAENV1720Parameter::~CAENV1720Parameter(){;}



string CAENV1720Parameter::GetPrintString(){

    stringstream ss;
    ss << "\n\tV1720 parameters:\n\t";
    
    for( int i=0; i<8; ++i){
        ss << "Channel " << i << "\n\t";
        ss << "\tEnabled       : " << ( (ch_enable_mask & (0x1<<i)) != 0 ) << "\n\t";
        ss << "\tLocal trigger : " << ( (local_trig_enable & (0x1<<i)) != 0 ) << "\n\t";
        ss << "\tLocal trig FP : " << ( (local_fp_trigout & (0x1<<i)) != 0 ) << "\n\t";
        ss << "\tThreshold     : " << channel_param[i].threshold << "\n\t";
        ss << "\tTime x thresh : " << channel_param[i].tcrossthresh << "\n\t";
        ss << "\tDAC           : " << channel_param[i].dac << "\n\t";
        ss << "\tDescriptor    : " << channel_param[i].descriptor << "\n\t";
        ss << "\tLabel         : " << channel_param[i].label << "\n\t";
        ss << "\tName          : " << channel_param[i].name << "\n\t";
    }

    ss << "\n\t";
    ss << "Trigger overlap    : " << ( trig_overlap ) << "\n\t";
    ss << "Trigger overthresh : " << ( trig_over_threshold ) << "\n\t";
    ss << "Software trigger   : " << ( sw_trig_enable ) << "\n\t";
    ss << "External trigger   : " << ( ext_trig_enable ) << "\n\t";
    ss << "FP sw trigger out  : " << ( sw_fp_trigout ) << "\n\t";
    ss << "FP ext trigger out : " << ( ext_fp_trigout ) << "\n\t";

    ss << "\n\t";
    ss << "Logic level        : " << (logic_level_ttl ? "TTL" : "NIM") << "\n\t";
    ss << "LVDS IO direction  : " << (lvds_io_output ? "OUT" : "IN");

    ss << "\n\t";
    ss << "Custom size        : " << ( enable_custom_size ) << "\n\t";
    ss << "Pre trigger sample : " << dec << pre_trig_sample << "\n\t";
    ss << "Post trigger sample: " << dec << post_trig_sample << "\n\t";
    ss << "Buffer code        : " << hex << buff_code << "\n\t";
    ss << "Samples per event  : " << dec <<GetEvtSizeInSamp() << "\n";
    ss << "\n";

    return ss.str();
}



void CAENV1720Parameter::SetParamFromConfig( ConfigParser* p, string dir){

    // if didn't find the required directory, return default settings.
    if( !p->Find(dir) && !p->Find("/module/daq/board*/") ){
        p->Print( " In the config file did not find parameters for V1720. Using defaults.\n", ERR);
        return ;
    }

    // dir should be specified as /dir/
    else if( (*dir.rbegin())!='/' ){
        cerr << "ERROR: "<<dir<<" is in wrong format. Directory ends with /.\n";
        return ;
    }

    SetLinkNumber( p->GetInt( dir+"link_number", 0));
    SetBoardNumber( p->GetInt( dir+"board_number", 0));
    SetBaseAddr( p->GetInt( dir+"address", 0x32110000) );


    /***********************************************************************************/
    /* local channel settings, threshold, DAC, enable mask, local trig, fp out.        */
    /***********************************************************************************/

    string wildboard = "/module/daq/board*/";
    string wbdwch = wildboard+"channel*/";
    string wildch = dir+"channel*/";
        // channel wildcard. Settings apply to all channels not explicitly specified.

    // first apply channel global settings.
    //
    // enable trigger mask, false by default.
    ch_enable_mask = 0x0;
    local_trig_enable = 0x0;
    local_fp_trigout = 0x0;

    // if at global level enable is found, all bits are enabled.
    if( p->Find( wbdwch+"enable") )
        ch_enable_mask = p->GetBool( wbdwch+"enable", false) ? 0xff : 0;
    if( p->Find( wildch+"enable") )
        ch_enable_mask = p->GetBool( wildch+"enable", false) ? 0xff : 0;

    if( p->Find( wbdwch+"local_trigger_enable") )
        local_trig_enable = p->GetBool( wbdwch+"local_trigger_enable", false ) ? 0xff : 0;
    if( p->Find( wildch+"local_trigger_enable") )
        local_trig_enable = p->GetBool( wildch+"local_trigger_enable", false ) ? 0xff : 0;


    coin_level = p->GetInt( wbdwch+"coin_level", coin_level );
    coin_level = p->GetInt( dir+"coin_level", coin_level );

    coin_window = p->GetInt( wbdwch+"coin_window", coin_window );
    coin_window = p->GetInt( dir+"coin_window", coin_window );

    if ( p->Find( wbdwch+"fp_local_trigger_out_enable" ))
        local_fp_trigout = p->GetBool( wbdwch+"fp_local_trigger_out_enable", false ) ? 0xff : 0;
    if ( p->Find( wildch+"fp_local_trigger_out_enable" ))
        local_fp_trigout = p->GetBool( wildch+"fp_local_trigger_out_enable", false ) ? 0xff : 0;


    // settings to each local channel
    

    for( int i=0; i<8; ++i){
        stringstream ss;
        ss << dir << "channel" << i << "/";
        string ch = ss.str();

        if( p->Find( ch+"enable") ){ // only when the channel is enabled.
            if( p->GetBool( ch+"enable", false) )
                ch_enable_mask |= (0x1<<i);
            else
                ch_enable_mask &= ~(0x1<<i);
        }

        // first check global settings, then check individual channel to overwrite changes.
        channel_param[i].threshold = p->GetInt(wbdwch+"threshold", channel_param[i].threshold);
        channel_param[i].threshold = p->GetInt(wildch+"threshold", channel_param[i].threshold);
        channel_param[i].threshold = p->GetInt(ch+"threshold", channel_param[i].threshold);

        channel_param[i].tcrossthresh = p->GetInt( wbdwch+"time_cross_threshold", channel_param[i].tcrossthresh);
        channel_param[i].tcrossthresh = p->GetInt( wildch+"time_cross_threshold", channel_param[i].tcrossthresh);
        channel_param[i].tcrossthresh = p->GetInt( ch+"time_cross_threshold", channel_param[i].tcrossthresh);

        channel_param[i].dac = p->GetInt( wbdwch+"DAC", channel_param[i].dac);
        channel_param[i].dac = p->GetInt( wildch+"DAC", channel_param[i].dac);
        channel_param[i].dac = p->GetInt( ch+"DAC", channel_param[i].dac);

        channel_param[i].descriptor = p->GetInt( wbdwch+"descriptor", channel_param[i].descriptor);
        channel_param[i].descriptor = p->GetInt( wildch+"descriptor", channel_param[i].descriptor);
        channel_param[i].descriptor = p->GetInt( ch+"descriptor", channel_param[i].descriptor);

        if( p->GetString( ch+"label") != "" )
            channel_param[i].label = p->GetString( ch+"label" );
        else if( p->GetString( wildch+"label") != "" )
            channel_param[i].label = p->GetString( wildch+"label" );
        else if( p->GetString( wbdwch+"label") != "" )
            channel_param[i].label = p->GetString( wbdwch+"label" );

        
        if( p->GetString( ch+"name") != "" )
            channel_param[i].name = p->GetString( ch+"name" );
        else if( p->GetString( wildch+"name") != "" )
            channel_param[i].name = p->GetString( wildch+"name" );
        else if( p->GetString( wbdwch+"name") != "" )
            channel_param[i].name = p->GetString( wbdwch+"name" );

        
        // local channel as trigger source
        if( p->Find( ch+"local_trigger_enable")){
            if( p->GetBool( ch+"local_trigger_enable", false))
                local_trig_enable |= (0x1<<i);
            else
                local_trig_enable &= (~(0x1<<i));
        }
        //
        // local trigger front panel output
        if( p->Find( ch+"fp_local_trigger_out_enable")){
            if( p->GetBool( ch+"fp_local_trigger_out_enable", false))
                local_fp_trigout |= (0x1<<i);
            else
                local_fp_trigout &= (~(0x1<<i));
        }
    }


    /* trigger over threshold and trigger overlap */
    trig_over_threshold = p->GetBool( wildboard+"trigger_over_threshold", trig_over_threshold);
    trig_over_threshold = p->GetBool( dir+"trigger_over_threshold", trig_over_threshold);

    trig_overlap = p->GetBool( wildboard+"trigger_overlap", trig_overlap);
    trig_overlap = p->GetBool( dir+"trigger_overlap", trig_overlap);


    /* software trigger and external trigger */
    sw_trig_enable = p->GetBool( wildboard+"software_trigger_enable", sw_trig_enable);
    sw_trig_enable = p->GetBool( dir+"software_trigger_enable", sw_trig_enable);
    
    ext_trig_enable = p->GetBool( wildboard+"external_trigger_enable", ext_trig_enable);
    ext_trig_enable = p->GetBool( dir+"external_trigger_enable", ext_trig_enable);


    /* software and external front-panel trigger output */
    ext_fp_trigout = p->GetBool( wildboard+"fp_external_trigger_out_enable", ext_fp_trigout);
    ext_fp_trigout = p->GetBool( dir+"fp_external_trigger_out_enable", ext_fp_trigout);

    sw_fp_trigout = p->GetBool( wildboard+"fp_software_trigger_out_enable", sw_fp_trigout);
    sw_fp_trigout = p->GetBool( dir+"fp_software_trigger_out_enable", sw_fp_trigout);


    /* front panel logic type and LVDS IO direction*/
    logic_level_ttl = p->GetBool( wildboard+"logic_TTL", logic_level_ttl);
    logic_level_ttl = p->GetBool( dir+"logic_TTL", logic_level_ttl);

    lvds_io_output = p->GetBool( wildboard+"LVDS_IO_out", lvds_io_output);
    lvds_io_output = p->GetBool( dir+"LVDS_IO_out", lvds_io_output);


    /* acquisition control */
    string mode = "register_controlled";
    if( p->Find( dir+"run_mode") )
        mode = p->GetString( dir+"run_mode");
    else if( p->Find( wildboard+"run_mode") )
        mode = p->GetString( wildboard+"run_mode");

    if(mode=="first_trigger_controlled")
        runmode = FIRST_TRIG_CON;
    else
        runmode = REG_CON;


    /* customer size enable/disable */
    enable_custom_size = p->GetBool( wildboard+"custom_size_enable", enable_custom_size);

    if( p->Find( dir+"custom_size_enable") ){
        enable_custom_size = p->GetBool( dir+"custom_size_enable", enable_custom_size);
        //cerr << "  custom size is "<<enable_custom_size << endl;
    }

    /* event size */
    float pre(0), post(0);
    pre = p->GetFloat( wildboard+"pre_trigger_window_us", pre);
    pre = p->GetFloat( dir+"pre_trigger_window_us", pre);
    post = p->GetFloat( wildboard+"post_trigger_window_us", post);
    post = p->GetFloat( dir+"post_trigger_window_us", post);


    pre_trig_sample = 4*(int(pre*250)/4);
    post_trig_sample = 4*(int(post*250)/4);

    // somehow below settings give bus error upon readout
    //pre_trig_sample = (4*int( round( pre*250 ) ))/4;
    //post_trig_sample = (4*int( round( post*250) ))/4;

    // manually set buffer code here
    buff_code =  0x00;
    uint32_t sum = pre_trig_sample + post_trig_sample;

    uint32_t one_buffer_size = 1024;

    if( sum <= 1024){
        buff_code = 0x0a;
    }
    else if( sum<= (1024<<1)){
        buff_code = 0x09;
        one_buffer_size *= (1<<1);
    }
    else if( sum<= (1024<<2)){
        buff_code = 0x08;
        one_buffer_size *= (1<<2);
    }
    else if( sum<= (1024<<3)){
        buff_code = 0x07;
        one_buffer_size *= (1<<3);
    }
    else if( sum<= (1024<<4)){
        buff_code = 0x06;
        one_buffer_size *= (1<<4);
    }
    else if( sum<= (1024<<5)){
        buff_code = 0x05;
        one_buffer_size *= (1<<5);
    }
    else if( sum<= (1024<<6)){
        buff_code = 0x04;
        one_buffer_size *= (1<<6);
    }
    else if( sum<= (1024<<7)){
        buff_code = 0x03;
        one_buffer_size *= (1<<7);
    }
    else if( sum<= (1024<<8)){
        buff_code = 0x02;
        one_buffer_size *= (1<<8);
    }
    else if( sum<= (1024<<9)){
        buff_code = 0x01;
        one_buffer_size *= (1<<9);
    }
    else{
        buff_code = 0x0;
        one_buffer_size *= (1<<10);
    }

    if( !enable_custom_size ){
        post_trig_sample = one_buffer_size - pre_trig_sample;
    }

    for( int i=0; i<NCHANNEL; i++){
        channel_param[i].pre_trig_sample = pre_trig_sample;
        channel_param[i].post_trig_sample = post_trig_sample;
    }

    return;
}



unsigned int CAENV1720Parameter::GetNChannelEnabled(){
    int c = 0;  uint32_t val = ch_enable_mask;
    for(int i=0;i<8;i++){
        if( (val&0x1)==0x1 ) c++;
        val = (val>>1);
    }
    return c;
}


uint32_t CAENV1720Parameter::GetTotalSizeInWord(){
    return GetEvtSizeInSamp()/2+4;
}


uint32_t CAENV1720Parameter::GetTotalSizeInByte(){
    return GetTotalSizeInWord()*4;
}


uint32_t CAENV1720Parameter::GetEvtSizeInSamp(){
    int n = GetNChannelEnabled();
    if(enable_custom_size)
        return n * (pre_trig_sample + post_trig_sample);
    else
        return n* (1024*1024)/(0x1<<buff_code);
}


uint32_t CAENV1720Parameter::GetEvtSizeInWord(){
    return GetEvtSizeInSamp()/2;
}


uint32_t CAENV1720Parameter::GetEvtSizeInByte(){
    return GetEvtSizeInWord()*4;
}


uint32_t CAENV1720Parameter::GetChanSizeInSamp(){
    return GetEvtSizeInSamp()/GetNChannelEnabled();
}


uint32_t CAENV1720Parameter::GetChanSizeInWord(){
    return GetChanSizeInSamp()/2;
}


uint32_t CAENV1720Parameter::GetChanSizeInByte(){
    return GetChanSizeInWord()*4;
}


unsigned int CAENV1720Parameter::GetHeaderSize(){
    int bytes = 0;

    bytes += 5 + 4*8 + 17 + 1;
        // channelwise parameters + other registers + header/version...

    for( int i=0; i<8; i++){
        bytes += 1 + channel_param[i].label.length();
        bytes += 1 + channel_param[i].name.length();
    }

    return bytes;
}


void CAENV1720Parameter::Serialize( char* p){

    vector<uint32_t> data;

    data.push_back( 0xad1234ad );
        // begin of ADC header
    data.push_back( sizeof( base_addr) * GetHeaderSize());
        // size of header in bytes, including current word and last header, but excluding first header.
    data.push_back( GetVersion() );
        // version
    data.push_back( GetModel() );


    data.push_back( base_addr);

    for( int i=0; i<8; ++i){
        data.push_back( channel_param[i].threshold );
        data.push_back( channel_param[i].tcrossthresh );
        data.push_back( channel_param[i].dac );
        data.push_back( channel_param[i].descriptor );
        data.push_back( channel_param[i].label.length() );
        for( unsigned int j=0; j<channel_param[i].label.length(); ++j ){
            uint32_t c = channel_param[i].label[j];
            data.push_back( c );
        }
        data.push_back( channel_param[i].name.length() );
        for( unsigned int j=0; j<channel_param[i].name.length(); ++j ){
            uint32_t c = channel_param[i].name[j];
            data.push_back( c );
        }
    }

    data.push_back( ch_enable_mask );
    data.push_back( trig_over_threshold );
    data.push_back( trig_overlap );
    data.push_back( enable_custom_size );
    data.push_back( pre_trig_sample );
    data.push_back( post_trig_sample );
    data.push_back( buff_code );
    data.push_back( GetEvtSizeInSamp() );
    data.push_back( runmode );
    data.push_back( lvds_io_output );
    data.push_back( logic_level_ttl );
    data.push_back( sw_trig_enable );
    data.push_back( sw_fp_trigout );
    data.push_back( ext_trig_enable );
    data.push_back( ext_fp_trigout );
    data.push_back( local_trig_enable );
    data.push_back( local_fp_trigout );
    data.push_back( 0xad1234ad );

    int bytes_copied = 0;
    for( unsigned int i=0; i<data.size(); ++i){
        memcpy( p+bytes_copied, &data[i], sizeof( uint32_t) );
        bytes_copied += sizeof(uint32_t);       
    }
}

void CAENV1720Parameter::Deserialize( ifstream& file ){

    uint32_t header;
    uint32_t size = 0; //sizeof(uint32_t)*GetHeaderSize();
        // since contains variable length string, needs to read it

    file.read( reinterpret_cast<char*>( &header ), sizeof( header ) );
    file.read( reinterpret_cast<char*>( &size ), sizeof( size ) );
    file.seekg( -2*sizeof(uint32_t), ios_base::cur);

    char* buffer = new char[size];
    file.read( buffer, size);

    Deserialize( buffer );

    delete [] buffer;
}


void CAENV1720Parameter::Deserialize( char* p, bool flip){

    int offset = 0;
    vector<uint32_t> data;
    uint32_t temp = 0;

    uint32_t header = 0;
    memcpy( &header, p, sizeof( uint32_t) );
    if( header!= 0xad1234ad)
        return;
    data.push_back(header);

    uint32_t size = 0;
    memcpy( &size, p+1*sizeof(header), sizeof( uint32_t) );
    data.push_back(size);

    for( unsigned int i=2; i<size/4; ++i){
        memcpy( &temp, p+i*sizeof(uint32_t), sizeof( uint32_t) );
        data.push_back(temp);
    }
    // copy everything into a vector of uint32_t

    uint32_t version = data[2];
    offset = 3;

    if( version>1 )
        offset++;


    base_addr = data[offset++];

    for( int i=0; i<8; ++i){
        channel_param[i].threshold = data[offset++];
        channel_param[i].tcrossthresh = data[offset++];
        channel_param[i].dac = data[offset++];
        channel_param[i].descriptor = data[offset++];
        if( version>=1 ){
            uint32_t len = data[offset++];
            channel_param[i].label = "";
            for( unsigned int j=0; j<len; j++){
                uint32_t c = data[offset++];
                channel_param[i].label.push_back( static_cast<char>(c) );
            }
        }
        if( version>=2 ){
            uint32_t len = data[offset++];
            channel_param[i].name = "";
            for( unsigned int j=0; j<len; j++){
                uint32_t c = data[offset++];
                channel_param[i].name.push_back( static_cast<char>(c) );
            }
        }
    }

    ch_enable_mask = data[offset++];
    trig_over_threshold = data[offset++];
    trig_overlap = data[offset++];
    enable_custom_size = data[offset++];
    pre_trig_sample = data[offset++];
    post_trig_sample = data[offset++];
    buff_code = data[offset++];
    offset++;
        // skip direct setting of event size.
    runmode = static_cast<V1720_RUNMODE>( data[offset++]);
    lvds_io_output = data[offset++];
    logic_level_ttl = data[offset++];
    sw_trig_enable = data[offset++];
    sw_fp_trigout = data[offset++];
    ext_trig_enable = data[offset++];
    ext_fp_trigout = data[offset++];
    local_trig_enable = data[offset++];
    local_fp_trigout = data[offset++];
}


void CAENV1720Parameter::ExportHDF5( H5FileManager* h5man){

    if( h5man->IsFileOpen()==false ){
        return;
    }

    stringstream ss;
    ss << "adc_" << GetBoardIndex();
    string board_name = ss.str();

    h5man->OpenGroup( "/"+board_name );

    h5man->AddAttribute( "/"+board_name, "board_index", GetBoardIndex() );
    h5man->AddAttribute( "/"+board_name, "fw_version", string("2.0.0") );
    h5man->AddAttribute( "/"+board_name, "model", string("CAEN_V1720") );
    h5man->AddAttribute( "/"+board_name, "sampling_rate", float(250e6) );
    
    h5man->AddAttribute( "/"+board_name, "vme_address", GetBaseAddr() );

    h5man->AddAttribute( "/"+board_name, "channel_mask",  ch_enable_mask );
    h5man->AddAttribute( "/"+board_name, "nb_channels", GetNChannelEnabled() );
    h5man->AddAttribute( "/"+board_name, "nb_samples", GetEvtSizeInSamp()/GetNChannelEnabled() );
    
    h5man->AddAttribute( "/"+board_name, "nb_pre_trigger_sample", pre_trig_sample );
    h5man->AddAttribute( "/"+board_name, "nb_post_trigger_sample", post_trig_sample );
    
    if( runmode==FIRST_TRIG_CON ){
        h5man->AddAttribute( "/"+board_name, "run_mode", string("first_trigger_controlled") );
    }
    else{
        h5man->AddAttribute( "/"+board_name, "run_mode", string("register_controlled") );
    }

    h5man->AddAttribute( "/"+board_name, "buffer_code", buff_code );
    
    h5man->AddAttribute( "/"+board_name, "trigger_ext_enable", (unsigned int)(ext_trig_enable) );
    h5man->AddAttribute( "/"+board_name, "trigger_sw_enable", (unsigned int)(sw_trig_enable) );

    h5man->AddAttribute( "/"+board_name, "trigger_ext_fp_out",  (unsigned int)(ext_fp_trigout) );
    h5man->AddAttribute( "/"+board_name, "trigger_sw_fp_out",  (unsigned int)(sw_fp_trigout) );
    
    h5man->AddAttribute( "/"+board_name, "trigger_loc_enable", local_trig_enable );
    h5man->AddAttribute( "/"+board_name, "trigger_loc_fp_out",  local_fp_trigout );
    
    h5man->AddAttribute( "/"+board_name, "trigger_polarity", (unsigned int)(trig_over_threshold) );
    h5man->AddAttribute( "/"+board_name, "trigger_overlap", (unsigned int)(trig_overlap) );
    
    h5man->AddAttribute( "/"+board_name, "logic_TTL", (unsigned int)(logic_level_ttl) );
    h5man->AddAttribute( "/"+board_name, "io_LVDS",  (unsigned int)(lvds_io_output) );
    
    h5man->AddAttribute( "/"+board_name, "trigger_coin_level", coin_level );
    h5man->AddAttribute( "/"+board_name, "trigger_coin_window", coin_window );
    
     
    vector<CAENV1720ChannelParameter> channels = GetEnabledChannels();

    vector<unsigned int> chan_enable;
        // whether the channel is enabled or not
    vector<unsigned int> chan_indices;
        // the index in the data set, skipping the unenabled channels
    vector<string> chan_label;
        // label that represents what the channel is measuring
    vector<unsigned int> chan_DAC;
        // DAC offset value
    vector<unsigned int> chan_local_trig_enable;
        // whether local channel is enabled or not
    vector<unsigned int> chan_local_fp_trigout;
        // whether front panel trigger output upon local trigger
    vector<unsigned int> chan_threshold;
        // local threshold value
    vector<unsigned int> chan_tcross_threshold;
        // time cross threshold

    unsigned int channel_order = 0;

    for ( unsigned int j=0; j<8; j++){

        if ( ChannelNEnabled( j )==false ){
            continue;
        }

        CAENV1720ChannelParameter channel = channels[channel_order];
        channel_order++;

        chan_indices.push_back( j );

        if( channel.label!="" )
            chan_label.push_back( channel.label );
        else{
            stringstream ss;
            ss << "channel_" << j;
            chan_label.push_back( ss.str() );
        }

        chan_DAC.push_back( channel.dac );
        chan_threshold.push_back( channel.threshold );
        chan_local_trig_enable.push_back( uint32_t((local_trig_enable & (0x1<<j))!=0 ));
        chan_local_fp_trigout.push_back(  uint32_t( (local_fp_trigout & (0x1<<j))!=0 ));
        chan_tcross_threshold.push_back(  uint32_t( channel.tcrossthresh) );
    }

    h5man->AddAttribute( "/"+board_name, "channel_index", chan_indices );
    h5man->AddAttribute( "/"+board_name, "channel_label", chan_label );
    h5man->AddAttribute( "/"+board_name, "channel_DAC", chan_DAC );
    h5man->AddAttribute( "/"+board_name, "channel_threshold", chan_threshold );
    h5man->AddAttribute( "/"+board_name, "channel_tx_threshold", chan_tcross_threshold );
    h5man->AddAttribute( "/"+board_name, "channel_trigger_loc_enable", chan_local_trig_enable );
    h5man->AddAttribute( "/"+board_name, "channel_trigger_loc_fp_out",  chan_local_fp_trigout );
}
