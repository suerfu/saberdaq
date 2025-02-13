#include <cstdint>  // needed for int32_t and uint32_t
#include <vector>
#include <map>

#include "ConfigParser.h"

#include "CAENV1495Parameter.h"
#include "CAENV1720Parameter.h"

#include "SaberDecoder.h"
#include "SaberDAQHeader.h"

using namespace std;


int main( int argc, char* argv[] ){

    if( argc==1 ){
        cerr << "usage: "<< argv[0] << " --input old-file.raw --cfg config.cfg --output new-file.raw\n"
             << "       The program will add config file and adc info to the old data format in which config and adc are missing.\n"
             << "       The config file should be in the new format.\n";
        return -1;
    }

    ConfigParser config( argc, argv);
    config.Print( cout );

    string infile = config.GetString( "/cmdl/input" );
    if( infile=="" ){
        cout << "input raw file not specified.\n";
        return -1;
    }

    ifstream ifile;
    ifile.open( infile.c_str(), ios_base::in );
    if( !ifile ){
        cerr << "error opening " << infile.c_str() << endl;
        return -1;
    }
    
    string outfile = config.GetString( "/cmdl/output" );
    if( outfile=="" ){
        cout << "output raw file not specified.\n";
        return -1;
    }

    ofstream ofile;
    ofile.open( outfile.c_str(), ios_base::out );
    if( !ofile ){
        cerr << "error opening " << outfile.c_str() << endl;
        return -1;
    }
    
    // boolean varialbe to check if parameter is found
    bool found = false;;

    vector<SaberDAQHeader*> header_to_send;

    // **************************************************************
    //                      trigger
    // **************************************************************

    cout << "Assembling trigger header...\n";
    bool trig_en = config.GetBool("/module/daq/logic_trigger/enable", &found );
    if( trig_en && found ){

        CAENV1495Parameter param;
        param.SetParamFromConfig( &config );
    
        char* p = new char[ 4*param.GetHeaderSize() ];
        param.Serialize( p );

        SaberDAQHeader* hdr = new SaberDAQHeader();
        hdr->CopyHeader( p, 4*param.GetHeaderSize() );
        header_to_send.push_back( hdr );
        delete p;
    }


    // ****************************************************************
    //                      ADC
    // ****************************************************************

    map< string, vector<string> > adcs = config.GetListOfParameters( "/module/daq/board" );
    map< string, vector<string> >::iterator adc_itr;

    vector< CAENV1720Parameter > param_adc;

    cout << "Assembling ADC header...\n";
    for( adc_itr = adcs.begin(); adc_itr!=adcs.end(); ++adc_itr){

        string dirname = adc_itr->first;
        if(dirname=="/module/daq/board*/")
            continue;

        CAENV1720Parameter param;
        param.SetParamFromConfig( &config, dirname);

        bool adc_enabled = config.GetBool( dirname+"enable", &found );
        if( adc_enabled && found ){
            if( param.ch_enable_mask>0x0 ){

                char* p = new char[ 4*param.GetHeaderSize() ];
                param.Serialize( p );

                SaberDAQHeader* hdr = new SaberDAQHeader();
                hdr->CopyHeader( p, 4*param.GetHeaderSize() );

                header_to_send.push_back( hdr );
                delete p;

                param_adc.push_back( param );
            }
        }
    }

    if( param_adc.size()==0 ){
        cout << "In configuration file no enabled board found. Please make sure board is enabled with enable true.\n";
        return -1;
    }

    config.Serialize( ofile );

    // **************************************************************
    //               Config global header and send all
    // **************************************************************

    // global header begin
    cout << "Writing global header...\n";
    SaberDAQHeader* glb = new SaberDAQHeader();

    uint32_t glb_header[4];
    glb_header[0] = 0xaa1234aa;

    glb_header[1] = 4*sizeof( glb_header[0] );
    for( unsigned int i=0; i<header_to_send.size(); ++i)
        glb_header[1] += header_to_send[i]->size();

    glb_header[2] = 0;
    glb_header[3] = 0;

    glb->CopyHeader( glb_header, 4*sizeof( glb_header[0] ) );
    glb->Write( ofile );

    // send ADC and Trigger header
    cout << "Writing trigger and adc header...\n";
    for( unsigned int i=0; i<header_to_send.size(); ++i)
        header_to_send[i]->Write( ofile );

    uint32_t glb_header_cls[2];
    glb_header_cls[0] = 0xaa1234aa;
    glb_header_cls[1] = 2*sizeof( glb_header_cls[0]);

    SaberDAQHeader* glb_cls = new SaberDAQHeader();
    glb_cls->CopyHeader( glb_header_cls, 2*sizeof( glb_header_cls[0] ) );
    glb_cls->Write( ofile );



    // *** initial event header

    cout << "Writing event header...\n";

    uint32_t evt_header[4];
    evt_header[0] = 0xee1234ee;
    evt_header[1] = 4*sizeof(evt_header[0]);
    evt_header[2] = 0;
    evt_header[3] = 0;

    // calculate bytes per event:
    for( unsigned int i=0; i<param_adc.size(); ++i){
        evt_header[3] += param_adc[i].GetTotalSizeInByte();
    }

    SaberDAQHeader* evt = new SaberDAQHeader();
    evt->CopyHeader( evt_header, 4*sizeof( evt_header[0] ) );
    evt->Write( ofile );

    // **********************************************************
    // copy the old event data

    cout << "Copying old event data...\n";

    uint32_t data;
        // variable to contain temporary data read out

    uint32_t* buff = new uint32_t[ param_adc[0].GetTotalSizeInWord()*2 ];
        // contains waveform read out from input file and to be written to output.
    
    // locate pointer to the correct position of input stream
    ifile.seekg( 0, ios::beg);
    while( !ifile.eof() ){

        ifile.read( reinterpret_cast<char*>(&data), sizeof(data) );

        if( data != 0xa0000000 + param_adc[0].GetTotalSizeInWord()){
            continue;
            ifile.seekg( -3, ios::cur );
                // every read will advance by 4 bytes
                // if not the correct header, then move back by 3 bytes so that the raw file is checked byte by byte.
        }
        else{
            cout << "next data is " << hex << data << endl;
            ifile.seekg( -4, ios::cur );
                // found correct word, move back by one word.
            break;
        }
    }

    int progress_indic = 0;

    int words_per_event = 0;
        // number of 32-bit integers per event
    uint32_t prev_time = 0;
        // trigger time tag of previous event.
    uint32_t period_counter = 0;
        // count how many carryover in the trigger time tag

    // copy old data
    while( !ifile.eof() ){

        // event size, channel enabled, trigger time tag, etc.
        for( int i=0; i<4; i++){
            ifile.read( reinterpret_cast<char*>(&buff[i]), sizeof(uint32_t));
        }

        // event size
        words_per_event = (buff[0]&0xfffffff);

        // increment time
        if( buff[3]<prev_time )
            period_counter++;
        prev_time = buff[3];

        ifile.read( reinterpret_cast<char*>(&buff[4]), sizeof(uint32_t) * (words_per_event-4) );
        ofile.write( reinterpret_cast<char*>(buff), sizeof(uint32_t) * words_per_event );

        progress_indic++;
        if( progress_indic%100000==0 )
            cout << dec << progress_indic << " events processed.\t event counter is "<< (buff[2]&0xffffff) <<endl;
    }

    if( buff!=0 )
        delete [] buff;


    // **********************************************************

    // ****** close event header

    cout << "Closing event header...\n";

    uint32_t evt_header2[2];
    evt_header2[0] = 0xee1234ee;
    evt_header2[1] = sizeof(evt_header2[0])*2;

    SaberDAQHeader* evt2 = new SaberDAQHeader();
    evt2->CopyHeader( evt_header2, 2*sizeof( evt_header2[0] ) );
    evt2->Write( ofile );

    // ****** clode global header

    cout << "Closing global header...\n";
    uint32_t glb_header2[5];
    glb_header2[0] = 0xff1234ff;
    glb_header2[1] = 5*sizeof( glb_header2[0] );
    glb_header2[2] = 0;
    glb_header2[3] = period_counter*17.179869176;
        // V1720 trigger time tag is 8 ns each bit, and 31 bit long -> roll over after ~17 s.
    glb_header2[4] = 0xff1234ff;

    SaberDAQHeader* glb2 = new SaberDAQHeader();
    glb2->CopyHeader( glb_header2, 5*sizeof( glb_header2[0] ) );
    glb2->Write( ofile );

    return 0;
}
