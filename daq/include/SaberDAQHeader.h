#ifndef SABERDAQHEADER_H
    #define SABERDAQHEADER_H 1

#include "SaberDAQData.h"

#include <cstring>

class SaberDAQHeader : public SaberDAQData {

public :

    SaberDAQHeader(){;}

    ~SaberDAQHeader(){
        header.clear();
    }

    void CopyHeader( void* v, int bytes){
        char* p = reinterpret_cast<char*>(v);
        header.clear();
        for( int i=0; i<bytes; i++){
            header.push_back(p[i]);
        }
    }

    void Write( ostream& os){
        if( header.size()>0 )
            os.write( &header[0], header.size() );
    }

    bool IsHeader(){ return true;}

    uint32_t GetHeader(){
        uint32_t val = 0;
        if( header.size()>0 )
            memcpy( &val, &header[0], sizeof( val ) );
        return val;
    }

private :

    vector<char> header;

};

#endif
