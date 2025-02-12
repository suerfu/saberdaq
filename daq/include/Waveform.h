// Waveform object. This function is templated and can be used to store various objects:
// int for raw digitizer counts, and float for processed waveform.
// This class is a wrapper for vector.

#ifndef WAVEFORM_H
#define WAVEFORM_H 1

#include <vector>
#include <algorithm>
#include <iostream>

using std::vector;
using std::cout;
using std::endl;

template < class T >
class Waveform{

public:

    Waveform(){;}
    
    Waveform( vector<T> vec ){ waveform = vec;}

    ~Waveform(){}

    Waveform( const Waveform& rhs){ waveform = rhs.waveform;}

    Waveform& operator=( const Waveform& rhs){
        waveform = rhs.waveform;
        return *this;
    }

    unsigned int size(){ return waveform.size(); }

    bool empty(){ return waveform.empty(); }

    void push_back( T obj ){ waveform.push_back( obj ); }

    void reserve( int n){ waveform.reserve(n); }

    T& operator[]( int index){ return waveform[index]; }
    
    //const T& operator[]( int index){ return waveform[index]; }

    void clear(){ waveform.clear(); }

    virtual float Height( unsigned int a, unsigned int b){
        return *std::min_element( waveform.begin()+a, waveform.begin()+b );    
    }

    virtual float Integral( unsigned int a, unsigned int b ){
        float integral = 0;
        for( unsigned int i=a; i<b; i++){
            integral += ( (*this)[i] + (*this)[i+1])/2;
        }
        return integral;
    }
        //!< Pulse integral from begin to end
        //
    virtual float CWMT( unsigned int a, unsigned int b ){
        float sum = 0;
        float weight = 0;
        for( unsigned int i=a; i<b; i++){
            sum += (i-a)*(*this)[i];
            weight += (*this)[i];
        }
        return sum/weight;
    }
        //!< Pulse integral from begin to end
    

    int Max(){ return *std::max_element( waveform.begin(), waveform.end() );}

    int Min(){ return *std::min_element( waveform.begin(), waveform.end() );}

    int MaxPosition(){ return *std::max_element( waveform.begin(), waveform.end() ) - waveform.begin();}

    int MinPosition(){ return *std::min_element( waveform.begin(), waveform.end() ) - waveform.begin();}

    typename vector<T>::iterator begin(){ return waveform.begin();}

    typename vector<T>::iterator end(){ return waveform.end();}

    typename vector<T>::const_iterator cbegin() const { return waveform.cbegin();}

    typename vector<T>::const_iterator cend() const { return waveform.cend();}

private:

    vector< T > waveform;

};

#endif
