
#include "CAENV1495Parameter.h"

#include <cstring>

using namespace std;


CAENV1495Parameter::CAENV1495Parameter() : VMEBoardParameter(){
    // set default values.

    enable = false;

    link_number = 0;
    board_number = 0;
    base_addr = 0x10000000;

    output_mode = CRYSTAL;

    retrig_xystal = false;
    retrig_veto = false;

    veto_mask = 0x3ff;
    maj_lev = 3;
    sig_delay = 0x2;
    trig_time = 0x1;

    gate_len_xystal = 0x0003;
    gate_len_veto = 0x0003;

    dead_time = 50;
    veto_time = 50;
}


CAENV1495Parameter::~CAENV1495Parameter(){;}


string CAENV1495Parameter::GetPrintString(){

    stringstream ss;
    ss << "\n\n\tV1495 parameters:\n\t";
    ss << "Trigger mode : ";
    switch( output_mode ){
        case CRYSTAL:
            ss << "crystal\n\t";
            break;
        case LSCINTILLATOR:
            ss << "pc\n\t";
            break;
        case VETO:
            ss << "veto\n\t";
            break;
        case CALIBRATION:
            ss << "calibration\n\t";
            break;
        default:
            break;
    }
    
    ss << "Crystal re-trigger : " << retrig_xystal << "\n\t";
    ss << "pc re-trigger      : " << retrig_veto << "\n\t";
    ss << "Veto mask          : " << hex << veto_mask << "\n\t";
    ss << "PC Majority. level : " << dec << maj_lev << "\n\t";
    ss << "Signal delay       : " << dec << sig_delay << "\n\t";
    ss << "Trigger duration   : " << dec << trig_time << "\n\t";
    ss << "Crystal coin window: " << dec << gate_len_xystal << "\n\t";
    ss << "PC coin window     : " << dec << gate_len_veto << "\n\t";
    ss << "Dead time          : " << dec << dead_time << "\n\t";
    ss << "Veto window        : " << dec << veto_time << "\n";

    ss << "\n";

    return ss.str();
}


void CAENV1495Parameter::SetParamFromConfig( ConfigParser* p, string dir){

    if(!p->Find(dir)){
        PushMessage( "Cannot find trigger directory "+dir+".\n" );
        status = -1;
        return ;
    }

    SetLinkNumber( p->GetInt( dir+"link_number", link_number ) );
    SetBoardNumber( p->GetInt( dir+"board_number", board_number ) );
    SetBaseAddr( p->GetInt( dir+"address", base_addr ) );

    enable = p->GetBool( dir+"enable", enable );

    string s,t;
    s = p->GetString( dir+"trigger_mode", "crystal" );
    if(s=="crystal")    output_mode = CRYSTAL;
    else if(s=="veto")  output_mode = VETO;
    else if(s=="pc")    output_mode = LSCINTILLATOR;
    else if(s=="calibration")   output_mode = CALIBRATION;
    else{
        PushMessage( "Invalid trigger mode specified.\n");
        status = -1;
        return ;
    }

    retrig_xystal = p->GetBool( dir+"enable_crystal_retrigger", retrig_xystal );
    retrig_veto = p->GetBool( dir+"enable_pc_retrigger", retrig_veto );

    veto_mask = p->GetInt( dir+"veto_mask", veto_mask );
    maj_lev = p->GetInt( dir+"majority_level", maj_lev );
    sig_delay = p->GetInt( dir+"signal_delay", sig_delay );
    trig_time = p->GetInt( dir+"trigger_time", trig_time );

    gate_len_xystal = p->GetInt( dir+"crystal_gate_length", gate_len_xystal );
    gate_len_veto = p->GetInt( dir+"pc_gate_length", gate_len_veto );

    dead_time = p->GetInt( dir+"dead_time", dead_time );
    veto_time = p->GetInt( dir+"veto_time", veto_time );

    return ;
}


// Get number of words in event header, including everything.
unsigned int CAENV1495Parameter::GetHeaderSize(){
    return 16;
}


