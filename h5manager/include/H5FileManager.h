#ifndef H5FILEMANAGER_H
    #define H5FILEMANAGER_H 1

#include "H5Cpp.h"

#include <map>
#include <vector>
#include <iostream>

using std::string;
using std::map;
using std::vector;

class H5FileManager{

public:

    // =====================================
    // Constructor & Destructor
    // =====================================
    
    H5FileManager();
    virtual ~H5FileManager();


    // =====================================
    // File operations
    // =====================================
    
    unsigned int GetFileSize(){
        if( file_ptr==0 )
            return 0;
        return file_ptr->getFileSize();
    }

    bool OpenFile( const string& name, const string& flag="rw");
        //!< Open a file specified by the file name as a string
    
    bool IsFileOpen(){
        return file_ptr!=0;
    }
        //!< This function helps check if H5FileManager is managing an open file.
    
    bool CloseFile();
        //!< Close the current file.
        //!< This method also closes all open attributes and groups.

    string GetFileName(){ return filename;}


    // =====================================
    // Group operations
    // =====================================
    
    bool OpenGroup( string name);
    bool CloseGroup( const string& name);

    
    // =====================================
    // Data operations
    // =====================================
    
    // Add data by specifying all arguments
    template < class PtrType >
    bool WriteData( PtrType* data, string name, const H5::DataType& type, unsigned int rank, unsigned int dims[] );

    // Add data by external dataset.
    template < class PtrType >
    bool WriteData( PtrType* data, const H5::DataType& type, H5::DataSet* dset);

    
    // =====================================
    // Dataset operations
    // =====================================
 
    H5::DataSet CreateDataSet( string name, const H5::DataType& type, unsigned int rank, unsigned int dims[] ){
        #ifdef debug
            std::cout << "CreateDataSet: Creating dataset...\n";
            std::cout << "CreateDataSet: Checking if dataset already exists...\n";
        #endif

        if( DataSetExist( name ) ){
            #ifdef debug
                std::cout << "CreateDataSet: Dataset already exists. Reusing existing one...\n";
            #endif
            return file_ptr->openDataSet( name.c_str());
        }
        else{
            #ifdef debug
                std::cout << "CreateDataSet: Dataset does not exist. Creating one...\n";
            #endif        
        }

        hsize_t h5dims[rank];
        for( unsigned int i=0; i<rank; i++)
            h5dims[i] = dims[i];
        return file_ptr->createDataSet( name.c_str(), type, H5::DataSpace(rank, h5dims) );
    }

    H5::DataSet OpenDataSet( string name){
        return file_ptr->openDataSet( name.c_str());
    }


    // =====================================
    // Attributes
    // =====================================

    // Add attributes individually 
    template < class D >
    bool AddAttribute( string app_name, string atr_name, const D& atr, const H5::DataType& );

    template < class D>
    bool AddAttribute( string app_name, string atr_name, const D& atr ){
        vector<D> atr_vec;
        atr_vec.push_back(atr);
        return AddAttribute<D>( app_name, atr_name, atr_vec );
    }


    // Add vector attributes
    template < class D >
    bool AddAttribute( string app_name, string atr_name, const vector<D>& atr, const H5::DataType&, int rank = 1, unsigned int dim[] = {} );

    template < class D>
    bool AddAttribute( string app_name, string atr_name, const vector<D>& atr, int rank=1, unsigned int dim[] = {} );

    // Add list/collection of attributes
    template < class D >
    bool AddAttribute( string app_name, const std::map<string, vector<D>>& atr_map, const H5::DataType& );

    template < class D>
    bool AddAttribute( string app_name, const std::map<string, vector<D>>& atr_map );
    

private:

    bool GroupExist( string name){
        if( *name.begin()!='/' ){
            name = "/"+name;
        }

        if( list_group.find( name )==list_group.end() ){
            #ifdef debug
                std::cout << "GroupExist: Couldn't find entry for " << name << ". Below is the whole list of groups:\n";
                for( std::map< string, H5::Group*>::iterator itr = list_group.begin(); itr!=list_group.end(); itr++){
                    std::cout << itr->first << '\t';
                }
                std::cout << std::endl;
            #endif
            return false;
        }
        return true;
    }

    bool DataSetExist( string name, bool warn=false){
        H5::DataSet ds;
        try{
            H5::Exception::dontPrint();
            ds = file_ptr->openDataSet( name.c_str() );
        }
        catch( H5::FileIException not_found_error){
            #ifdef debug
                std::cout << "DataSetExist: Dataset does not exist. Exception caught."<< std::endl;
            #endif
            return false;
        }
        return true;
    }


private:

    H5::H5File* file_ptr;
    string filename;

    std::map<string, H5::Group*> list_group;
};



