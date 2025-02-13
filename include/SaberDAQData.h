#ifndef SABERDAQDATA_H
    #define SABERDAQDATA_H 1

#include <ostream>
#include <vector>

#include "SaberBoardRawData.h"
#include "CAENV1720Parameter.h"


class SaberDAQData {

public:

    SaberDAQData();

    SaberDAQData( std::vector<CAENV1720Parameter> adc);

    SaberDAQData( const SaberDAQData& rhs);

    virtual ~SaberDAQData();

    SaberDAQData& operator=(const SaberDAQData& rhs);

    virtual bool IsHeader(){ return false;}

    void AddBoardData( const SaberBoardRawData& b);

    virtual void Write( ostream& os);

    int size(){ return board_data.size();}
        //!< Returns number of boards recorded in this object.

    SaberBoardRawData& operator[]( unsigned int n);
        //!< Element access operator. If argument exceeds vector length, will result in unpredictable behavior.

    SaberBoardRawData GetBoardData( int n);
        //!< Returns board data object. If argument exceeds vector length, will return default board data object.
        
    SaberBoardRawData* GetBoardDataPtr( int n);
        //!< Returns a pointer to a board data object. If argument exceeds vector length, it will return 0 to indicate an error.

    int GetNSignal( int min_dev);

private:

    std::vector<SaberBoardRawData> board_data;


};

#endif
