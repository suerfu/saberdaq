#include "SaberDAQData.h"


SaberDAQData::SaberDAQData(){;}


// Constructor to make object from an array of ADC board and one FPGA trigger board.
// A simple deep copy is performed
//
SaberDAQData::SaberDAQData( std::vector<CAENV1720Parameter> a, CAENV1495Parameter b){

    // for each ADC board, allocate memory enough to store raw waveform
    // needed memory is calculated from the ADC parameter object directly
    //
    std::vector<CAENV1720Parameter>::iterator itr;
    
    for(  itr = a.begin(); itr!=a.end(); ++itr){

        adc_parameters.push_back( *itr );
            // copy and store ADC parameter object

        board_data.push_back( SaberBoardRawData() );
            // create RawData object
        board_data.back().AllocateBuffer( itr->GetTotalSizeInByte() );
            // for the RawData object just created, manually allocate just enough data for the waveform to be read out.
    }

    trigger_parameter = b;
}


SaberDAQData::SaberDAQData( const SaberDAQData& rhs){
    board_data = rhs.board_data;
    trigger_parameter = rhs.trigger_parameter;
}


SaberDAQData& SaberDAQData::operator=(const SaberDAQData& rhs){
    board_data = rhs.board_data;
    trigger_parameter = rhs.trigger_parameter;
    return *this;
}


void SaberDAQData::AddBoardData( const SaberBoardRawData& b){
    board_data.push_back(b);
}


// Function to add trigger configuration after object creation
//
void SaberDAQData::AddTriggerParameter( const CAENV1495Parameter& a){
    trigger_parameter = a;
}


// Function to add ADC configuration
//
void SaberDAQData::AddADCParameter( const CAENV1720Parameter& a){
    adc_parameters.push_back( a );
}


// Function to write current data to file
//
void SaberDAQData::Write( ostream& os){
    if( board_data.size()!=0 )
        for( unsigned int i=0; i<board_data.size(); i++){
            os.write( reinterpret_cast<char*>( board_data[i].GetBufferAddr()), board_data[i].bytes());
        }
}


// Member access function
//
SaberBoardRawData& SaberDAQData::operator[]( unsigned int n){
    return board_data[n];
}


SaberBoardRawData SaberDAQData::GetBoardData( int n){
    return board_data[n];
}


SaberBoardRawData* SaberDAQData::GetBoardDataPtr( int n){
    if( n<int(board_data.size()) )
        return &board_data[n];
    else
        return 0;
}


CAENV1495Parameter SaberDAQData::GetTriggerParameter(){
    return trigger_parameter;
}


vector<CAENV1720Parameter> SaberDAQData::GetADCParameters(){
    return adc_parameters;
}


int SaberDAQData::GetNSignal( int min_dev){
    int n = 0;
    for( unsigned int i=0; i<board_data.size(); ++i)
        n += board_data[i].GetNSignal( min_dev);
    return n;
}
