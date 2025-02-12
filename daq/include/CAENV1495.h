#ifndef LINUX
    #define LINUX
#endif

#ifndef CAENV1495_H
#define CAENV1495_H

#include "CAENVMElib.h"
#include "CAENVMEtypes.h"
#include "VMEBoard.h"
#include "CAENV1495Parameter.h"

/// This class implements access and control of the physical V1495 board.
//
/// SetXXX methods will first modify data member in the CAENV1495Parameter class member, then call relevant ConfigRegister methods to write value onto the registers.
class CAENV1495 : public VMEBoard<CAENV1495Parameter>{

public:
    CAENV1495( int32_t h, CAENV1495Parameter p=CAENV1495Parameter());
        //!< Constructor. handle, base and V1495Param are handled in the parent abstract class already. Nothing to be done.
    ~CAENV1495();
        //!< Destructor. Since no moemry is allocated, do nothing.

    void StartBoard();
        //!< To start board, both write start bit and clear the reset bit by writing 0b11.
    void StopBoard();
        //!< To stop board write 0 to bit[0]
    void Reset();
        //!< To reset, temperarily set bit[1] to 0.

    void SetOutputMode(V1495_OMODE mode);
        //!< Sets the output mode (write to register).
        //
        //!< Output mode represented as enum, bit[3:2]
        //!< First set the parameter, and call ConfigControlReg() to update.
    V1495_OMODE GetOutputMode();
        //!< Retrieves output mode from register.
        //
        //!< output mode is enum type, explicit static cast is needed to convert from int to enum.

    void EnableRetrigger(bool crystal, bool veto);
        //!< Write to register to enable retrig on crystal (1st arg) and liquid scintillator (2nd arg).

    void SetVetoMask(uint32_t mask);
        //!< Sets liquid scintillator mask. Input is 32-bit, but will be masked with 0x3ff to take only first 10 bits.
    uint32_t GetVetoMask();
        //!< Returns an integer corresponding to the liquid scintillator mask.

    void SetMajorityLevel(uint32_t level);
        //!< Sets liquid scintillator majority level.
    uint32_t GetMajorityLevel();
        //!< Retrieves liquid scintillator majority level stored on the register.

    void SetSignalDelay(uint32_t delay);
        //!< Sets signal delay, effective only in veto mode.
    uint32_t GetSignalDelay();
        //!< Retrieves value of signal delay from register.

    void SetTriggerTime(uint32_t time);
        //!< Sets trigger output duration. First update member variable, and then write onto register by calling relevant Config method.
    uint32_t GetTriggerTime();
        //!< Returns trigger time stored on the register.

    void SetGateLenXystal(uint32_t time);
        //!< Updates member variable for crystal coincidence window length and writes register via Config function.
    uint32_t GetGateLenXystal();
        //!< Returns value stored in the register.
    void SetGateLenVeto(uint32_t time);
        //!< Updates member variable for liquid scintillator coincidence window length and writes register via Config function.
    uint32_t GetGateLenVeto();
        //!< Returns value stored in the register.

    void SetDeadTime(uint32_t time);
        //!< Changes member variable for dead time and updates register via relevant Config method.
    uint32_t GetDeadTime();
        //!< Retrieves dead time from register.
    void SetVetoTime(uint32_t time);
        //!< Changes member variable for veto window length and updates register via relevant Config method.
    uint32_t GetVetoTime();
        //!< Retrieves veto time from register.

    void TriggerFPGA();
        //!< Generates trigger upon write access.


private:

    void Initialize();
        //!< Initialize will call all Config methods successively. It is used upon construction.

    void ConfigControlReg();
        //!< Updates control register with values from member variable CAENV1495Parameter.
        //
        //!< Updated members are output mode, re-trigger, liquid scintillator mask, majority level, trigger delay in veto mode, and trigger output duration.
    void ConfigInputGate();
        //!< Updates Gate Length Register with values in the member variable CAENV1495Parameter.
        //
        //!< crystal and liquid scintillator coincidence window will be updated.
    void ConfigDeadTime();
        //!< Updates dead time and veto window length.
};

#endif
