
#include <iostream>
#include <fstream>

#include "CAENV1495Parameter.h"
#include "CAENV1720Parameter.h"

using std::cout;
using std::endl;

int main( int argc, char* argv[] ){

    if( argc<3 ){
        cout << "usage: " << argv[0] << " cfg-file output" << endl;
        return -1;
    }

    ConfigParser cparser( argv[1] );

    CAENV1495Parameter trigger;
    CAENV1720Parameter adc;

    trigger.SetParamFromConfig( &cparser);
    adc.SetParamFromConfig( &cparser, "/module/daq/board0/");

    cout << trigger.GetPrintString();
    cout << adc.GetPrintString();

    char* trig_stream = new char[ 4*( trigger.GetHeaderSize() )];
    char* adc_stream = new char[ 4*( adc.GetHeaderSize() )];

    trigger.Serialize( trig_stream);
    adc.Serialize( adc_stream);

    ofstream file( argv[2], ios_base::out);
    file.write( trig_stream, 4*trigger.GetHeaderSize());
    file.write( adc_stream, 4*adc.GetHeaderSize());

    file.close();

    delete trig_stream;
    delete adc_stream;

    return 0;

}
