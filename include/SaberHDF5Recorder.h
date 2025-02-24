#ifndef SABERH5RECORDER_H
    #define SABERH5RECORDER_H 1

#include <string>
#include <sstream>

#include "plrsController.h"
#include "plrsModuleRecorder.h"

#include "SaberDAQData.h"

#include "H5FileManager.h"

class SaberHDF5Recorder : public plrsModuleRecorder{

public:
    SaberHDF5Recorder( plrsController* c);

    ~SaberHDF5Recorder();

protected:

    void Configure();

	void Deconfigure();

    void PreRun();

    virtual void Run();

    void PostRun();

private:

    // configuration function to set HDF5 addributes and directories
    //
    void ConfigureOutput( SaberDAQData* );

    // function to write actual event
    //
    void WriteToOutput( SaberDAQData* );

    void CloseOutput( SaberDAQData* );

    int next_addr;

    string GetFileName();

    int GetNextModuleID();

    H5FileManager* h5man;

    //uint32_t timestamp_g;   // Global timestamp at the beginning of first dump.
    //uint32_t timestamp_l;   // Local timestamp of each dump.

    //set<string> parameter_added;
        // Used to keep track of added parameters.
        // In the end, all other parameters not added will be added as strings.

    unsigned int nb_adc_board;

    uint64_t evt_counter;

    /*
    template< typename T>
    void AddAttribute(string  app_name, string attr_name, vector<T> attr, int rank=1, unsigned int dim[]={} ){
        h5man->AddAttribute<T>( app_name, attr_name, attr, rank, dim );
        parameter_added.insert(attr_name);
    }

    template< typename T>
    void AddAttribute(string  app_name, string attr_name, T attr ){
        h5man->AddAttribute<T>( app_name, attr_name, attr );
        parameter_added.insert(attr_name);
    }

    template< typename T>
    void AddAttribute( string full_name, T attr){
        h5man->AddAttribute<T>( GetDirectory(full_name), GetName(full_name), attr);
    }
    */

    string GetDirectory( string foo ){
        if( foo[0]!='/' )
            foo = "/"+foo;
        string sub = foo.substr(0, foo.find_last_of('/'));
        if( sub=="" )
            sub = "/";
        return sub;
    }

    string GetName( string foo ){
        string sub = foo.substr(foo.find_last_of('/')+1);
        if( sub=="" )
            sub = foo;
        return sub;
    }
    
};


extern "C" SaberHDF5Recorder* create_SaberHDF5Recorder( plrsController* c);

extern "C" void destroy_SaberHDF5Recorder( SaberHDF5Recorder* p );


#endif
