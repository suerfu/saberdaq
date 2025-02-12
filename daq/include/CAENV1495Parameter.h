#ifndef CAENV1495PARAMETER_H
#define CAENV1495PARAMETER_H

#include <cstdint>

#include "VMEBoardParameter.h"
#include "ConfigParser.h"

#include "H5FileManager.h"

/// Enum for four different trigger output modes for V1495 board.

/// In VETO mode NaI coincident signals are vetoed by majority trigger of liquid scintillator.
/// In CRYSTAL mode, output is solely triggered by coincidence of NaI crystal.
/// In LSCINTILLATOR mode, output is solely triggered by majority trigger of liquid scintillator.
/// In CALIBRATION mode, output is triggered by the presence of either crystal coincidence or majority trigger of liquid scintillator.
/// The enumeration is in this order because the numerical number of each item corresponds to the register value that must be written to the FPGA register.

enum V1495_OMODE{
    VETO=0,
    CRYSTAL=1,
    LSCINTILLATOR=2,
    CALIBRATION=3
};


/// Register address of V1495 FPGA board.

/// REG_CTRL (register control) controls start, stop of run, majority level, veto mask, output duration, etc.
/// GATE_LEN controls the coincidence window of NaI crystal detector (first 16 bits) and liquid scintillator (last 16-bit).
/// DEAD_TIME controls the dead time after successful trigger (first 16-bit) and veto window (last 16-bit, useful only in the veto mode).
/// SOFT_TRIG generates software trigger upon write access. There is no way to disable this feature.
/// TIME_BOMB prevents board hanging up due to time bomb triggered. This register will be disabled in the entire application.

enum V1495_REG{
    REG_CTRL=0x1030,
    GATE_LEN=0x1034,
    DEAD_TIME=0x1038,
    SOFT_TRIG=0x1042,
    TIME_BOMB=0x100C
};


/// class containing relevant parameters for V1495 board.
//
/// Base address and handle is created by the constructor.

class CAENV1495Parameter : public VMEBoardParameter{

public:

    CAENV1495Parameter();

    ~CAENV1495Parameter();

    string GetPrintString();

    void SetParamFromConfig( ConfigParser*, string s="/module/daq/logic_trigger/");
        //!< This static method is used to retrieve parameters from user-specified config file via SVParamHandler object and returns a corresponding CAENV1495Parameter object.

    V1495_OMODE output_mode;
        //!< Output mode.

    string GetModeString( V1495_OMODE );
        //!< convert the specified output mode to actual string describing the mode.

    bool retrig_xystal;      //!< Enable crystal re-trigger (extend previous trigger upon arrival of next trigger).
    bool retrig_veto;        //!< Enable liquid scintillator re-trigger.

    uint32_t veto_mask; //!< Mask for liquid scintillator signals.

    uint32_t maj_lev;    //!< Majority level for triggering the liquid scintillator.

    uint32_t sig_delay;  //!< Delays crystal coincidence trigger to allow for veto.

    uint32_t trig_time; //!< Duration of global trigger output.

    uint32_t gate_len_xystal;   //!< Crystal coincidence window.
    uint32_t gate_len_veto;     //!< Liquid scintillator coincidence window.

    uint32_t dead_time; //!< Dead time imposed after successful global trigger.
    uint32_t veto_time; //!< Veto window. Useful only in veto mode.

    bool enable;        //!< Whether this board is enabled or not.

    unsigned int GetHeaderSize();

    void Serialize( char* p );

    void Deserialize( ifstream& p );

    void Deserialize( char* p, bool flip = false );

    uint32_t GetVersion(){ return 0;}

    void ExportHDF5( H5FileManager* h5man);

};

#endif
