#ifndef VMEBOARDPARAMETER_H
    #define VMEBOARDPARAMETER_H 1

#include <cstdint>
#include <map>
#include <queue>
#include <string>

using namespace std;

/// Base class for accessing and controlling CAEN VME boards.
//
/// It has public interface GetHandle() to access handle that is used to identify the VME bridge used to access each board.
class VMEBoardParameter{

public:

    VMEBoardParameter( );
        //!< Constructor. Initialize handle with the argument.
    
    virtual ~VMEBoardParameter();
        //!< Destructor.

    uint32_t GetBaseAddr();
        //!< Returns current board base address.
    
    void SetBaseAddr( uint32_t s);
        //!< Set board VME base address.

    uint32_t GetLinkNumber();
        //!< Returns link number for current board.
    
    void SetLinkNumber( uint32_t s);
        //!< Set link number for current board.
        
    uint32_t GetBoardNumber();
        //!< Returns current board number in daisy chain. Negative means no daisy chain is used.
    
    void SetBoardNumber( uint32_t s);
        //!< Set board number.

    uint32_t GetBoardIndex(){ return board_index;}
        //!< Board index.
        //!< This is different from board number which has physical meaning.
        //!< This index is simply the index it was specified.

    void SetBoardIndex( uint32_t s){ board_index = s;}
        //!< Sets board index.
    

    string GetMessage();
        //!< Returns accumulated messages to report status and error.

    int GetStatus();
        //!< Returns 0 if no error.
        //!< Else program should be terminated.

    virtual uint32_t GetVersion();
        //!< Returns the version number of current parameter class.

    virtual unsigned int GetHeaderSize();
        //!< Return size of header for board parameter in number of bytes.

    virtual void Serialize( char* p);
        //!< Write the details of board parameters in raw binary format.

    virtual void Deserialize( char* p, bool flip);

    uint32_t FlipByteOrder( uint32_t );

protected:

    int status;
        //!< Error status of the board. 0 - no error. negative - error.

    queue< string > message_pipe;
        //!< Accumulates error messages from the class and make them accessible to higher classes.

    
    int32_t link_number;
        //!< By storing the link number, each board can be individually initialized.
    
    int board_number;
        //!< By storing the board number, each board can be individually initialized.
        //!< Negative number means no daisy chain is used.
        //!< Direct connection is considered as a daisy chain.

    uint32_t base_addr;
        //!< VME base address of the board.
        //!< It is protected so that derived classes can access.

    uint32_t board_index;
        //!< Index of this board. Starts from 0.

//    std::map< uint32_t, pair< string, uint32_t> > register_map;

    void PushMessage( string msg ){
        message_pipe.push( msg );
    }
};

#endif
