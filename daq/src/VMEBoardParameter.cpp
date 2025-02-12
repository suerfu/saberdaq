
#include "VMEBoardParameter.h"

using namespace std;

VMEBoardParameter::VMEBoardParameter(){
    base_addr = 0x0;
}


VMEBoardParameter::~VMEBoardParameter(){;}


uint32_t VMEBoardParameter::GetLinkNumber(){
    return link_number;
}


void VMEBoardParameter::SetLinkNumber( uint32_t s){
    link_number = s;
}


uint32_t VMEBoardParameter::GetBoardNumber(){
    return board_number;
}


void VMEBoardParameter::SetBoardNumber( uint32_t s){
    board_number = s;
}


uint32_t VMEBoardParameter::GetBaseAddr(){
    return base_addr;
}


void VMEBoardParameter::SetBaseAddr( uint32_t s){
    base_addr = s;
}


string VMEBoardParameter::GetMessage(){
    if( message_pipe.empty() )
        return "";
    else{
        string s = message_pipe.front();
        message_pipe.pop();
        return s;
    }
}


int VMEBoardParameter::GetStatus(){
    return status;
}


uint32_t VMEBoardParameter::GetVersion(){
    return 0;
}


unsigned int VMEBoardParameter::GetHeaderSize(){
    return 0;
}


void VMEBoardParameter::Serialize( char* p){}


void VMEBoardParameter::Deserialize( char* p, bool flip_byte=false){}


uint32_t FlipByteOrder( uint32_t input){
    uint32_t output;
    char* p = reinterpret_cast<char*>( &input );
    char* q = reinterpret_cast<char*>( &output );
    for( int i=0; i<4; i++){
        *(q+i) = *(p+3-i);
    }
    return output;
}