void CAENV1495Parameter::Serialize( char* p ){

    int bytes_copied = 0;

    vector<uint32_t> data;

    data.push_back( 0xae1234ae);
        // trigger header
    data.push_back( sizeof( base_addr) * GetHeaderSize() );
        // number of bytes to follow, including this word
    data.push_back( GetVersion() );
        // version number

    data.push_back( base_addr);
        // base address
    data.push_back( output_mode);
        //
    data.push_back( retrig_xystal);
        //
    data.push_back( retrig_veto);
        //
    data.push_back( veto_mask);
        //
    data.push_back( maj_lev);
        //
    data.push_back( sig_delay);
        //
    data.push_back( trig_time);
        //
    data.push_back( gate_len_xystal);
        //
    data.push_back( gate_len_veto);
        //
    data.push_back( dead_time);
        //
    data.push_back( veto_time);
        //
    data.push_back( 0xae1234ae);
        // closing trigger header

    for( unsigned int i=0; i<data.size(); ++i){
        memcpy( p+bytes_copied, &data[i], sizeof( uint32_t) );
        bytes_copied += sizeof(uint32_t);       
    }

}



void CAENV1495Parameter::Deserialize( ifstream& file ){

    int size = sizeof(uint32_t)*GetHeaderSize();
    
    char* buffer = new char[ size ];
    file.read( buffer, size);

    Deserialize( buffer);

    delete [] buffer;
}



void CAENV1495Parameter::Deserialize( char* p, bool flip ){

    vector<uint32_t> data;
    uint32_t temp = 0;

    for( unsigned int i=0; i<GetHeaderSize(); ++i){
        memcpy( &temp, p+i*sizeof(uint32_t), sizeof( uint32_t) );
        data.push_back(temp);
    }

    int offset = 2;
        // skip number of bytes and version.

    base_addr = data[offset++];
    output_mode = static_cast<V1495_OMODE>(data[offset++]);
    retrig_xystal = data[offset++];
    retrig_veto = data[offset++];
    veto_mask = data[offset++];
    maj_lev = data[offset++];
    sig_delay = data[offset++];
    trig_time = data[offset++];
    gate_len_xystal = data[offset++];
    gate_len_veto = data[offset++];
    dead_time = data[offset++];
    veto_time = data[offset++];
}


void CAENV1495Parameter::ExportHDF5( H5FileManager* h5man){

    if( h5man->IsFileOpen()==false ){
        return;
    }

    string name = "/logic_trigger";
    h5man->OpenGroup( name );

    //h5man->AddAttribute( name, "enable", (unsigned int)(enable) );
        // board is by default all enabled if it is added into the HDF5 file
    h5man->AddAttribute( name, "version", string("1.0.0") );
    h5man->AddAttribute( name, "vme_address", base_addr );
    h5man->AddAttribute( name, "mode", GetModeString(output_mode) );

    h5man->AddAttribute( name, "crystal_gate_length", gate_len_xystal );
    h5man->AddAttribute( name, "veto_gate_length", gate_len_veto );

    h5man->AddAttribute( name, "dead_time", dead_time );
    h5man->AddAttribute( name, "veto_time", veto_time );

    h5man->AddAttribute( name, "crystal_retrigger", (unsigned int)(retrig_xystal) );
    h5man->AddAttribute( name, "veto_retrigger", (unsigned int)(retrig_veto) );

    h5man->AddAttribute( name, "veto_mask", veto_mask );
    h5man->AddAttribute( name, "veto_majority_level", maj_lev );

    h5man->AddAttribute( name, "trigger_delay", sig_delay );
    h5man->AddAttribute( name, "trigger_duration", trig_time );

}


string CAENV1495Parameter::GetModeString( V1495_OMODE mode ){
    switch(mode){

        case CRYSTAL :
            return "CRYSTAL";

        case VETO :
            return "VETO";

        case LSCINTILLATOR :
            return "LIQUID_SCINTILLATOR";

        case CALIBRATION :
            return "CALIBRATION";

        default:
            return "INVALID";
    }
}
