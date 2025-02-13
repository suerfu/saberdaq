/*
    On July 5, 2021
    Created by

    Burkhant Suerfu
    suerfu64@gmail.com

    This code demonstrates the basic usage of H5FileManager.
    Most of the operations are self-explanatory.
*/

#include "H5FileManager.h"

#include <iostream>
#include <string>
using namespace std;

int main( int argc, char* argv[] ){

    // Create the manager object.

    H5FileManager* man = new H5FileManager();

    // =================================================
    // Create output file
    // =================================================

    man->OpenFile("demo.hdf5", "w+");

    // Add some attributes or comments to the entire file
    //
    man->AddAttribute( "/", "string-comment", string("This is a global attribute") );


    // Integer types can be added by default
    // Float types need explicit template
    // For double types, also need to explicitly specify the HDF5 data type.
    //
    man->AddAttribute( "/", "integer", 123 );
    man->AddAttribute<float>( "/", "pi-flt", 3.1415926 );
    man->AddAttribute( "/", "pi-flt2", float(3.1415926) );
    man->AddAttribute<double>( "/", "pi-dbl", 3.1415926, H5::PredType::NATIVE_DOUBLE );


    // Integer array can also be added as an attribute.
    // The rank is by default 1.
    vector<int> int_array;
    int_array.push_back(0);
    int_array.push_back(1);
    int_array.push_back(2);
    man->AddAttribute( "/", "integer-array", int_array);

    // Float array can be added in a similar way.
    //
    vector<float> float_array;
    float_array.push_back(1.414);
    float_array.push_back(3.141);
    float_array.push_back(2.78);
    man->AddAttribute( "/", "float-array", float_array);


    // Adding a string array
    //
    vector<string> str_array;
    str_array.push_back("Hello");
    str_array.push_back("World");
    man->AddAttribute( "/", "string-array", str_array);


    // =================================================
    // Add some data under root directory
    // =================================================
    
    // First specify the rank
    const int Nrow = 2;
    const int Ncol = 10;
    unsigned int dims[2] = { Nrow, Ncol};

    // Generate the actual data
    //
    float* voltage = new float[ dims[0]*dims[1] ];
    for( unsigned int i=0; i<dims[0]; i++){
        for( unsigned int j = 0; j<dims[1]; j++){
            voltage[ j+i*dims[1] ] = i+0.1*j;
        }
    }

    // Data added under root "/" with name event0
    //
    man->WriteData<float>( voltage, "/event0", H5::PredType::NATIVE_FLOAT, 2, dims );

    // Add some attribute to this event
    //
    man->AddAttribute( "/event0", "comment", string("This is a string attribute added to the event") );


    // =================================================
    // Create a group
    // =================================================

    man->OpenGroup("dir0");
    man->OpenGroup("/dir1");
    man->OpenGroup("/dir2/dir3");

    // Add attributes to groups
    //
    man->AddAttribute( "/dir0", "comment", string("This is a string attribute added to dir0") );
    man->AddAttribute( "/dir2/dir3", "comment", string("This is a string attribute added to dir3 which is under dir2") );


    // Add some data under dir3
    //
    man->WriteData<float>( voltage, "/dir2/dir3/event1", H5::PredType::NATIVE_FLOAT, 2, dims );


    // Add some attributes as matrices by reshaping

    vector<int> mat_attr;
    for( unsigned int i=0; i<12; i++)
        mat_attr.push_back( i );
    
    unsigned int dim1[2] = {2,6};
    man->AddAttribute<int>( "/dir0", "mat_attr_2x6", mat_attr, H5::PredType::NATIVE_INT, 2, dim1 );

    unsigned int dim2[2] = {3,4};
    man->AddAttribute<int>( "/dir0", "mat_attr_3x4", mat_attr, H5::PredType::NATIVE_INT, 2, dim2 );


    // =================================================
    // Close the output file
    // =================================================

    cout << "File Size: " << man->GetFileSize() << endl;

    man->CloseFile();

    return 0;

}