template <class PtrType>
bool H5FileManager::WriteData( PtrType* data, string name, const H5::DataType& type, unsigned int rank, unsigned int dims[]){
    
    try{
    #ifdef debug
        std::cout << "WriteData: Attempting to write data to " << name << "\n";
        std::cout << "WriteData: Creating dataset...\n";
    #endif

    H5::DataSet dset( CreateDataSet( name, type, rank, dims) );
    
    #ifdef debug
        std::cout << "WriteData: Dataset created\n";
        std::cout << "WriteData: Writing data...\n";
    #endif

    dset.write( data, type );

    #ifdef debug
        std::cout << "WriteData: Data written" << std::endl;
    #endif
    }
    catch( H5::FileIException error ){
        #ifdef debug
            std::cout << "Failed due to FileIException. " << std::endl;
            error.printErrorStack();
        #endif
        //std::cout << error.getDetailMsg() << std::endl;
        return false;
    }

    return true;
}


template <class PtrType>
bool H5FileManager::WriteData( PtrType* data, const H5::DataType& type, H5::DataSet* dset){

    if( dset==0 ){
        #ifdef debug
            std::cout << "WriteData: Data writing failed due to invalid pointer to DataSet" << std::endl;
        #endif
        return false;
    }

    #ifdef debug
        std::cout << "WriteData: Data written" << std::endl;
    #endif
    
    dset->write( data, type );

    return true;
}

// Add an array of attributes by vector.

template< class D >
bool H5FileManager::AddAttribute( string app_name, string attr_name, const vector<D>& attr_vec, const H5::DataType& datatype, int rank, unsigned int dim[] ){
    
    if( !file_ptr )
        return false;

    void* foo = 0;
        // pointer to object to which attribute is added. Will point to either file or group.

    bool file_attr = false;
    bool group_attr = false;

    if( app_name=="" || app_name=="/"){
        file_attr = true;
        foo = file_ptr;
    }
    else if( GroupExist( app_name) ){
        group_attr = true;
        foo = list_group[app_name];
    }
    else if( DataSetExist( app_name) ){
        foo =  new H5::DataSet( this->OpenDataSet( app_name ) );
    }
    else{
        return false;
    }

    if( attr_vec.empty() ){
        return false;
    }

    hsize_t* dims = 0;
    if( rank==1 ){
        dims = new hsize_t( attr_vec.size() );
    }
    else{
        dims = new hsize_t[rank];
        for( int r=0; r<rank; r++)
            dims[r] = dim[r];
    }

    H5::DataSpace attr_ds;
    if( attr_vec.size()==1 ){
        attr_ds = H5::DataSpace( H5S_SCALAR);
    }
    else{
        attr_ds = H5::DataSpace( rank, dims );
        delete dims;
    }

    H5::Attribute attr;
    if( file_attr ){
        try{
            H5::Exception::dontPrint();
            attr = file_ptr->createAttribute( attr_name.c_str(), datatype, attr_ds);
        }
        catch( H5::AttributeIException error ){
            // Print( error.getDetailMsg(), ERR);
            return false;
        }
    }
    else if( group_attr){
        try{
            H5::Exception::dontPrint();
            attr = reinterpret_cast<H5::Group*>(foo)->createAttribute( attr_name.c_str(), datatype, attr_ds);
        }
        catch( H5::AttributeIException error ){
            // Print( error.getDetailMsg(), ERR);
            return false;
        }
    }
    else{
        try{
            H5::Exception::dontPrint();
            attr = reinterpret_cast<H5::DataSet*>(foo)->createAttribute( attr_name.c_str(), datatype, attr_ds);
        }
        catch( H5::AttributeIException error){
            // Print( error.getDetailMsg(), ERR);
            return false;
        }
    }

    try{
        attr.write( datatype, attr_vec.data());
        attr.close();

        // If attribute belongs to a DataSet, then close it since it is opened in this function.
        if( !file_attr && !group_attr ){
            reinterpret_cast<H5::DataSet*>(foo)->close();
            delete reinterpret_cast<H5::DataSet*>(foo);
            foo = 0;
        }
    }
    catch( H5::AttributeIException error){
        error.printErrorStack();
        return false;
    }

    return true;
}


// Add a collection of attributes as a map.

template< class D >
bool H5FileManager::AddAttribute( string app_name, const std::map<string, vector<D>>& atr_map, const H5::DataType& datatype ){

    if( !file_ptr )
        return false;

    typename map<string, vector<D>>::const_iterator itr;
    for( itr = atr_map.begin(); itr!=atr_map.end(); itr++){
        
        string attr_name = itr->first;
        vector<D> attr_vec = itr->second;
        if( attr_vec.empty() )
            continue;
        if (AddAttribute<D>( app_name, attr_name, attr_vec, datatype )==false )
            return false;
    }

    return true;
}


// Add attributes individually.

template< class D >
bool H5FileManager::AddAttribute( string app_name, string attr_name, const D& attr_data, const H5::DataType& datatype ){
    vector<D> attr_vec;
    attr_vec.push_back( attr_data);
    return AddAttribute( app_name, attr_name, attr_vec, datatype);
}


#endif
