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



// Processes byte 
// Returns the state of the machine
enum machineStates handleByte(unsigned char byte);

// Returns the current stateMachine State
enum machineStates getMachineState();

// Returns the controlByte received 
enum machineStates getControlByte();

unsigned char isInfoRepeated();

unsigned char* getMachineData();

int getMachineDataSize();

int getFrameNum();

void cleanMachineData();

void invertControlByte();

void invertFrameNum();