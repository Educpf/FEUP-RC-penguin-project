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


// Processes byte 
// Returns the state of the machine
enum machineStates handleByte(unsigned char byte);

// Returns the current stateMachine State
enum machineStates getMachineState();

// Returns the controlByte received 
enum machineStates getControlByte();

// Returns whether frame is repeated or not
unsigned char isInfoRepeated();

// Returns data buffer
unsigned char* getMachineData();

// Returns data buffer size
int getMachineDataSize();

// Returns current frame number (0 or 1)
int getFrameNum();

// Reset machine's variables
void cleanMachineData();

// Toggle Information Control Byte (0x80 or 0x00)
void invertControlByte();

// Toggle Information Frame Number (0x80 or 0x00)
void invertFrameNum();