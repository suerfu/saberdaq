
#include <iostream>
using namespace std;

#include "H5FileManager.h"

int main( int argc, char* argv[] ){

    // ============================================
    // Test creating various group structures
    // ============================================

    H5FileManager* man = new H5FileManager();
    man->OpenFile("test.hdf5", "w+");
    man->OpenGroup("dir0");
    man->OpenGroup("/dir1");
    man->OpenGroup("/dir2/");
    man->OpenGroup("dir3/");

    // ============================================
    // Sub-groups, created directly by specifying the entire string
    man->OpenGroup("dir4/dir5");
    man->OpenGroup("/dir6/dir7");
    man->OpenGroup("/dir8/dir9/");
    man->OpenGroup("dir10/dir11");

    // ===========================================
    // Write some data
    const int Nrow = 2;
    const int Ncol = 10;
    unsigned int dims[2] = { Nrow, Ncol};
    float* voltage = new float[ dims[0]*dims[1] ];
    for( unsigned int i=0; i<dims[0]; i++){
        for( unsigned int j = 0; j<dims[1]; j++){
            voltage[ j+i*dims[1] ] = i+0.1*j;
        }
    }
    man->WriteData<float>( voltage, "/dir0/voltage0", H5::PredType::NATIVE_FLOAT, 2, dims );

    //map<string, vector<string/*const char**/>> str_attr;
    //str_attr["str1"].push_back("Hello World! ");
    //str_attr["str1"].push_back("Welcome ");
    //str_attr["str1"].push_back("to Magia Record.");
    //str_attr["str2"].push_back("Another test string with new key name.");
    //man->AddAttribute<string/*const char**/>( "/dir0/voltage0", str_attr);
    string msg = "Magia Record";
    man->AddAttribute("/dir0/voltage0", "str1", msg);
    //for( int i=0; i<3; i++)
        //cout << str_attr["str1"][i] << '\t' << (void*) str_attr["str1"][i].c_str() << endl;

    man->WriteData<float>( voltage, "/dir10/dir11/voltage0", H5::PredType::NATIVE_FLOAT, 2, dims );

    //for( int i=0; i<3; i++)
        //cout << str_attr["str1"][i] << '\t' << (void*) str_attr["str1"][i].c_str() << endl;

    vector<int> int_attr;
    int_attr.push_back( 1 );
    int_attr.push_back( 2 );
    int_attr.push_back( 3 );
    int_attr.push_back( 3 );
    
    unsigned int dim[2] = {2,2};
    man->AddAttribute<int>( "/dir10", "test_attr_w_rank", int_attr, H5::PredType::NATIVE_INT, 2, dim );

    cout << "File Size: " << man->GetFileSize() << endl;
    man->CloseFile();

    return 0;
}

