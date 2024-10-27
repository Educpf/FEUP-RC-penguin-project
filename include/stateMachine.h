#pragma once
#include "macros.h"


enum machineStates
{
    START,
    FLAG_RCV,
    A_RCV,
    C_RCV,
    BCC_OK,
    END
};


typedef struct stateMachine
{
    unsigned char addressByte, controlByte;
    enum machineStates state;
    unsigned char buf[BUF_SIZE]; 
    unsigned int byteSize;
    unsigned repeated;

}stateMachine;




enum machineStates handleByte(unsigned char byte);


enum machineStates getMachineState();

enum machineStates getControlByte();

unsigned char isInfoRepeated();

unsigned char* getMachineData();

unsigned int getMachineDataSize();

unsigned int getFrameNum();

void cleanMachineData();

void invertControlByte();

void invertFrameNum();