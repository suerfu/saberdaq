#ifndef LINUX
#define LINUX
#endif

#ifndef VMEBOARD_H
#define VMEBOARD_H 1

#include "CAENVMElib.h"
#include "CAENVMEtypes.h"

#include <iostream>
#include <string>
#include <queue>
#include <deque>

using namespace std;

/// VMEBoard is the abstract base class for CAEN VME boards.

/// Currently digitizer V1720 and trigger V1495 are implemented.
/// The board contains a template param to store board parameters.
/// Parameter is set by SetParameters and retrieved by GetParameters.
/// Each derived class will use GetParamFromConfig to set param object from user input file.
/// It also provides Register read and write convenience functions.

template <typename T>
class VMEBoard{

public:
    
    VMEBoard( int32_t _handle, T param = T());
        //!< Constructor.
        //!< Upon initialization, handle is needed to access VME board.
        //!< Template is used to specify and set board parameters.
    
    virtual ~VMEBoard();
        //!< Destructor.

    void SetParameters(const T& p);
        //!< It reads parameters from the input argument into the class member parameter.
    
    T GetParameters(){ return param;}
        //!< Access methods for parameter structs.

    virtual void StartBoard() = 0;

    virtual void StopBoard() = 0;

    virtual void Reset() = 0;
        //!< Calls software reset method on the board.

    void PrintRegister( uint32_t reg );

    string GetMessage(){
        if( message_pipe.empty() )
            return "";
        else{
            string s = message_pipe.front();
            message_pipe.pop();
            return s;
        }
    }

    int GetStatus(){
        return status;
    }

protected:

    int status;
        //!< Error status of the board. 0 - no error. negative - error.

    queue< string > message_pipe;
        //!< Accumulates error messages from the class and make them accessible to higher classes.

    void PushMessage( string msg ){
        message_pipe.push( msg );
    }

    T param;
        //!< Template member holding board parameters. It must contain member base_addr.

    int32_t handle;
        //!< 32-bit number identifying the connection device (used in VME access).

    uint32_t base_addr;

    virtual void Initialize() = 0;
        //!< This method actually writes board parameters onto respective VME registers.

    uint32_t ReadRegister(uint32_t reg);
        //!< Read 32-bit data from register. For this method to work correctly, correct handle and base_addr must be given.

    CVErrorCodes WriteRegister(uint32_t reg, uint32_t data);
        //!< Write 32-bit data onto VME register.

    uint32_t SetBit(uint32_t oval, int32_t nval, int l, int h);
        //!< This function modifies bit fields of the variable and returns the new value.
        //!< Arguments are old-value, new-value, low and high position of the bit field to modify.
    
    uint32_t SetBit(uint32_t* oval, uint32_t nval, int l, int h);
        //!< This version modifies the bit fields directly via pointer.
    
    uint32_t GetBit(uint32_t val, int l, int h);
        //!< Returns integer corresponding to the low to high bits of the argument.

};



template <class T>
VMEBoard<T>::VMEBoard( int32_t h, T p) : param(p), handle(h){
    base_addr = param.GetBaseAddr();
    // calling abstract method in a constructor is not safe, since base constructor will be called first.
}


template <class T>
VMEBoard<T>::~VMEBoard(){;}


template <class T>
void VMEBoard<T>::SetParameters(const T& p){
    param = p;
    base_addr = param.GetBaseAddr();
}


template <class T>
uint32_t VMEBoard<T>::ReadRegister(uint32_t reg){
    uint32_t data = 0x0;
    CVErrorCodes er = CAENVME_ReadCycle( handle, base_addr+reg, &data, cvA32_U_DATA, cvD32);
    if( er!=cvSuccess ){
        std::cerr << "WARNING: Failed to read register 0x" << std::hex << base_addr+reg << std::endl << CAENVME_DecodeError( er) << std::endl;
        std::cerr.flush();
        er = CAENVME_ReadCycle( handle, base_addr+reg, &data, cvA32_U_DATA, cvD32);
    }
    return data;
}


template <class T>
CVErrorCodes VMEBoard<T>::WriteRegister(uint32_t reg, uint32_t d){
    uint32_t data = d;
    CVErrorCodes er = CAENVME_WriteCycle(handle, base_addr+reg, &data, cvA32_U_DATA, cvD32);
    if( er!=cvSuccess ){
        std::cerr << "WARNING: Failed to write to register 0x" << std::hex << base_addr+reg << " with value " << d << std::endl;
        std::cerr << CAENVME_DecodeError( er) << std::endl;
        std::cerr.flush();
    }
    //std::cout<<" ***\tWriting register\t0x"<<std::hex<<reg<<" with value 0x"<<std::hex<<d<<std::endl;
    return er;
}


template <class T>
uint32_t VMEBoard<T>::SetBit(uint32_t oval, int32_t nval, int l, int h){
    int nbits = 8*sizeof(oval);
    if( l<0 || h>=nbits ){
        cout<<"---\tERROR: Bitfield out of range: "<<l<<" "<<h<<endl;
        return oval;
    }
    int low = l<h?l:h;    int high = h>l?h:l;
    if( nval>~((~0)<<(high-low+1)) ){
        nval = ~((~0)<<(high-low+1));
        cout<<"---\tWARNING: New value out of range. Setting it to "<<nval<<endl;
    }
    uint32_t mask = 0;
    if(high<nbits) mask = ( ((~0)<<(high+1)) ) | ~((~0)<<low) ;
    else mask = ~((~0)<<low) ;
    return ( oval & mask) | (nval<<low);
}


template <class T>
uint32_t VMEBoard<T>::SetBit(uint32_t* oval, uint32_t nval, int l, int h){
    uint32_t old = *oval;
    *oval = SetBit(old, nval, l, h);
    return (*oval);
}


template <class T>
uint32_t VMEBoard<T>::GetBit(uint32_t val, int l, int h){
    if( l<int(0) || h>=int(8*sizeof(val)) ){
        cout<<"---\tERROR: Bitfield out of range: "<<l<<" "<<h<<endl;
        return 0;
    }
    int low = l<h?l:h;    int high = h>l?h:l;
    uint32_t ones = -1;
    uint32_t mask = ~(ones<<(high-low+1));
    return ( mask & (val>>low) );
}



template <class T>
void VMEBoard<T>::PrintRegister(uint32_t reg){
    std::cout<<"Printing Register "<<std::hex<<reg<<" :\t"<<std::hex<<ReadRegister(reg)<<std::endl;
}

#endif
