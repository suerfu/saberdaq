#include "SaberDAQData.h"


SaberDAQData::SaberDAQData(){;}


SaberDAQData::SaberDAQData( std::vector<CAENV1720Parameter> a){

    std::vector<CAENV1720Parameter>::iterator itr;
    for(  itr = a.begin(); itr!=a.end(); ++itr){
        board_data.push_back( SaberBoardRawData() );
        board_data.back().AllocateBuffer( itr->GetTotalSizeInByte() );
    }
}


SaberDAQData::SaberDAQData( const SaberDAQData& rhs){
    board_data = rhs.board_data;
}


SaberDAQData& SaberDAQData::operator=(const SaberDAQData& rhs){
    board_data = rhs.board_data;
    return *this;
}


SaberDAQData::~SaberDAQData(){}


void SaberDAQData::AddBoardData( const SaberBoardRawData& b){
    board_data.push_back(b);
}


void SaberDAQData::Write( ostream& os){
    if( board_data.size()!=0 )
        for( unsigned int i=0; i<board_data.size(); i++){
            os.write( reinterpret_cast<char*>( board_data[i].GetBufferAddr()), board_data[i].bytes());
        }
}


SaberBoardRawData& SaberDAQData::operator[]( unsigned int n){
    //if( n>=board_data.size())
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



int SaberDAQData::GetNSignal( int min_dev){
    int n = 0;
    for( unsigned int i=0; i<board_data.size(); ++i)
        n += board_data[i].GetNSignal( min_dev);
    return n;
}
