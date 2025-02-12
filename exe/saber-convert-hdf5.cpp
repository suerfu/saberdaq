/*
 *  Written by B. Suerfu on July 5, 2021
 *  This file generates executable that converts saber raw data format into HDF5 binary files.
 *  HDF5 binary files can be conveniently accessed with Python and Jupyter-notebook.
 */ 

#include "ConfigParser.h"
#include <fstream>
#include <string>
#include <iostream>

#include "SaberDecoder.h"

#include "H5FileManager.h"

using namespace std;


int main( int argc, char* argv[] ){

    // If no argument is specified, print usage and return.
    //
    if( argc==1 ){
        cerr << "usage: "<< argv[0] << " --input raw-file [--output root-file]\n";
        return -1;
    }

    ConfigParser config( argc, argv);

    // If there is no --input option, then print usage and exit.
    // The input file must be specified by the keyword.
    //
    string rawfile = config.GetString( "/cmdl/input" );
    if( rawfile=="" ){
        cerr << "error: input raw file not specified.\n";
        cerr << "usage: "<< argv[0] << " --input raw-file [--output root-file]\n";
        return -1;
    }

    // Next get the output file.
    // If it is specified by a keyword, then use it. Otherwise, replate the trailing .raw with .hdf5
    //
    string outfile = config.GetString( "/cmdl/output" );

    unsigned int suffix_index;
    string new_suffix = ".hdf5";

    if( outfile == "" ){

        outfile = rawfile;

        if( ( suffix_index = outfile.rfind(".raw") ) != string::npos ) {
            
            // Pattern is found. Replace
            outfile.replace( suffix_index, new_suffix.length(), new_suffix);

        }
        else{

            outfile += ".hdf5";

        }

    }

    cout << "Converting saber raw file \n\t" << rawfile << " ==> " << outfile << endl;

    // Open the input rawfile as an input stream object.
    //
    ifstream file;
    file.open( rawfile.c_str(), ios_base::in );
    if( !file ){
        cerr << "error opening " << rawfile.c_str() << endl;
        return -1;
    }
    cout << "Input file opened." << endl;

    // Create HDF5 manager object and open the output file.
    //
    H5FileManager* h5man  = new H5FileManager();
    bool stat = h5man->OpenFile(outfile, "w+");

    if( stat==false ){
        cout << "Failed to open output file." << endl;
        return -1;
    }
    cout << "Output file opened." << endl;

    // Create the event decoder object and decode the header
    //
    SaberDecoder decoder;
    decoder.Decode( &file );
    cout << "Input raw file decoded.\nWriting output file..." << endl;

    decoder.WriteHDF5( h5man );

    h5man->CloseFile();

    return 0;

}
