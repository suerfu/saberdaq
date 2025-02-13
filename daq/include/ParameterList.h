#ifndef PARAMETERLIST
    #define PARAMETERLIST

#include <map>
#include <string>

using std::map;

template < class T >

class{

public:

    ParameterList(){}
        //!< Constructor. Nothing to be done.
    
    ~ParameterList(){}
        //!< Destructor. Nothing to be done.

    void Add( string name ){
        parameters.insert( std::pair< string, T >( name, T() ));
    }

    void Add( string name, T val ){
        parameters.insert( std::pair< string, T >( name, val));
    }

    void Set( string name, T val ){
        parameters[name] = val;
    }

    T Get( string name, bool& found ){
        found = Find(name);
        if( found )
            return parameters[name];
        else
            return T();
    }

    void Rm( string name ){
        map<string, T>::iterator itr = parameters.find(name);
        if( itr!=parameters.end() )
            parameters.erase( itr );
    }

    bool Find( string name ){
        map<string, T>::iterator itr = parameters.find( name );
        if( itr!=parameters.end() )
            return true;
        else
            return false;
    }

    void Clear(){
        parameters.clear();
    }




private:

    map< string, T > parameters;

};



#endif
