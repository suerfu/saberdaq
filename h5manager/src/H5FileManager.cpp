
#include "H5FileManager.h"

#include <iostream>
#include <cstring>

using std::cout;
using std::endl;
using H5::Group;

H5FileManager::H5FileManager(){
    file_ptr = 0;
    filename = "";
}


H5FileManager::~H5FileManager(){
    // if file is still open, close it.
    if( file_ptr!=0 ){
        std::map<string, H5::Group*>::iterator itr;
        for( itr= list_group.begin(); itr!=list_group.end(); itr++){
            H5::Group* foo = itr->second;
            if( foo!=0 ){
                foo->close();
                delete foo;
                foo = 0;
            }
        }
        CloseFile();
        delete file_ptr;
        file_ptr = 0;
    }
}


bool H5FileManager::OpenFile( const string& name, const string& f){

    if( file_ptr!=0 )
        return false;

    unsigned int flag = H5F_ACC_RDWR;
    if( f=="r" )
        flag = H5F_ACC_RDONLY;
    else if( f=="w" )
        flag = H5F_ACC_EXCL;
    else if( f=="w+" )
        flag = H5F_ACC_TRUNC;

    try{
        file_ptr = new H5::H5File( name.c_str(), flag);
    }
    catch( H5::FileIException err){
        std::cout << "Error: " << err.getCDetailMsg() << std::endl;
        return false;
    }
    filename = name;
    return true;
}


bool H5FileManager::CloseFile(){
    if( file_ptr!=0 ){
        for( map<string, H5::Group*>::iterator itr = list_group.begin(); itr!=list_group.end(); itr++ ){
            if( itr->second != 0 ){
                itr->second->close();
            }
        }
        list_group.clear();
        file_ptr->close();
        delete file_ptr;
        file_ptr = 0;
        return true;
    }
    return false;
}


bool H5FileManager::OpenGroup( string name ){

    if( file_ptr==0  )
        return false;
    
    // check and convert name to standard format: /foor/bar

    if( *(name.end()-1)=='/')
        name.erase( name.end()-1);

    if( *(name.begin())!='/' )
        name = "/"+name;

    std::size_t pos = 0;

    do{
        pos = name.find( '/', pos+1);   // positions of substrings separated by '/' in the name
        string gname = name.substr( 0, pos);    // group name

        std::map<string, Group*>::iterator itr = list_group.find( gname );
        if( itr==list_group.end() ){
            list_group[gname] = new Group( file_ptr->createGroup( gname.c_str()) );
        }
    }while( pos!=std::string::npos );
    
    return true;
}


bool H5FileManager::CloseGroup( const string& name){

    if( list_group.find( name )!=list_group.end() ){
        list_group[name]->close();
        delete list_group[name];
        list_group[name] = 0;
        return true;
    }
    return false;
}


// ====================================================================================
// Operations with map.

template <>
bool H5FileManager::AddAttribute<int>( string app_name, const std::map<string, vector<int>>& atr_map ){
    return AddAttribute<int>( app_name, atr_map, H5::PredType::NATIVE_INT);
}

template <>
bool H5FileManager::AddAttribute<unsigned int>( string app_name, const std::map<string, vector<unsigned int>>& atr_map ){
    return AddAttribute<unsigned int>( app_name, atr_map, H5::PredType::NATIVE_UINT);
}

template <>
bool H5FileManager::AddAttribute<float>( string app_name, const std::map<string, vector<float>>& atr_map ){
    return AddAttribute<float>( app_name, atr_map, H5::PredType::NATIVE_FLOAT);
}

template <>
bool H5FileManager::AddAttribute<const char*>( string app_name, const std::map<string, vector<const char*>>& atr_map ){
    return AddAttribute<const char*>( app_name, atr_map, H5::StrType(H5::PredType::C_S1, H5T_VARIABLE) );
}

template <>
bool H5FileManager::AddAttribute<string>( string app_name, const std::map<string, vector<string>>& atr_map ){
    
    std::map<string, vector<const char*>> cstr;

    std::map<string, vector<string>>::const_iterator itr;
    for( itr = atr_map.begin(); itr!=atr_map.end(); itr++ ){
        for( vector<string>::const_iterator itr2 = itr->second.cbegin(); itr2!=itr->second.cend(); itr2++ ){
            cstr[ itr->first ].push_back( itr2->c_str() );
        }
    }
    return AddAttribute<const char*>( app_name, cstr, H5::StrType(H5::PredType::C_S1, H5T_VARIABLE) );
}

// ====================================================================================

// Operations on vector

template <>
bool H5FileManager::AddAttribute<int>( string app_name, string atr_name, const vector<int>& atr, int rank, unsigned int dim[]  ){
    return AddAttribute<int>( app_name, atr_name, atr, H5::PredType::NATIVE_INT, rank, dim);
}


template <>
bool H5FileManager::AddAttribute<float>( string app_name, string atr_name, const vector<float>& atr, int rank, unsigned int dim[] ){
    return AddAttribute<float>( app_name, atr_name, atr, H5::PredType::NATIVE_FLOAT, rank, dim);
}

template <>
bool H5FileManager::AddAttribute<uint32_t>( string app_name, string atr_name, const vector<uint32_t>& atr, int rank, unsigned int dim[] ){
    return AddAttribute<uint32_t>( app_name, atr_name, atr, H5::PredType::NATIVE_UINT32, rank, dim);
}

template <>
bool H5FileManager::AddAttribute<uint64_t>( string app_name, string atr_name, const vector<uint64_t>& atr, int rank, unsigned int dim[] ){
    return AddAttribute<uint64_t>( app_name, atr_name, atr, H5::PredType::NATIVE_UINT64, rank, dim);
}

template <>
bool H5FileManager::AddAttribute<string>( string app_name, string atr_name, const vector<string>& atr, int rank, unsigned int dim[] ){
    vector<const char*> cstr;
    for( unsigned int i=0; i<atr.size(); i++)
        cstr.push_back( atr[i].c_str() );
    return AddAttribute<const char*>( app_name, atr_name, cstr, H5::StrType(H5::PredType::C_S1, H5T_VARIABLE), rank, dim );
}

template <>
bool H5FileManager::AddAttribute<const char*>( string app_name, string atr_name, const vector<const char*>& atr, int rank, unsigned int dim[]  ){
    return AddAttribute<const char*>( app_name, atr_name, atr, H5::StrType(H5::PredType::C_S1, H5T_VARIABLE), rank, dim );
}

