#ifndef LINUX
    #define LINUX
#endif

#include "CAENVMElib.h"
#include "CAENV1495.h"

#include <ctime>
#include <cstring>

using namespace std;


CAENV1495::CAENV1495( int32_t h, CAENV1495Parameter p) : VMEBoard<CAENV1495Parameter>( h, p){
    SetParameters(p);
    Initialize();
}


CAENV1495::~CAENV1495(){}


//===================================================================
//Start, Stop and Reset are dynamic:

void CAENV1495::StartBoard(){
    uint32_t data = ReadRegister(REG_CTRL);
    SetBit(&data,3,0,1);
    WriteRegister(REG_CTRL,data);
}


void CAENV1495::StopBoard(){
    uint32_t data = ReadRegister(REG_CTRL);
    SetBit(&data,0,0,0);
    WriteRegister(REG_CTRL,data);
}


void CAENV1495::Reset(){
    uint32_t data = ReadRegister(REG_CTRL);
    SetBit(&data,0,1,1);    // set low to reset and restore.
    WriteRegister(REG_CTRL,data);
    SetBit(&data,1,1,1);
    WriteRegister(REG_CTRL,data);
}

//====================================================================
// Following methods are for REGISTER_CONTROL

void CAENV1495::SetOutputMode(V1495_OMODE mode){
    param.output_mode = mode;
    ConfigControlReg();
}


V1495_OMODE CAENV1495::GetOutputMode(){
    uint32_t data = ReadRegister(REG_CTRL);
    return static_cast<V1495_OMODE>(GetBit(data, 2, 3));
}


void CAENV1495::EnableRetrigger(bool crystal, bool veto){
    param.retrig_xystal = crystal;
    param.retrig_veto = veto;
    ConfigControlReg();
}


void CAENV1495::SetVetoMask(uint32_t level){
    param.veto_mask = level&0x3ff;
    ConfigControlReg();
}


uint32_t CAENV1495::GetVetoMask(){
    return GetBit(ReadRegister(REG_CTRL),6,15);
}


void CAENV1495::SetMajorityLevel(uint32_t level){
    param.maj_lev = level>10?10:level;
    ConfigControlReg();
}


uint32_t CAENV1495::GetMajorityLevel(){
    return GetBit(ReadRegister(REG_CTRL),16,19);
}


void CAENV1495::SetSignalDelay(uint32_t time){
    param.sig_delay = time&0x7;
    ConfigControlReg();
}


uint32_t CAENV1495::GetSignalDelay(){
    return GetBit(ReadRegister(REG_CTRL),20,22);
}


void CAENV1495::SetTriggerTime(uint32_t time){
    param.trig_time = time & 0xff;
    ConfigControlReg();
}


uint32_t CAENV1495::GetTriggerTime(){
    return GetBit(ReadRegister(REG_CTRL),23,30);
}


void CAENV1495::ConfigControlReg(){
    uint32_t data = 0x0;
    SetBit(&data,param.output_mode,2,3);
    SetBit(&data,param.retrig_xystal?1:0,4,4);
    SetBit(&data,param.retrig_veto?1:0,5,5);
    SetBit(&data,param.veto_mask,6,15);
    SetBit(&data,param.maj_lev,16,19);
    SetBit(&data,param.sig_delay,20,22);
    SetBit(&data,param.trig_time,23,30);
    WriteRegister(REG_CTRL, data);
}


void CAENV1495::ConfigInputGate(){
    uint32_t data = param.gate_len_xystal | (param.gate_len_veto<<16);
        // crystal is first 16 bits, and liquid scintillator is the last 16 bits.
    WriteRegister(GATE_LEN,data);
}


void CAENV1495::SetGateLenXystal(uint32_t time){
    param.gate_len_xystal = time&0xffff;    // take first 16 bits so it doesn't exceed range.
    ConfigInputGate();
}


uint32_t CAENV1495::GetGateLenXystal(){
    return (ReadRegister(GATE_LEN) & 0xffff);
}


void CAENV1495::SetGateLenVeto(uint32_t time){
    param.gate_len_veto = time & 0xffff;
    ConfigInputGate();
}


uint32_t CAENV1495::GetGateLenVeto(){
    return (ReadRegister(GATE_LEN)>>16) & 0xffff;
}


void CAENV1495::ConfigDeadTime(){
    uint32_t data = param.dead_time | (param.veto_time<<16);
    WriteRegister(DEAD_TIME,data);
}


void CAENV1495::SetDeadTime(uint32_t time){
    param.dead_time = time & 0xffff;
    ConfigDeadTime();
}


uint32_t CAENV1495::GetDeadTime(){
    return ReadRegister( DEAD_TIME ) & 0xffff;
}


void CAENV1495::SetVetoTime(uint32_t time){
    param.veto_time = time & 0xffff;
    ConfigDeadTime();
}


uint32_t CAENV1495::GetVetoTime(){
    return ReadRegister( DEAD_TIME )>>16 & 0xffff;
}


void CAENV1495::TriggerFPGA(){
    WriteRegister( SOFT_TRIG, 0x0a0a );
}


void CAENV1495::Initialize(){
    Reset();
    ConfigControlReg();
    ConfigInputGate();
    ConfigDeadTime();
}
